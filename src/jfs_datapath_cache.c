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
#include <pthread.h>

#define JFS_DATAPATH_CACHE_SIZE 1000

typedef struct jfs_datapath_cache jfs_datapath_cache_t;
struct jfs_datapath_cache {
  int                   jfs_id;
  char                 *datapath;

  jfs_datapath_cache_t *next;
};

static jfs_datapath_cache_t *hashtable[JFS_DATAPATH_CACHE_SIZE];

#define JFS_DATAPATH_CACHE_T_CMP(e1, e2) (e1->jfs_id - e2->jfs_id)

/*
 * Hash function for symlinks.
 */
static unsigned int 
jfs_datapath_cache_t_hash(jfs_datapath_cache_t *item)
{
  unsigned int hash;

  hash = item->jfs_id % JFS_DATAPATH_CACHE_SIZE;

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

static pthread_rwlock_t cache_lock;

/*
 * Initialize the jfs_datapath_cache.
 */
void 
jfs_datapath_cache_init()
{
  pthread_rwlock_init(&cache_lock, NULL);
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
  
  pthread_rwlock_wrlock(&cache_lock);
  for(item = sglib_hashed_jfs_datapath_cache_t_it_init(&it,hashtable); 
	  item != NULL; item = sglib_hashed_jfs_datapath_cache_t_it_next(&it)) {
	free(item->datapath);
	free(item);
  }

  pthread_rwlock_unlock(&cache_lock);
  pthread_rwlock_destroy(&cache_lock);
}

/*
 * Destroy the jfs_datapath_cache.
 */
void
jfs_datapath_cache_log()
{
  struct sglib_hashed_jfs_datapath_cache_t_iterator it;
  jfs_datapath_cache_t *item;

  log_msg("DATAPATH CACHE LOG START\n");
  pthread_rwlock_rdlock(&cache_lock);
  for(item = sglib_hashed_jfs_datapath_cache_t_it_init(&it,hashtable); 
	  item != NULL; item = sglib_hashed_jfs_datapath_cache_t_it_next(&it)) {
	log_msg("jfs_id:%d, datapath:%s\n", item->jfs_id, item->datapath);
  }

  pthread_rwlock_unlock(&cache_lock);
  pthread_rwlock_destroy(&cache_lock);

  log_msg("LOG END\n");
}

int
jfs_datapath_cache_add(int jfs_id, const char *datapath)
{
  jfs_datapath_cache_t *item;

  char *path;

  size_t path_len;

  jfs_datapath_cache_remove(jfs_id);

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

  item->jfs_id = jfs_id;
  item->datapath = path;

  pthread_rwlock_wrlock(&cache_lock);
  sglib_hashed_jfs_datapath_cache_t_add(hashtable, item);
  pthread_rwlock_unlock(&cache_lock);

  return 0;
}

int
jfs_datapath_cache_remove(int jfs_id)
{
  jfs_datapath_cache_t check;
  jfs_datapath_cache_t *elem;

  int rc;

  check.jfs_id = jfs_id;
  pthread_rwlock_wrlock(&cache_lock);
  rc = sglib_hashed_jfs_datapath_cache_t_delete_if_member(hashtable, &check, &elem);
  pthread_rwlock_unlock(&cache_lock);
  
  if(rc) {
    free(elem->datapath);
    free(elem);
  }
  
  return 0;
}

int
jfs_datapath_cache_get_datapath(int jfs_id, char **datapath)
{
  jfs_datapath_cache_t check;
  jfs_datapath_cache_t *result;

  char *path;

  size_t path_len;

  check.jfs_id = jfs_id;
  pthread_rwlock_rdlock(&cache_lock);
  result = sglib_hashed_jfs_datapath_cache_t_find_member(hashtable, &check);

  if(!result) {
    pthread_rwlock_unlock(&cache_lock);
	return jfs_datapath_cache_miss(jfs_id, datapath);
  }

  path_len = strlen(result->datapath) + 1;
  path = malloc(sizeof(*path) * path_len);
  if(!path) {
    return -ENOMEM;
  }
  strncpy(path, result->datapath, path_len);
  pthread_rwlock_unlock(&cache_lock);

  *datapath = path;

  return 0;
}

/*
  Handles the cache miss.
 */
static int
jfs_datapath_cache_miss(int jfs_id, char **datapath)
{
  struct jfs_db_op *db_op;
  
  char *path;

  size_t path_len;

  int rc;

  rc = jfs_db_op_create(&db_op, jfs_datapath_cache_op, 
                        "SELECT path FROM links WHERE jfs_id=%d;", 
                        jfs_id);
  if(rc) {
    return rc;
  }
  
  jfs_read_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
    jfs_db_op_destroy(db_op);
    return rc;
  }

  if(db_op->result == NULL) {
    db_op->rc = 1;
    jfs_db_op_destroy(db_op);
    return -ENOENT;
  }

  path_len = strlen(db_op->result->datapath) + 1;
  path = malloc(sizeof(*path) * path_len);
  if(!path) {
    jfs_db_op_destroy(db_op);
    return -ENOMEM;
  }
  strncpy(path, db_op->result->datapath, path_len);
  jfs_db_op_destroy(db_op);

  rc = jfs_datapath_cache_add(jfs_id, path);
  if(rc) {
    free(path);

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
