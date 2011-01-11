/********************************************************************
 * Copyright 2010, 2011 Matthew Harlan <mharlan@gwmail.gwu.edu>
 *
 * This file is part of joinFS.
 *	 
 * JoinFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * JoinFS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with joinFS.  If not, see <http://www.gnu.org/licenses/>.
 ********************************************************************/

#include "error_log.h"
#include "jfs_uuid.h"
#include "jfs_file.h"
#include "jfs_util.h"
#include "jfs_file_cache.h"
#include "sqlitedb.h"
#include "joinfs.h"

#include <fuse.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

static int jfs_file_db_add(int syminode, int datainode, char *datapath, char *filename, int mode);
static char *create_datapath(char *uuid);

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

  syminode = jfs_util_get_inode(path);
  if(syminode < 0) {
	return -1;
  }

  uuid = jfs_create_uuid();
  jfs_generate_uuid(uuid);

  datapath = create_datapath(uuid);
  filename = jfs_util_get_filename(path);

  log_error("Datapath:%s\n", datapath);

  fd = creat(datapath, mode);
  if(fd < 0) {
	free(datapath);
	return -1;
  }

  datainode = jfs_util_get_inode(datapath);
  if(datainode < 0) {
	free(datapath);
	return -1;
  }

  rc = jfs_file_db_add(syminode, datainode, datapath, filename, mode);
  if(rc < 0) {
	printf("New file database inserts failed.\n");
  }

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

  syminode = jfs_util_get_inode(path);
  if(syminode < 0) {
	return -1;
  }

  uuid = jfs_create_uuid();
  jfs_generate_uuid(uuid);

  datapath = create_datapath(uuid);
  jfs_destroy_uuid(uuid);

  filename = jfs_util_get_filename(path);

  log_error("Datapath:%s\n", datapath);

  rc = open(datapath, O_CREAT | O_EXCL | O_WRONLY, mode);
  if(rc < 0) {
	free(datapath);
	return -1;
  }
  rc = close(rc);

  datainode = jfs_util_get_inode(datapath);
  if(datainode < 0) {
	free(datapath);
	return -1;
  }
  
  rc = jfs_file_db_add(syminode, datainode, datapath, filename, mode);
  if(rc < 0) {
	printf("New file database inserts failed.\n");
  }

  return rc;
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
		   "INSERT OR ROLLBACK INTO files VALUES(NULL, %d, %d, \"%s\", \"%s\");",
		   datainode, mode, datapath, filename);
  
  jfs_write_pool_queue(db_op);
  jfs_db_op_wait(db_op);

  if(db_op->error == JFS_QUERY_FAILED) {
	jfs_db_op_destroy(db_op);
	free(datapath);
	return -1;
  }
  jfs_db_op_destroy(db_op);

  /* now add to symlinks */
  db_op = jfs_db_op_create();
  db_op->res_t = jfs_write_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "INSERT OR ROLLBACK INTO symlinks VALUES(%d,%d,(SELECT fileid FROM files WHERE inode=%d));",
		   syminode, datainode, datainode);
  
  jfs_write_pool_queue(db_op);
  jfs_db_op_wait(db_op);

  if(db_op->error == JFS_QUERY_FAILED) {
	jfs_db_op_destroy(db_op);
	free(datapath);
	return -1;
  }
  jfs_db_op_destroy(db_op);

  jfs_file_cache_add(syminode, datainode, datapath);

  return 0;
}

/*
 * Deletes a joinFS static file.
 */
int
jfs_file_unlink(const char *path)
{
  struct jfs_db_op *db_op;
  char *datapath;
  int datainode;
  int syminode;
  int rc;

  printf("---Started file UNLINK.\n");

  syminode = jfs_util_get_inode(path);
  datapath = jfs_util_get_datapath_and_datainode(path, &datainode);
  if(!datapath) {
	printf("Failed to get datapath.\n");
	return -1;
  }

  rc = unlink(path);
  if(rc) {
	printf("Unlink for path:%s failed.\n", path);
	return rc;
  }
  
  if(datainode > 0) {
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
	  jfs_db_op_destroy(db_op);
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
	  jfs_db_op_destroy(db_op);
	  return -1;
	}
	jfs_db_op_destroy(db_op);
  }

  rc = unlink(datapath);
  if(rc) {
	printf("Unlink on path:%s failed.\n", datapath);
  }

  rc = jfs_file_cache_remove(syminode);

  return rc;
}

/*
 * Perform a rename on a joinFS file.
 */
int
jfs_file_rename(const char *from, const char *to)
{
  int syminode;
  char *filename;
  struct jfs_db_op *db_op;

  printf("Called jfs_rename, from:%s to:%s\n", from, to);

  syminode = jfs_util_is_db_symlink(from);
  if(syminode > 0) {
	filename = jfs_util_get_filename(to);
	
	db_op = jfs_db_op_create();
	db_op->res_t = jfs_write_op;
	snprintf(db_op->query, JFS_QUERY_MAX,
			 "UPDATE OR ROLLBACK files SET filename=\"%s\" WHERE inode=(SELECT datainode FROM symlinks WHERE syminode=%d);",
			 filename, syminode);

	printf("Rename query called:%s\n", db_op->query);
	
	jfs_write_pool_queue(db_op);
	jfs_db_op_wait(db_op);

	if(db_op->error == JFS_QUERY_FAILED) {
	  jfs_db_op_destroy(db_op);
	  return -1;
	}

	printf("Database cleanup.\n");
	
	jfs_db_op_destroy(db_op);

	printf("Renaming from:%s , to:%s\n", from, to);
  }
  else {
	printf("Syminode not found for rename of from:%s. Performing regular rename\n", from);
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
  int rc;

  datapath = jfs_util_get_datapath(path);
  if(!datapath) {
	return -1;
  }

  rc = truncate(datapath, size);

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

  if(flags & O_CREAT) {
	printf("Open called with explicit create flag set.\n");
	rc = open(path, O_CREAT | O_EXCL | O_WRONLY);
	if(rc > 0) {
	  close(rc);

	  rc = unlink(path);
	  if(rc) {
		return -1;
	  }

	  return jfs_file_create(path, flags);
	}
	
	if(flags & O_EXCL) {
	  return -1;
	}
  }

  datapath = jfs_util_get_datapath(path);
  if(!datapath) {
	return -1;
  }

  printf("Opening file at datapath, %s\n", datapath);

  return open(datapath, flags);
}

/*
 * Get the system attributes for a file.
 */
int
jfs_file_getattr(const char *path, struct stat *stbuf)
{
  char *datapath;

  datapath = jfs_util_get_datapath(path);
  if(!datapath) {
	return -1;
  }

  return stat(datapath, stbuf);
}

int
jfs_file_utimes(const char *path, const struct timeval tv[2])
{
  char *datapath;
  int rc;
  
  datapath = jfs_util_get_datapath(path);
  if(strcmp(datapath, path)) {
	rc = utimes(path, tv);
	if(rc) {
	  return -1;
	}
  }
  
  return utimes(datapath, tv);
}

/*
 * Get the location where the data will be saved.
 * 
 * joinFS_root_dir/.data/uuid-string
 *
 * The datapath must be freed if not added to the cache.
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
