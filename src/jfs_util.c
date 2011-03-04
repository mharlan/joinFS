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
#include "sqlitedb.h"
#include "jfs_file_cache.h"
#include "jfs_dynamic_paths.h"
#include "jfs_key_cache.h"
#include "jfs_util.h"
#include "joinfs.h"

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

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
jfs_util_is_realpath(const char *path)
{
  int mask;

  mask = F_OK;

  if(!access(path, mask)) {
    return 1;
  }

  return 0;
}

int
jfs_util_is_path_dynamic(const char *path)
{
  char *datapath;
  int inode;
  int rc;

  if(jfs_util_is_realpath(path)) {
    return 0;
  }

  rc = jfs_dynamic_path_resolution(path, &datapath, &inode);
  if(rc) {
      return rc;
  }

  return 1;
}

int
jfs_util_get_datapath(const char *path, char **datapath)
{
  char *dpath;
  mode_t mode;

  int inode;
  int rc;

  if(!jfs_util_is_realpath(path)) {
    rc = jfs_util_get_inode_and_mode(path, &inode, &mode);
    if(rc) {
      return rc;
    }
  }
  else {
    return jfs_dynamic_path_resolution(path, datapath, &inode);
  }

  if(S_ISDIR(mode)) {
	*datapath = (char *)path;
	return 0;
  }
  else if(S_ISREG(mode)) {
	rc = jfs_file_cache_get_datapath(inode, &dpath);
	if(rc) {
      return rc;
	}
	*datapath = dpath;

	return 0;
  }
  else {
	return -1;
  }
}

int
jfs_util_get_datainode(const char *path)
{
  char *datapath;
  mode_t mode;

  int datainode;
  int inode;
  int rc;

  if(jfs_util_is_realpath(path)) {
    rc = jfs_util_get_inode_and_mode(path, &inode, &mode);
    if(rc) {
      return rc;
    }
  }
  else {
	rc = jfs_dynamic_path_resolution(path, &datapath, &datainode);
	if(rc) {
	  return rc;
	}	

	return inode;
  }

  if(S_ISDIR(mode)) {
	return inode;
  }
  else if(S_ISREG(mode)) {
	datainode = jfs_file_cache_get_datainode(inode);
	if(datainode < 1) {
      return rc;
	}
  
	return datainode;
  }
  else {
	return -1;
  }
}

int
jfs_util_get_datapath_and_datainode(const char *path, char **datapath, int *datainode)
{
  char *dpath;
  mode_t mode;

  int inode;
  int rc;

  if(jfs_util_is_realpath(path)) {
    rc = jfs_util_get_inode_and_mode(path, &inode, &mode);
    if(rc) {
      return rc;
    }
  }
  else {
	return jfs_dynamic_path_resolution(path, datapath, datainode);
  }

  if(S_ISDIR(mode)) {
	*datainode = inode;
	*datapath = (char *)path;

	return 0;
  }
  else if(S_ISREG(mode)) {	
	rc = jfs_file_cache_get_datapath_and_datainode(inode, &dpath, datainode);
	if(rc) {
      return rc;
	}
	*datapath = dpath;
    
	return 0;
  }
  else {
	return -1;
  }
}

char *
jfs_util_get_filename(const char *path)
{
  char *filename;
  
  filename = strrchr(path, '/') + 1;

  return filename;
}

int
jfs_util_get_subpath(const char *path, char **sub_path)
{
  char *subpath;
  char *filename;
  size_t sub_len;

  filename = strrchr(path, '/') + 1;
  sub_len = strlen(path) - strlen(filename);

  subpath = malloc(sizeof(*subpath) * sub_len);
  if(!subpath) {
    return -ENOMEM;
  }
  strncpy(subpath, path, sub_len);

  *sub_path = subpath;

  return 0;
}

int
jfs_util_change_filename(const char *path, const char *filename, char **newpath)
{
  char *subpath;
  char *npath;
  size_t npath_len;
  int rc;

  rc = jfs_util_get_subpath(path, &subpath);
  if(rc) {
    return rc;
  }

  npath_len = strlen(subpath) + strlen(filename) + 1;
  npath = malloc(sizeof(*npath) * npath_len);
  if(!npath) {
    free(subpath);
    return -ENOMEM;
  }

  snprintf(npath, npath_len, "%s%s", subpath, filename);
  free(subpath);
  *newpath = npath;

  return 0;
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

  //hit the cache first
  keyid = jfs_key_cache_get_keyid(key);
  if(keyid > 0) {
    return keyid;
  }

  //cache miss, insert, but ignore if it exists
  rc = jfs_db_op_create(&db_op);
  if(rc) {
	return rc;
  }

  db_op->op = jfs_write_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "INSERT OR IGNORE INTO keys VALUES(NULL, \"%s\");",
		   key);
  
  jfs_write_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
	jfs_db_op_destroy(db_op);
	return rc;
  }
  jfs_db_op_destroy(db_op);

  //go out to the database for the keyid
  rc = jfs_db_op_create(&db_op);
  if(rc) {
	return rc;
  }

  db_op->op = jfs_key_cache_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "SELECT keyid FROM keys WHERE keytext=\"%s\";",
		   key);
  
  jfs_read_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
	jfs_db_op_destroy(db_op);
	return rc;
  }

  //this should never happen
  if(db_op->result == NULL) {
	return -EINVAL;
  }

  keyid = db_op->result->keyid;
  jfs_db_op_destroy(db_op);

  jfs_key_cache_add(keyid, key);

  return keyid;
}

int
jfs_util_strip_last_path_item(char *path)
{
  char *terminator;

  terminator = strrchr(path, '/');
  if(!terminator) {
    return 1;
  }
  *terminator = '\0';

  return 0;
}

char *
jfs_util_get_last_path_item(const char *path)
{
  return strrchr(path, '\0');
}
