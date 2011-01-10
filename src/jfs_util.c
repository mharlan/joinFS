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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

static int jfs_util_file_cache_failure(int syminode);

int 
jfs_util_get_inode(const char *path)
{
  struct stat buf;
  int rc;

  rc = stat(path, &buf);
  if(rc < 0) {
	return rc;
  }

  return buf.st_ino;
}

char *
jfs_util_get_datapath(const char *path)
{
  char *datapath;
  int syminode;
  int rc;
												
  syminode = jfs_util_get_inode(path);
  if(syminode < 1) {
	return 0;
  }

  printf("Checking cache for syminode:%d\n", syminode);
  datapath = jfs_file_cache_get_datapath(syminode);
  printf("Cache returned datapath:%s\n", datapath);
  if(!datapath) {
	rc = jfs_util_file_cache_failure(syminode);
	if(rc) {
	  printf("File cache recovery failed.\n");
	  return 0;
	}

	datapath = jfs_file_cache_get_datapath(syminode);
  }

  printf("Returning datapath:%s\n", datapath);

  return datapath;
}

char *
jfs_util_get_filename(const char *path)
{
  char *filename;

  filename = strrchr(path, '/');

  return ++filename;
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

  //insert or fail if it exists
  db_op = jfs_db_op_create();
  db_op->res_t = jfs_write_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "INSERT OR IGNORE INTO keys VALUES(NULL, \"%s\");",
		   key);
  
  jfs_write_pool_queue(db_op);
  jfs_db_op_wait(db_op);

  if(db_op->error == JFS_QUERY_FAILED) {
	printf("Insert key query failed.\n");
	jfs_db_op_destroy(db_op);
	return -1;
  }
  jfs_db_op_destroy(db_op);
  
  //return the keyid
  db_op = jfs_db_op_create();
  db_op->res_t = jfs_key_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "SELECT keyid FROM keys WHERE keytext=\"%s\";",
		   key);
  
  jfs_read_pool_queue(db_op);
  jfs_db_op_wait(db_op);

  if(db_op->error == JFS_QUERY_FAILED) {
	jfs_db_op_destroy(db_op);
	return -1;
  }

  keyid = db_op->result->keyid;
  jfs_db_op_destroy(db_op);

  return keyid;
}

int
jfs_util_get_datainode(const char *path)
{
  int syminode;
  int datainode;
  int rc;

  syminode = jfs_util_get_inode(path);
  if(syminode < 1) {
	return syminode;
  } 

  datainode = jfs_file_cache_get_datainode(syminode);
  if(datainode < 1) {
	rc = jfs_util_file_cache_failure(syminode);
	if(rc) {
	  return -1;
	}
	
	datainode = jfs_file_cache_get_datainode(syminode);
  }

  return datainode;
}

static int
jfs_util_file_cache_failure(int syminode)
{
  struct jfs_db_op *db_op;
  char *datapath;
  int datainode;

  db_op = jfs_db_op_create();
  db_op->res_t = jfs_file_cache_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "SELECT datapath, inode FROM files WHERE inode=(SELECT datainode FROM symlinks WHERE syminode=%d);",
		   syminode);
  
  jfs_read_pool_queue(db_op);
  jfs_db_op_wait(db_op);
  
  if(db_op->error == JFS_QUERY_FAILED) {
	jfs_db_op_destroy(db_op);
	return -1;
  }
  
  if(!db_op->result->datapath) {
	jfs_db_op_destroy(db_op);
	return -1;
  }
  
  datainode = db_op->result->inode;
  datapath = malloc(sizeof(*datapath) * strlen(db_op->result->datapath) + 1);
  if(datapath) {
	strcpy(datapath, db_op->result->datapath);
  }
  else {
	jfs_db_op_destroy(db_op);
	return -1;
  }
  jfs_db_op_destroy(db_op);

  printf("Adding syminode:%d, datainode:%d, datapath:%s to file cache.\n",
		 syminode, datainode, datapath);

  return jfs_file_cache_add(syminode, datainode, datapath);
}
