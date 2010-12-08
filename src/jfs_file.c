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
static int get_datainode(const char *path);
static char *get_datapath(int datainode);
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

  struct stat buf;

  log_error("Called jfs_file_create.");
  rc = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
  if(rc < 0) {
	return -1;
  }
  rc = close(rc);

  rc = stat(path, &buf);
  syminode = buf.st_ino;

  uuid = jfs_create_uuid();
  jfs_generate_uuid(uuid);

  datapath = create_datapath(uuid);
  filename = get_filename(path);

  log_error("Datapath:%s\n", datapath);

  fd = creat(datapath, mode);
  if(fd < 0) {
	rc = unlink(path);
	free(datapath);
	return -1;
  }

  rc = stat(datapath, &buf);
  datainode = buf.st_ino;

  rc = jfs_file_db_add(syminode, datainode, datapath, filename, mode);
  if(rc < 0) {
	log_error("New file database inserts failed.\n");
	rc = unlink(path);
	rc = unlink(datapath);
	free(datapath);
	return -1;
  }

  jfs_file_cache_add(syminode, datainode);

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

  struct stat buf;

  log_error("Called jfs_file_mknod.");
  rc = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
  if(rc < 0) {
	return -1;
  }
  rc = close(rc);

  rc = stat(path, &buf);
  syminode = buf.st_ino;

  uuid = jfs_create_uuid();
  jfs_generate_uuid(uuid);

  datapath = create_datapath(uuid);
  filename = get_filename(path);

  log_error("Datapath:%s\n", datapath);

  rc = open(datapath, O_CREAT | O_EXCL | O_WRONLY, mode);
  if(rc < 0) {
	rc = unlink(path);
	free(datapath);
	return -1;
  }
  rc = close(rc);

  rc = stat(datapath, &buf);
  datainode = buf.st_ino;
  
  rc = jfs_file_db_add(syminode, datainode, datapath, filename, mode);
  if(rc < 0) {
	log_error("New file database inserts failed.\n");
	rc = unlink(path);
	rc = unlink(datapath);
	free(datapath);
	return -1;
  }

  jfs_file_cache_add(syminode, datainode);

  free(datapath);

  return 0;
}

/*
 * Used by jfs_file_create and jfs_file_mknod to add
 * the file to the database.
 */
static int
jfs_file_db_add(int syminode, int datainode, char *datapath, char *filename, int mode)
{
  struct jfs_db_op *db_op;

  db_op = jfs_db_op_create();
  db_op->res_t = jfs_write_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "INSERT INTO files VALUES(NULL, %d, %d, \"%s\", \"%s\"); INSERT INTO symlinks VALUES(%d,%d,(SELECT fileid FROM files WHERE inode=%d));",
		   datainode, mode, datapath, filename, syminode, datainode, datainode);

  log_error("Create query:%s\n", db_op->query);
  
  jfs_write_pool_queue(db_op);
  jfs_db_op_wait(db_op);

  if(db_op->error == JFS_QUERY_FAILED) {
	jfs_db_op_destroy(db_op);
	return -1;
  }

  jfs_file_cache_add(syminode, datainode);
  jfs_db_op_destroy(db_op);

  return 0;
}

/*
 * Delets a joinFS static file.
 */
int
jfs_file_unlink(const char *path)
{
  char *datapath;
  int datainode;
  int rc;

  datainode = get_datainode(path);
  datapath = get_datapath(datainode);

  rc = unlink(path);
  if(rc) {
	free(datapath);
	return rc;
  }

  rc = unlink(datapath);
  free(datapath);

  return rc;
}

/*
 * Perform a rename on a joinFS file.
 */
int
jfs_file_rename(const char *from, const char *to)
{
  int datainode;
  char *filename;
  struct jfs_db_op *db_op;

  datainode = get_datainode(from);
  if(datainode >= 0) {
	filename = get_filename(to);
	
	db_op = jfs_db_op_create();
	db_op->res_t = jfs_write_op;
	snprintf(db_op->query, JFS_QUERY_MAX,
			 "UPDATE files SET filename=\"%s\" WHERE inode=%d;",
			 filename, datainode);
	
	jfs_write_pool_queue(db_op);
	jfs_db_op_wait(db_op);
	
	jfs_db_op_destroy(db_op);

	return rename(from, to);
  }
  
  return -1;
}

/*
 * Truncate a joinFS file.
 */
int
jfs_file_truncate(const char *path, off_t size)
{
  char *datapath;
  int datainode;
  int rc;

  datainode = get_datainode(path);
  datapath = get_datapath(datainode);

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
  int datainode;
  int rc;

  datainode = get_datainode(path);
  datapath = get_datapath(datainode);

  log_error("Opening file at datapath, %s\n", datapath);
  
  rc = open(datapath, flags);
  free(datapath);

  return rc;
}

/*
 * Gets the datainode associated with a joinFS static file symlink.
 *
 * Returns the datainode or -1.
 */
static int 
get_datainode(const char *path)
{
  struct jfs_db_op *db_op;
  int syminode;
  int datainode;
  int rc;
  struct stat buf;

  rc = open(path, O_RDONLY);
  if(rc < 0) {
	return -1;
  }
  rc = close(rc);

  rc = stat(path, &buf);
  syminode = buf.st_ino;

  datainode = jfs_file_cache_get(syminode);
  if(!datainode) {
	db_op = jfs_db_op_create();
	db_op->res_t = jfs_s_file;
	snprintf(db_op->query, JFS_QUERY_MAX,
			 "SELECT datainode FROM symlinks WHERE syminode=\"%d\";",
			 syminode);
	
	jfs_read_pool_queue(db_op);
	jfs_db_op_wait(db_op);
	
	datainode = db_op->result->inode;
	if(!datainode) {
	  log_error("Failed to find inode. Query=%s.\n", db_op->query);
	  return -1;
	}
	
	jfs_file_cache_add(syminode, datainode);
	jfs_db_op_destroy(db_op);
  }
  
  return datainode;
}

/*
 * Get the location where the data is saved.
 *
 * The result must be freed.
 */
static char *
get_datapath(int datainode)
{
  char *datapath;
  struct jfs_db_op *db_op;

  db_op = jfs_db_op_create();
  db_op->res_t = jfs_datapath_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "SELECT datapath FROM files WHERE inode=\"%d\";",
		   datainode);
	  
  jfs_read_pool_queue(db_op);
  jfs_db_op_wait(db_op);

  datapath = malloc(sizeof(*datapath) * strlen(db_op->result->datapath) + 1);
  if(datapath) {
	strcpy(datapath, db_op->result->datapath);
  }


  jfs_db_op_destroy(db_op);

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
