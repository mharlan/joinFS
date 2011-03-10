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
#include "jfs_datapath_cache.h"
#include "sglib.h"
#include "sqlitedb.h"
#include "joinfs.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define JFS_DATAPATH_CACHE_SIZE 1000

typedef struct jfs_datapath_cache jfs_datapath_cache_t;
struct jfs_datapath_cache {
  int                   inode;
  char                 *datapath;

  jfs_datapath_cache_t *next;
};

static jfs_datapath_cache_t *hashtable[JFS_DATAPATH_CACHE_SIZE];

#define JFS_DATAPATH_CACHE_T_CMP(e1, e2) (e1->inode - e2->inode)

/*
 * Hash function for symlinks.
 */
static unsigned int 
jfs_datapath_cache_t_hash(jfs_datapath_cache_t *item)
{
  unsigned int hash;

  hash = item->inode % JFS_DATAPATH_CACHE_SIZE;

  return hash;
}

/*
 * SGLIB generator macros for jfs_path_cache_t lists.
 */
SGLIB_DEFINE_LIST_PROTOTYPES(jfs_datapath_cache_t, JFS_DATAPATH_CACHE_T_CMP, next)
SGLIB_DEFINE_LIST_FUNCTIONS(jfs_datapath_cache_t, JFS_DATAPATH_CACHE_T_CMP, next)

/*
 * SGLIB generator macros for hashtable function prototypes.
 */
SGLIB_DEFINE_HASHED_CONTAINER_PROTOTYPES(jfs_datapath_cache_t, JFS_DATAPATH_CACHE_SIZE,
										 jfs_datapath_cache_t_hash)
SGLIB_DEFINE_HASHED_CONTAINER_FUNCTIONS(jfs_datapath_cache_t, JFS_DATAPATH_CACHE_SIZE,
										jfs_datapath_cache_t_hash)

static int jfs_datapath_cache_miss(int inode, char **datapath);

/*
 * Initialize the jfs_datapath_cache.
 */
void 
jfs_datapath_cache_init()
{
  log_error("JFS_DATPATH_CACHE_INIT\n");

  sglib_hashed_jfs_datapath_cache_t_init(hashtable);
}

/*
 * Destroy the jfs_datapath_cache.
 */
void
jfs_datapath_cache_destroy()
{
  struct sglib_hashed_jfs_datapath_cache_t_iterator it;
  jfs_datapath_cache_t *item;
  
  log_error("JFS_DATAPATH_CACHE_CLEANUP\n");

  for(item = sglib_hashed_jfs_datapath_cache_t_it_init(&it,hashtable); 
	  item != NULL; item = sglib_hashed_jfs_datapath_cache_t_it_next(&it)) {
	free(item->datapath);
	free(item);
  }
}

int
jfs_datapath_cache_add(int inode, const char *datapath)
{
  jfs_datapath_cache_t *item;

  char *path;

  size_t path_len;

  jfs_datapath_cache_remove(inode);

  item = malloc(sizeof(*item));
  if(!item) {
	return -ENOMEM;
  }

  path_len = strlen(datapath) + 1;
  path = malloc(sizeof(*path) * path_len);
  if(!path) {
    free(item);
    return -ENOMEM;
  }
  strncpy(path, datapath, path_len);

  item->inode = inode;
  item->datapath = path;

  sglib_hashed_jfs_datapath_cache_t_add(hashtable, item);

  return 0;
}

int
jfs_datapath_cache_remove(int inode)
{
  jfs_datapath_cache_t check;
  jfs_datapath_cache_t *elem;

  int rc;

  check.inode = inode;

  rc = sglib_hashed_jfs_datapath_cache_t_delete_if_member(hashtable, &check, &elem);
  
  if(rc) {
    free(elem->datapath);
    free(elem);
  }

  return 0;
}

int
jfs_datapath_cache_get_datapath(int inode, char **datapath)
{
  jfs_datapath_cache_t check;
  jfs_datapath_cache_t *result;

  char *path;

  size_t path_len;

  check.inode = inode;
  result = sglib_hashed_jfs_datapath_cache_t_find_member(hashtable, &check);

  if(!result) {
	return jfs_datapath_cache_miss(inode, datapath);
  }

  path_len = strlen(result->datapath) + 1;
  path = malloc(sizeof(*path) * path_len);
  if(!path) {
    return -ENOMEM;
  }
  strncpy(path, result->datapath, path_len);

  *datapath = path;

  return 0;
}

int
jfs_datapath_cache_change_inode(int inode, int new_inode)
{
  jfs_datapath_cache_t check;
  jfs_datapath_cache_t *result;

  check.inode = inode;
  result = sglib_hashed_jfs_datapath_cache_t_find_member(hashtable, &check);

  if(!result) {
	return -ENOENT;
  }
  
  result->inode = new_inode;

  return 0;
}

/*
  Handles the cache miss.
 */
static int
jfs_datapath_cache_miss(int inode, char **datapath)
{
  struct jfs_db_op *db_op;
  
  char *path;
  size_t path_len;

  int rc;

  rc = jfs_db_op_create(&db_op);
  if(rc) {
    return rc;
  }

  db_op->op = jfs_datapath_cache_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
           "SELECT datapath FROM files WHERE inode=%d;", inode);

  jfs_read_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
    jfs_db_op_destroy(db_op);
    return rc;
  }

  if(db_op->result == NULL) {
    return -ENOENT;
  }

  path_len = strlen(db_op->result->datapath) + 1;
  path = malloc(sizeof(*path) * path_len);
  if(!path) {
    return -ENOMEM;
  }
  strncpy(path, db_op->result->datapath, path_len);

  jfs_db_op_destroy(db_op);

  rc = jfs_datapath_cache_add(inode, path);
  if(rc) {
    return rc;
  }

  if(datapath) {
    *datapath = path;
  }
  else {
    free(path);
  }

  return 0;
}
