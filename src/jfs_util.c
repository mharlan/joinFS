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
#include "jfs_path_cache.h"
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
jfs_util_is_path_dynamic(const char *path)
{
  char *datapath;
  int inode;
  int rc;

  inode = jfs_util_get_inode(path);
  if(inode < 1) {
	rc = jfs_path_cache_get_datapath(path, &datapath);
    if(rc) {
      return rc;
    }

    return 1;
  }
  
  return 0;
}

int
jfs_util_get_datapath(const char *path, char **datapath)
{
  char *dpath;
  mode_t mode;

  int inode;
  int rc;

  log_error("--jfs_util_get_datapath called\n");

  rc = jfs_util_get_inode_and_mode(path, &inode, &mode);
  if(rc) {
	rc = jfs_path_cache_get_datapath(path, datapath);
	return rc;
  }

  if(S_ISDIR(mode)) {
	*datapath = (char *)path;
	return 0;
  }
  else if(S_ISREG(mode)) {
	log_error("--Checking cache for syminode:%d\n", inode);

	rc = jfs_file_cache_get_datapath(inode, &dpath);
	if(rc) {
	  rc = jfs_util_file_cache_failure(inode, &dpath, NULL);
	  if(rc) {
		return rc;
	  }
	}
	*datapath = dpath;

	log_error("--Found datapath:%s\n", dpath);

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

  int inode;
  int datainode;
  int rc;

  log_error("--jfs_util_get_datainode called\n");

  rc = jfs_util_get_inode_and_mode(path, &inode, &mode);
  if(rc) {
	rc = jfs_path_cache_get_datapath(path, &datapath);
	if(rc) {
	  return rc;
	}	

	return jfs_util_get_inode(datapath);
  }

  if(S_ISDIR(mode)) {
	return inode;
  }
  else if(S_ISREG(mode)) {
	log_error("--Checking cache for syminode:%d\n", inode);
	
	datainode = jfs_file_cache_get_datainode(inode);
	if(datainode < 1) {
	  rc = jfs_util_file_cache_failure(inode, NULL, &datainode);
	  if(rc) {
		return rc;
	  }
	}

	log_error("--Found datainode:%d\n", datainode);
  
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

  log_error("--jfs_util_get_datapath_and_datainode called\n");

  rc = jfs_util_get_inode_and_mode(path, &inode, &mode);
  if(rc) {
	rc = jfs_path_cache_get_datapath(path, &dpath);
	if(rc) {
	  return rc;
	}
	
	*datapath = dpath;
	*datainode = jfs_util_get_inode(dpath);
	
	return 0;
  }

  if(S_ISDIR(mode)) {
	*datainode = inode;
	*datapath = (char *)path;
	return 0;
  }
  else if(S_ISREG(mode)) {
	log_error("--Checking cache for syminode:%d\n", inode);
	
	rc = jfs_file_cache_get_datapath_and_datainode(inode, &dpath, datainode);
	if(rc) {
	  rc = jfs_util_file_cache_failure(inode, &dpath, datainode);
	  if(rc) {
		return rc;
	  }
	}
	*datapath = dpath;
	
	log_error("--Retrieved datapath:%s, datainode:%d\n", dpath, *datainode);
	
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
    log_error("--!!--Key cache hit, returning keyid:%d for key:%s\n", keyid, key);
    return keyid;
  }

  log_error("--!!--Key cache miss for key:%s, attempting insert\n", key);

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

  log_error("--!!--Getting the keyid from the database.\n");

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
jfs_util_file_cache_failure(int syminode, char **datapath, int *datainode)
{
  struct jfs_db_op *db_op;

  char *sympath;
  char *dpath;

  size_t sympath_len;
  size_t dpath_len;

  int inode;
  int rc;

  log_error("--CACHE-MISS-jfs_util_file_cache_failure\n");
  rc = jfs_db_op_create(&db_op);
  if(rc) {
	return rc;
  }

  db_op->op = jfs_file_cache_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "SELECT s.sympath, f.datapath, f.inode FROM files AS f, symlinks AS S WHERE f.inode=s.datainode and s.syminode=%d;",
		   syminode);

  log_error("--EXECUTING QUERY:%s\n", db_op->query);
  
  jfs_read_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
	jfs_db_op_destroy(db_op);
	return rc;
  }

  if(db_op->result == NULL) {
	return -ENOENT;
  }
 
  inode = db_op->result->inode;

  dpath_len = strlen(db_op->result->datapath) + 1;
  dpath = malloc(sizeof(*dpath) * dpath_len);
  if(!dpath) {
	jfs_db_op_destroy(db_op);
	return -ENOMEM;
  }
  strncpy(dpath, db_op->result->datapath, dpath_len);

  sympath_len = strlen(db_op->result->sympath) + 1;
  sympath = malloc(sizeof(*sympath) * sympath_len);
  if(!sympath) {
	jfs_db_op_destroy(db_op);
	return -ENOMEM;
  }
  strncpy(sympath, db_op->result->sympath, sympath_len);

  jfs_db_op_destroy(db_op);

  log_error("--RESULTS-- PATH(%s) INODE(%d)\n", dpath, inode);

  if(datapath != NULL) {
	*datapath = dpath;
  }

  if(datainode != NULL) {
	*datainode = inode;
  }

  return jfs_file_cache_add(syminode, sympath, inode, dpath);
}

int
jfs_util_file_cache_sympath_failure(int datainode, char **spath)
{
  struct jfs_db_op *db_op;

  char *sympath;
  char *dpath;

  size_t sympath_len;
  size_t dpath_len;

  int syminode;
  int rc;

  log_error("--CACHE-MISS-jfs_util_file_cache_failure\n");
  rc = jfs_db_op_create(&db_op);
  if(rc) {
	return rc;
  }

  db_op->op = jfs_file_cache_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
           "SELECT s.sympath, f.datapath, s.syminode FROM files AS f, symlinks AS s WHERE f.inode=s.datainode and f.inode=%d;",
		   datainode);

  log_error("--EXECUTING QUERY:%s\n", db_op->query);
  
  jfs_read_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
	jfs_db_op_destroy(db_op);
	return rc;
  }

  if(db_op->result == NULL) {
	return -ENOENT;
  }
 
  syminode = db_op->result->inode;

  dpath_len = strlen(db_op->result->datapath) + 1;
  dpath = malloc(sizeof(*dpath) * dpath_len);
  if(!dpath) {
	jfs_db_op_destroy(db_op);
	return -ENOMEM;
  }
  strncpy(dpath, db_op->result->datapath, dpath_len);

  sympath_len = strlen(db_op->result->sympath) + 1;
  sympath = malloc(sizeof(*sympath) * sympath_len);
  if(!sympath) {
	jfs_db_op_destroy(db_op);
	return -ENOMEM;
  }
  strncpy(sympath, db_op->result->sympath, sympath_len);

  jfs_db_op_destroy(db_op);

  log_error("--RESULTS-- PATH(%s) INODE(%d)\n", dpath, datainode);

  if(spath != NULL) {
	*spath = sympath;
  }

  return jfs_file_cache_add(syminode, sympath, datainode, dpath);
}
