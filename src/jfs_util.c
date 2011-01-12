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

#include "sqlitedb.h"
#include "jfs_file_cache.h"
#include "jfs_util.h"
#include "joinfs.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

static int jfs_util_file_cache_failure(int syminode, char **datapath, int *datainode);

int 
jfs_util_get_inode(const char *path)
{
  struct stat buf;
  int rc;

  rc = stat(path, &buf);
  if(rc < 0) {
	return -errno;
  }

  return buf.st_ino;
}

int
jfs_util_get_inode_and_mode(const char *path, int *inode, mode_t *mode)
{
  struct stat buf;
  int rc;

  rc = stat(path, &buf);
  if(rc < 0) {
	return -errno;
  }

  *inode = buf.st_ino;
  *mode = buf.st_mode;

  return 0;
}

int
jfs_util_is_db_symlink(const char *path)
{
  mode_t mode;
  int inode;
  int rc;

  rc = jfs_util_get_inode_and_mode(path, &inode, &mode);
  if(rc) {
	return rc;
  }

  if(S_ISREG(mode)) {
	return inode;
  }

  return 0;
}

int
jfs_util_get_datapath(const char *path, char *datapath)
{
  int syminode;
  int rc;

  syminode = jfs_util_is_db_symlink(path);
  if(syminode < 1) {
	datapath = (char *)path;
	return 0;
  }

  datapath = jfs_file_cache_get_datapath(syminode);
  if(!datapath) {
	rc = jfs_util_file_cache_failure(syminode, &datapath, NULL);
  }

  return rc;
}

int
jfs_util_get_datainode(const char *path)
{
  int syminode;
  int datainode;
  int rc;

  syminode = jfs_util_is_db_symlink(path);
  if(syminode < 1) {
	return jfs_util_get_inode(path);
  }

  datainode = jfs_file_cache_get_datainode(syminode);
  if(datainode < 1) {
	rc = jfs_util_file_cache_failure(syminode, NULL, &datainode);
  }

  return rc;
}

int
jfs_util_get_datapath_and_datainode(const char *path, char *datapath, int *datainode)
{
  int syminode;
  int rc;

  syminode = jfs_util_is_db_symlink(path);
  if(syminode < 1) {
	datapath = (char *)path;
	*datainode = jfs_util_get_inode(path);
	return 0;
  }

  datapath = jfs_file_cache_get_datapath_and_datainode(syminode, datainode);
  if(!datapath) {
	rc = jfs_util_file_cache_failure(syminode, &datapath, datainode);
  }
  
  return rc;
}

char *
jfs_util_get_filename(const char *path)
{
  char *filename;
  
  filename = strrchr(path, '/') + 1;

  return filename;
}

/*
 * Returns they keyid associated with a key in the system.
 *
 * Returns -1 if the key is not found and can not be added.
 * This code sucks right now.
 */
int
jfs_util_get_keyid(const char *key)
{
  struct jfs_db_op *db_op;
  int keyid;
  int rc;

  //insert or fail if it exists
  db_op = jfs_db_op_create();
  db_op->op = jfs_write_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "INSERT OR IGNORE INTO keys VALUES(NULL, \"%s\");",
		   key);
  
  jfs_write_pool_queue(db_op);
  jfs_db_op_wait(db_op);

  rc = db_op->rc;
  if(rc) {
	jfs_db_op_destroy(db_op);
	return rc;
  }
  jfs_db_op_destroy(db_op);
  
  //return the keyid
  db_op = jfs_db_op_create();
  db_op->op = jfs_key_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "SELECT keyid FROM keys WHERE keytext=\"%s\";",
		   key);
  
  jfs_read_pool_queue(db_op);
  jfs_db_op_wait(db_op);

  rc = db_op->rc;
  if(rc) {
	jfs_db_op_destroy(db_op);
	return rc;
  }

  keyid = db_op->result->keyid;
  jfs_db_op_destroy(db_op);

  return keyid;
}

static int
jfs_util_file_cache_failure(int syminode, char **datapath, int *datainode)
{
  struct jfs_db_op *db_op;
  char *path;
  size_t path_len;
  int inode;
  int rc;

  db_op = jfs_db_op_create();
  db_op->op = jfs_file_cache_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "SELECT datapath, inode FROM files WHERE inode=(SELECT datainode FROM symlinks WHERE syminode=%d);",
		   syminode);
  
  jfs_read_pool_queue(db_op);
  jfs_db_op_wait(db_op);
  
  rc = db_op->rc;
  if(rc) {
	jfs_db_op_destroy(db_op);
	return rc;
  }
 
  inode = db_op->result->inode;
  path_len = strlen(db_op->result->datapath) + 1;
  path = malloc(sizeof(*datapath) * path_len);
  if(!path) {
	jfs_db_op_destroy(db_op);
	return -ENOMEM;
  }
  
  strncpy(path, db_op->result->datapath, path_len);
  jfs_db_op_destroy(db_op);

  if(datapath) {
	*datapath = path;
  }

  if(datainode) {
	*datainode = inode;
  }

  return jfs_file_cache_add(syminode, inode, datapath);
}
