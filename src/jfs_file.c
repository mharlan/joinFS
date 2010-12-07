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

static int get_file_inode(int q_inode);
static char *get_datapath(char *uuid);
static char *get_filename(const char *path);

int
jfs_s_file_create(const char *path, int q_inode, mode_t mode)
{
  char *uuid;
  char *datapath;
  char *filename;
  int inode;
  int rc;
  struct jfs_db_op *db_op;

  uuid = jfs_create_uuid();
  jfs_generate_uuid(uuid);

  datapath = get_datapath(uuid);
  filename = get_filename(path);
  inode = open(datapath, O_CREAT | O_EXCL | O_WRONLY, mode);

  if(inode >= 0) {
	db_op = jfs_db_op_create();
	db_op->res_t = jfs_write_op;
	snprintf(db_op->query, JFS_QUERY_MAX,
			 "INSERT INTO files VALUES(%d,1,%s,%s); INSERT INTO symlinks VALUES(%d,%d);",
			 inode, datapath, filename, q_inode, inode);

	jfs_write_pool_queue(db_op);
	jfs_db_op_wait(db_op);

	if(db_op->error == JFS_QUERY_FAILED) {
	  rc = -1;
	}
  }
  else {
	rc = -1;
  }

  return rc;
}

int 
jfs_s_file_open(const char *path, struct fuse_file_info *fi)
{
  int inode;

  inode = 0;

  return get_file_inode(inode);
}

int 
jfs_s_file_read(const char *path, char *buf, size_t size, off_t offset,
				struct fuse_file_info *fi)
{
  return 0;
}

int 
jfs_s_file_write(const char *path, const char *buf, size_t size,
				 off_t offset, struct fuse_file_info *fi)
{
  return 0;
}

int 
jfs_s_file_truncate(const char *path, off_t size)
{
  return 0;
}

int 
jfs_s_file_access(const char *path, int mask)
{
  return 0;
}

static int 
get_file_inode(int q_inode)
{
  struct jfs_db_op *db_op;

  if(jfs_file_cache_has(q_inode)) {
	return jfs_file_cache_value(q_inode);
  }
  else {
	db_op = jfs_db_op_create();
	db_op->res_t = jfs_s_file;
	snprintf(db_op->query, JFS_QUERY_MAX,
			 "SELECT inode FROM symlinks WHERE linknode=\"%d\";",
			 q_inode);

	jfs_read_pool_queue(db_op);
	jfs_db_op_wait(db_op);
  }

  return 0;
}

static char *
get_datapath(char *uuid)
{
  int path_len;
  char *path;

  path_len = JFS_CONTEXT->rootlen + JFS_UUID_LEN + 1;
  path = malloc(sizeof(*path) * path_len);
  if(path) {
	snprintf(path, path_len, "%s\%s", JFS_CONTEXT->rootdir, uuid);
  }
  else {
	log_error("Unable to allocate memory for a data path.\n");
  }

  return path;
}

static char *
get_filename(const char *path)
{
  char *filename;

  filename = strrchr(path, '/');

  return ++filename;
}
