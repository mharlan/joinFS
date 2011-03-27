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

#include "jfs_meta_cache.h"
#include "sglib.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <attr/xattr.h>
#include <pthread.h>

#define JFS_META_CACHE_SIZE 10000

typedef struct jfs_meta_cache jfs_meta_cache_t;
struct jfs_meta_cache {
  int               keyid;
  char             *value;
  char             *path;
  
  jfs_meta_cache_t *next;
};

static jfs_meta_cache_t *hashtable[JFS_META_CACHE_SIZE];

#define JFS_META_CACHE_T_CMP(e1, e2) (!((strcmp(e1->path, e2->path) == 0) && ((e1->keyid - e2->keyid) == 0)))

static unsigned int
jfs_meta_cache_t_hash(jfs_meta_cache_t *item)
{
  unsigned int hash;

  hash = (strlen(item->path) + item->keyid) % JFS_META_CACHE_SIZE;

  return hash;  
}

/*
 * SGLIB generator macros for jfs_meta_cache_t lists.
 */
SGLIB_DEFINE_LIST_PROTOTYPES(jfs_meta_cache_t, JFS_META_CACHE_T_CMP, next)
SGLIB_DEFINE_LIST_FUNCTIONS(jfs_meta_cache_t, JFS_META_CACHE_T_CMP, next)

/*
 * SGLIB generator macros for hashtable function prototypes.
 */
SGLIB_DEFINE_HASHED_CONTAINER_PROTOTYPES(jfs_meta_cache_t, JFS_META_CACHE_SIZE,
                                         jfs_meta_cache_t_hash)
SGLIB_DEFINE_HASHED_CONTAINER_FUNCTIONS(jfs_meta_cache_t, JFS_META_CACHE_SIZE,
                                        jfs_meta_cache_t_hash)

static pthread_rwlock_t cache_lock;

void
jfs_meta_cache_init()
{
  pthread_rwlock_init(&cache_lock, NULL);
  sglib_hashed_jfs_meta_cache_t_init(hashtable);
}

void
jfs_meta_cache_destroy()
{
  struct sglib_hashed_jfs_meta_cache_t_iterator it;
  jfs_meta_cache_t *item;

  pthread_rwlock_wrlock(&cache_lock);
  for(item = sglib_hashed_jfs_meta_cache_t_it_init(&it, hashtable);
      item != NULL; item = sglib_hashed_jfs_meta_cache_t_it_next(&it)) {
    free(item->path);
    free(item->value);
    free(item);
  }

  pthread_rwlock_unlock(&cache_lock);
  pthread_rwlock_destroy(&cache_lock);
}

int
jfs_meta_cache_get_value(const char *path, int keyid, char **value)
{
  jfs_meta_cache_t  check;
  jfs_meta_cache_t *result;

  char *val;

  size_t val_len;
  size_t path_len;

  check.keyid = keyid;

  path_len = strlen(path) + 1;
  check.path = malloc(sizeof(*check.path) * path_len);
  if(!check.path) {
    return -ENOMEM;
  }
  strncpy(check.path, path, path_len);

  pthread_rwlock_rdlock(&cache_lock);
  result = sglib_hashed_jfs_meta_cache_t_find_member(hashtable, &check);
  free(check.path);
  
  if(!result) {
    pthread_rwlock_unlock(&cache_lock);
    
	return -ENOATTR;
  }

  val_len = strlen(result->value) + 1;
  val = malloc(sizeof(*val) * val_len);
  if(!val) {
    free(val);
    pthread_rwlock_unlock(&cache_lock);

    return -ENOMEM;
  }
  strncpy(val, result->value, val_len);
  pthread_rwlock_unlock(&cache_lock);

  *value = val;

  return 0;
}

int
jfs_meta_cache_add(const char *path, int keyid, const char *value)
{
  jfs_meta_cache_t *item;
  
  size_t val_len;
  size_t path_len;

  jfs_meta_cache_remove(path, keyid);

  item = malloc(sizeof(*item));
  if(!item) {
	return -ENOMEM;
  }

  path_len = strlen(path) + 1;
  item->path = malloc(sizeof(*item->path) * path_len);
  if(!item->path) {
    free(item);
    return -ENOMEM;
  }
  strncpy(item->path, path, path_len);

  val_len = strlen(value) + 1;
  item->value = malloc(sizeof(*item->value) * val_len);
  if(!item->value) {
    free(item);
    free(item->path);
    return -ENOMEM;
  }
  strncpy(item->value, value, val_len);
  item->keyid = keyid;

  pthread_rwlock_wrlock(&cache_lock);
  sglib_hashed_jfs_meta_cache_t_add(hashtable, item);
  pthread_rwlock_unlock(&cache_lock);

  return 0;
}

int
jfs_meta_cache_remove(const char *path, int keyid)
{
  jfs_meta_cache_t check;
  jfs_meta_cache_t *elem;

  size_t path_len;

  int rc;
  
  check.keyid = keyid;

  path_len = strlen(path) + 1;
  check.path = malloc(sizeof(*check.path) * path_len);
  if(!check.path) {
    return -ENOMEM;
  }
  strncpy(check.path, path, path_len);

  pthread_rwlock_wrlock(&cache_lock);
  rc = sglib_hashed_jfs_meta_cache_t_delete_if_member(hashtable, &check, &elem);
  pthread_rwlock_unlock(&cache_lock);
  free(check.path);

  if(rc) {
    free(elem->path);
	free(elem->value);
    free(elem);
  }
  
  return 0;
}
