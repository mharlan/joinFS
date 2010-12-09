/*
 * joinFS - File Module
 * Matthew Harlan <mharlan@gwmail.gwu.edu>
 *
 * 30 % Demo
 */

#include "error_log.h"
#include "jfs_uuid.h"
#include "jfs_file.h"
#include "jfs_file_cache.h"
#include "sqlitedb.h"
#include "joinfs.h"

#include <fuse.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

static int jfs_file_db_add(int syminode, int datainode, char *datapath, char *filename, int mode);
static int get_inode(const char *path);
static char *get_datapath(const char *path);
static char *create_datapath(char *uuid);
static char *get_filename(const char *path);

/*
 * Create a joinFS static file. The file is added
 * to the Linux VFS and the database.
 *
 * Returns the inode of the newly created file or -1.
 */
int
jfs_file_create(const char *path, mode_t mode)
{
  char *uuid;
  char *datapath;
  char *filename;

  int datainode;
  int syminode;
  int rc;
  int fd;

  log_error("Called jfs_file_create.\n");
  rc = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
  if(rc < 0) {
	return -1;
  }
  rc = close(rc);

  syminode = get_inode(path);
  if(syminode < 0) {
	return -1;
  }

  uuid = jfs_create_uuid();
  jfs_generate_uuid(uuid);

  datapath = create_datapath(uuid);
  filename = get_filename(path);

  log_error("Datapath:%s\n", datapath);

  fd = creat(datapath, mode);
  if(fd < 0) {
	free(datapath);
	return -1;
  }

  datainode = get_inode(datapath);
  if(datainode < 0) {
	free(datapath);
	return -1;
  }

  rc = jfs_file_db_add(syminode, datainode, datapath, filename, mode);
  if(rc < 0) {
	printf("New file database inserts failed.\n");
  }

  free(datapath);

  return fd;
}

/*
 * Create a joinFS static file. The file is added
 * to the Linux VFS and the database.
 *
 * Returns 0 or -1 if there was an error.
 */
int
jfs_file_mknod(const char *path, mode_t mode)
{
  char *uuid;
  char *datapath;
  char *filename;

  int datainode;
  int syminode;
  int rc;

  log_error("Called jfs_file_mknod.");
  rc = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
  if(rc < 0) {
	return -1;
  }
  rc = close(rc);

  syminode = get_inode(path);
  if(syminode < 0) {
	return -1;
  }

  uuid = jfs_create_uuid();
  jfs_generate_uuid(uuid);

  datapath = create_datapath(uuid);
  jfs_destroy_uuid(uuid);

  filename = get_filename(path);

  log_error("Datapath:%s\n", datapath);

  rc = open(datapath, O_CREAT | O_EXCL | O_WRONLY, mode);
  if(rc < 0) {
	free(datapath);
	return -1;
  }
  rc = close(rc);

  datainode = get_inode(datapath);
  if(datainode < 0) {
	free(datapath);
	return -1;
  }
  
  rc = jfs_file_db_add(syminode, datainode, datapath, filename, mode);
  if(rc < 0) {
	printf("New file database inserts failed.\n");
  }

  free(datapath);

  return rc;;
}

/*
 * Used by jfs_file_create and jfs_file_mknod to add
 * the file to the database.
 *
 * It is two db_ops because I can't compile two statements 
 * into a single sqlite3_stmt
 */
static int
jfs_file_db_add(int syminode, int datainode, char *datapath, char *filename, int mode)
{
  struct jfs_db_op *db_op;

  /* first add to the files table */
  db_op = jfs_db_op_create();
  db_op->res_t = jfs_write_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "INSERT INTO files VALUES(NULL, %d, %d, \"%s\", \"%s\");",
		   datainode, mode, datapath, filename);
  
  jfs_write_pool_queue(db_op);
  jfs_db_op_wait(db_op);

  if(db_op->error == JFS_QUERY_FAILED) {
	jfs_db_op_destroy(db_op);
	return -1;
  }
  jfs_db_op_destroy(db_op);

  /* now add to symlinks */
  db_op = jfs_db_op_create();
  db_op->res_t = jfs_write_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "INSERT INTO symlinks VALUES(%d,%d,(SELECT fileid FROM files WHERE inode=%d));",
		   syminode, datainode, datainode);
  
  jfs_write_pool_queue(db_op);
  jfs_db_op_wait(db_op);

  if(db_op->error == JFS_QUERY_FAILED) {
	jfs_db_op_destroy(db_op);
	return -1;
  }
  jfs_db_op_destroy(db_op);

  //jfs_file_cache_add(syminode, datainode, datapath);

  return 0;
}

/*
 * Delets a joinFS static file.
 */
int
jfs_file_unlink(const char *path)
{
  struct jfs_db_op *db_op;
  char *datapath;
  int datainode;
  int rc;

  printf("---Started file UNLINK.\n");

  datapath = get_datapath(path);
  if(!datapath) {
	printf("Failed to get datapath.\n");
	return -1;
  }

  rc = unlink(path);
  if(rc) {
	printf("Unlink for path:%s failed.\n", path);
	return rc;
  }
  
  datainode = get_inode(datapath);
  if(datainode >= 0) {
	db_op = jfs_db_op_create();
	db_op->res_t = jfs_write_op;
	snprintf(db_op->query, JFS_QUERY_MAX,
			 "DELETE FROM files WHERE inode=%d;",
			 datainode);

	printf("Delete query called:%s\n", db_op->query);
	
	jfs_write_pool_queue(db_op);
	jfs_db_op_wait(db_op);
	
	if(db_op->error == JFS_QUERY_FAILED) {
	  printf("Delete from files table failed.\n");
	  free(datapath);
	  return -1;
	}
	jfs_db_op_destroy(db_op);

	db_op = jfs_db_op_create();
	db_op->res_t = jfs_write_op;
	snprintf(db_op->query, JFS_QUERY_MAX,
			 "DELETE FROM symlinks WHERE datainode=%d;",
			 datainode);

	printf("Delete query called:%s\n", db_op->query);
	
	jfs_write_pool_queue(db_op);
	jfs_db_op_wait(db_op);
	
	if(db_op->error == JFS_QUERY_FAILED) {
	  printf("Delete from symlinks table failed.\n");
	  free(datapath);
	  return -1;
	}
	jfs_db_op_destroy(db_op);
  }

  if(!datapath) {
	printf("No datapath.\n");
	return -1;
  }
  

  rc = unlink(datapath);
  if(rc) {
	printf("Unlink on path:%s failed.\n", datapath);
  }
  free(datapath);

  return rc;
}

/*
 * Perform a rename on a joinFS file.
 */
int
jfs_file_rename(const char *from, const char *to)
{
  int rc;
  int syminode;
  char *filename;
  struct jfs_db_op *db_op;

  printf("Called jfs_rename, from:%s to:%s\n", from, to);

  syminode = get_inode(from);
  if(syminode >= 0) {
	filename = get_filename(to);
	
	db_op = jfs_db_op_create();
	db_op->res_t = jfs_write_op;
	snprintf(db_op->query, JFS_QUERY_MAX,
			 "UPDATE files SET filename=\"%s\" WHERE inode=(SELECT datainode FROM symlinks WHERE syminode=%d);",
			 filename, syminode);

	printf("Rename query called:%s\n", db_op->query);
	
	jfs_write_pool_queue(db_op);
	jfs_db_op_wait(db_op);

	if(db_op->error == JFS_QUERY_FAILED) {
	  return -1;
	}

	printf("Database cleanup.\n");
	
	jfs_db_op_destroy(db_op);

	printf("Renaming from:%s , to:%s\n", from, to);

	rc = open(to, O_CREAT | O_EXCL);
	if(rc < 0) {
	  rc = jfs_file_unlink(to);
	}

	return rename(from, to);
  }

  printf("Syminode not found for rename of from:%s.\n", from);
  
  return -1;
}

/*
 * Truncate a joinFS file.
 */
int
jfs_file_truncate(const char *path, off_t size)
{
  char *datapath;
  int rc;

  datapath = get_datapath(path);
  if(!datapath) {
	return -1;
  }

  rc = truncate(datapath, size);
  free(datapath);

  return rc;
}

/*
 * Open a joinFS file.
 */
int
jfs_file_open(const char *path, int flags)
{
  char *datapath;
  int rc;

  printf("Called jfs_file_open.\n");
  //if the create flag is set, check to see if the file exists
  //if it does, call jfs_file_create
  if(flags & O_CREAT) {
	printf("Open called with create flag set.\n");
	rc = open(path, O_CREAT | O_EXCL | O_WRONLY);
	if(rc < 0) {
	  close(rc);
	  rc = unlink(path);
	  return jfs_file_create(path, flags);
	}
  }

  datapath = get_datapath(path);
  if(!datapath) {
	return -1;
  }

  printf("Opening file at datapath, %s\n", datapath);
  
  rc = open(datapath, flags);
  free(datapath);

  return rc;
}


/*
 * Get the location where the data is saved.
 *
 * The result must be freed. Returns 0 on failure.
 */
static char *
get_datapath(const char *path)
{
  int syminode;
  int datainode;
  char *datapath;
  struct jfs_db_op *db_op;
												
  syminode = get_inode(path);
  if(syminode < 0) {
	return 0;
  }

  printf("Checking cache for syminode:%d\n", syminode);
  //datapath = jfs_file_cache_get_datapath(syminode);
  datapath = 0;
  printf("Cache returned datapath:%s\n", datapath);
  if(!datapath) {
	db_op = jfs_db_op_create();
	db_op->res_t = jfs_datapath_op;
	snprintf(db_op->query, JFS_QUERY_MAX,
			 "SELECT datapath FROM files WHERE inode=(SELECT datainode FROM symlinks WHERE syminode=%d);",
			 syminode);
	
	jfs_read_pool_queue(db_op);
	jfs_db_op_wait(db_op);

	if(db_op->error == JFS_QUERY_FAILED) {
	  return 0;
	}

	if(!db_op->result->datapath) {
	  return 0;
	}
	
	datapath = malloc(sizeof(*datapath) * strlen(db_op->result->datapath) + 1);
	if(datapath) {
	  strcpy(datapath, db_op->result->datapath);
	}
		
	jfs_db_op_destroy(db_op);

	datainode = get_inode(datapath);
	if(datainode >= 0) {
	  jfs_file_cache_add(syminode, datainode, datapath);
	}
  }

  printf("Returning datapath:%s\n", datapath);

  return datapath;
}


/*
 * Get the location where the data will be saved.
 * 
 * joinFS_root_dir/.data/uuid-string
 *
 * The result datapath must be freed
 */
static char *
create_datapath(char *uuid)
{
  int path_len;
  char *path;

  path_len = JFS_CONTEXT->datapath_len + JFS_UUID_LEN + 1;
  path = malloc(sizeof(*path) * path_len);
  if(path) {
	snprintf(path, path_len, "%s/%s", JFS_CONTEXT->datapath, uuid);
  }
  else {
	log_error("Unable to allocate memory for a data path.\n");
  }

  return path;
}

/*
 * Get a filename from a path.
 */
static char *
get_filename(const char *path)
{
  char *filename;

  filename = strrchr(path, '/');

  return ++filename;
}

/*
 * Get the syminode from the path.
 */
static int
get_inode(const char *path)
{
  struct stat buf;
  int rc;

  rc = stat(path, &buf);
  if(rc < 0) {
	return -1;
  }

  return buf.st_ino;
}
