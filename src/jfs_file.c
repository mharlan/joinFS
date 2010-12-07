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

static int get_datainode(int syminode);
static char *get_datapath(int datainode);
static char *create_datapath(char *uuid);
static char *get_filename(const char *path);

/*
 * Create a joinFS static file. The file is added
 * to the Linux VFS and the database.
 *
 * Returns the inode of the newly created file.
 */
int
jfs_file_create(const char *path, int syminode, mode_t mode)
{
  char *uuid;
  char *datapath;
  char *filename;
  int datainode;
  struct jfs_db_op *db_op;

  uuid = jfs_create_uuid();
  jfs_generate_uuid(uuid);

  datapath = create_datapath(uuid);
  filename = get_filename(path);
  datainode = open(datapath, O_CREAT | O_EXCL | O_WRONLY, mode);

  if(inode >= 0) {
	rc = close(datainode);

	db_op = jfs_db_op_create();
	db_op->res_t = jfs_write_op;
	snprintf(db_op->query, JFS_QUERY_MAX,
             "INSERT INTO files VALUES(NULL, %d, 1, \"%s\", \"%s\"); INSERT INTO symlinks VALUES(%d,%d,(SELECT fileid FROM files WHERE inode=%d));",
			 datainode, datapath, filename, syminode, datainode, datainode);

	jfs_write_pool_queue(db_op);
	jfs_db_op_wait(db_op);

	if(db_op->error == JFS_QUERY_FAILED) {
	  log_error("New file database inserts failed.\n");
	  rc = unlink(datapath);
	  free(datapath);
	  return -1;
	}

	jfs_file_cache_add(syminode, datainode);
	jfs_db_op_destroy(db_op);
  }
  else {
	free(datapath);
	return -1;
  }

  free(datapath);

  return 0;
}

/*
 * Delets a joinFS static file.
 */
int
jfs_file_unlink(const char *path)
{
  int datatinode;
  char *datapath;
  int rc;

  datainode = get_datainode(path);
  datapath = get_datapath(datainode);

  rc = unlink(path);
  if(rc) {
	return rc;
  }

  return unlink(datapath);
}

/*
 * Perform a rename on a joinFS file.
 */
int
jfs_file_rename(const char *from, const char *to)
{
  int syminode;
  int datainode;
  char *filename;
  struct jfs_db_op *db_op;

  syminode = open(from, O_RDONLY);
  if(syminode >= 0) {
	datainode = get_datainode(syminode);
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
	}
  }
  
  return rename(from, to);
}

/*
 * Truncate a joinFS file.
 */
int
jfs_file_truncate(const char *path, off_t size)
{
  char *datapath;

  datapath = get_datapath(path);

  return truncate(datapath, size);
}

/*
 * Open a joinFS file.
 */
int
jfs_file_open(const char *path, int flags)
{
  char *datapath;

  datapath = get_datapath(path);
  
  return open(datapath, flags);
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

  syminode = open(path, O_RDONLY);
  if(syminode >= 0) {
	close(syminode);

	datainode = jfs_file_cache_get(syminode);
	if(!datainode) {
	  db_op = jfs_db_op_create();
	  db_op->res_t = jfs_s_file;
	  snprintf(db_op->query, JFS_QUERY_MAX,
			   "SELECT datainode FROM symlinks WHERE syminode=\"%d\";",
			   q_inode);
	  
	  jfs_read_pool_queue(db_op);
	  jfs_db_op_wait(db_op);
	  
	  datainode = db_op->inode;
	  if(!datainode) {
		log_error("Failed to find inode. Query=%s.\n", db_op->query);
		return -1;
	  }
	  
	  jfs_file_cache_add(syminode, datainode);
	  jfs_db_op_destroy(db_op);
	}
	
	return datainode;
  }
  
  return -1;
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
  db_op->res_t = jfs_datapath;
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

  return result->datapath;
}


/*
 * Get the location where the data will be saved.
 * 
 * joinFS_root_dir/.data/uuid-string
 */
static char *
create_datapath(char *uuid)
{
  int path_len;
  char *path;

  path_len = JFS_CONTEXT->rootlen + JFS_UUID_LEN + 7;
  path = malloc(sizeof(*path) * path_len);
  if(path) {
	snprintf(path, path_len, "%s/.data/%s", JFS_CONTEXT->rootdir, uuid);
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
