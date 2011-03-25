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

#if !defined(_REENTRANT)
#define	_REENTRANT
#endif

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

  datapath = NULL;
  if(jfs_util_is_realpath(path)) {
    return 0;
  }

  rc = jfs_dynamic_path_resolution(path, &datapath, &inode);
  
  if(rc) {
    return 0;
  }
  free(datapath);

  return 1;
}

int
jfs_util_get_datapath(const char *path, char **datapath)
{
  int rc;

  rc = jfs_util_get_datapath_and_datainode(path, datapath, NULL);
  if(rc) {
    return rc;
  }

  return 0;
}

int
jfs_util_get_datainode(const char *path)
{
  int datainode;
  int rc;

  rc = jfs_util_get_datapath_and_datainode(path, NULL, &datainode);
  if(rc) {
    return rc;
  }

  return datainode;
}

int
jfs_util_get_datapath_and_datainode(const char *path, char **datapath, int *datainode)
{
  char *filename;
  char *realpath;
  char *subpath;
  char *d_path;

  mode_t mode;

  size_t realpath_len;

  int d_inode;
  int inode;
  int rc;

  if(jfs_util_is_realpath(path)) {
    rc = jfs_util_get_inode_and_mode(path, &inode, &mode);
    if(rc) {
      return rc;
    }
  }
  else {
	rc = jfs_dynamic_path_resolution(path, &d_path, &d_inode);

    //check if it is stored in a .jfs_sub_query folder
	if(rc) {
      rc = jfs_util_get_subpath(path, &subpath);
      if(rc) {
        return rc;
      }
      
      rc = jfs_dynamic_path_resolution(subpath, &d_path, &d_inode);
      free(subpath);

      if(rc) {
        return rc;
      }

      filename = jfs_util_get_filename(path);
      realpath_len = strlen(d_path) + strlen(filename) + 2; //null and '/'
      realpath = malloc(sizeof(*realpath) * realpath_len);
      if(!realpath) {
        return -ENOMEM;
      }
      snprintf(realpath, realpath_len, "%s/%s", d_path, filename);
      d_path = realpath;

      //does it actually exist?
      d_inode = jfs_util_get_inode(d_path);
      if(d_inode < 0) {
        return d_inode;
      }

      //cache it for next time
      rc = jfs_dynamic_hierarchy_add_file(path, d_path, d_inode);
      if(rc) {
        return rc;
      }
    }
    
    if(datainode) {
      *datainode = d_inode;
    }
    
    if(datapath) {
      *datapath = d_path;
    }
    else {
      free(d_path);
    }

    return 0;
  }

  if(S_ISDIR(mode)) {
    if(datainode) {
      *datainode = inode;
    }

    if(datapath) {
      realpath_len = strlen(path) + 1;
      realpath = malloc(sizeof(*realpath) * realpath_len);
      if(!realpath) {
        return -ENOMEM;
      }
      strncpy(realpath, path, realpath_len);

      *datapath = realpath;
    }

	return 0;
  }
  else if(S_ISREG(mode)) {	
	rc = jfs_file_cache_get_datapath_and_datainode(inode, &d_path, &d_inode);
    
	if(rc) {
      return rc;
	}

    if(datainode) {
      *datainode = d_inode;
    }

    if(datapath) {
      *datapath = d_path;
    }
    else {
      free(d_path);
    }
    
	return 0;
  }
  
  return -ENOENT;
}

char *
jfs_util_get_filename(const char *path)
{
  char *filename;
  
  filename = strrchr(path, '/') + 1;

  return filename;
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
  rc = jfs_db_op_create(&db_op, jfs_write_op,
                        "INSERT OR IGNORE INTO keys VALUES(NULL, \"%s\");",
                        key);
  if(rc) {
	return rc;
  }
  
  jfs_write_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
	jfs_db_op_destroy(db_op);
	return rc;
  }
  jfs_db_op_destroy(db_op);

  //go out to the database for the keyid
  rc = jfs_db_op_create(&db_op, jfs_key_cache_op,
                        "SELECT keyid FROM keys WHERE keytext=\"%s\";",
                        key);
  if(rc) {
	return rc;
  }
  
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
    return -ENOENT;
  }
  *terminator = '\0';

  return 0;
}

char *
jfs_util_get_last_path_item(const char *path)
{
  return strrchr(path, '/');
}

int
jfs_util_get_subpath(const char *path, char **new_path)
{
  char *subpath;
  char *filename;
  size_t sub_len;

  filename = jfs_util_get_filename(path);
  if(!filename || *filename == '\0') {
    return -ENOENT;
  }

  sub_len = strlen(path) - strlen(filename) + 1;
  subpath = malloc(sizeof(*subpath) * sub_len);
  if(!subpath) {
    return -ENOMEM;
  }
  strncpy(subpath, path, sub_len);
  subpath[sub_len - 1] = '\0';

  *new_path = subpath;

  return 0;
}

int
jfs_util_resolve_new_path(const char *path, char **new_path)
{
  char *sub_datapath;
  char *subpath;
  char *realpath;
  char *filename;

  size_t realpath_len;
  
  int rc;
  
  subpath = NULL;
  sub_datapath = NULL;
  realpath = NULL;
  
  filename = jfs_util_get_filename(path);
  if(!filename || *filename == '\0') {
    return -ENOENT;
  }
  
  rc = jfs_util_get_subpath(path, &subpath); 
  if(rc){
    return rc;
  }
  
  //if the subpath isn't real, get the sub_datapath
  if(!jfs_util_is_realpath(subpath)) {
    rc = jfs_util_get_datapath(subpath, &sub_datapath);
    free(subpath);

    if(rc) {
      return rc;
    }
    subpath = sub_datapath;
  }

  realpath_len = strlen(subpath) + strlen(filename) + 1;
  realpath = malloc(sizeof(*realpath) * realpath_len);
  if(!realpath) {
    free(subpath);

    return -ENOMEM;
  }
  snprintf(realpath, realpath_len, "%s%s", subpath, filename);
  free(subpath);
  
  *new_path = realpath;

  return 0;
}
