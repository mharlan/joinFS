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

#include "jfs_key_cache.h"
#include "sglib.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <attr/xattr.h>
#include <pthread.h>

#define JFS_KEY_CACHE_SIZE 1000

typedef struct jfs_key_cache jfs_key_cache_t;
struct jfs_key_cache {
  int              keyid;
  char            *keytext;

  jfs_key_cache_t *next;
};

static jfs_key_cache_t *hashtable[JFS_KEY_CACHE_SIZE];

#define JFS_KEY_CACHE_T_CMP(e1, e2) (strcmp(e1->keytext, e2->keytext))

static unsigned int
jfs_key_cache_t_hash(jfs_key_cache_t *item)
{
  size_t key_len;
  unsigned int hash;

  key_len = strlen(item->keytext);

  hash = (key_len * (key_len / 2) * 10) % JFS_KEY_CACHE_SIZE;

  return hash;  
}

/*
 * SGLIB generator macros for jfs_key_cache_t lists.
 */
SGLIB_DEFINE_LIST_PROTOTYPES(jfs_key_cache_t, JFS_KEY_CACHE_T_CMP, next)
SGLIB_DEFINE_LIST_FUNCTIONS(jfs_key_cache_t, JFS_KEY_CACHE_T_CMP, next)

/*
 * SGLIB generator macros for hashtable function prototypes.
 */
SGLIB_DEFINE_HASHED_CONTAINER_PROTOTYPES(jfs_key_cache_t, JFS_KEY_CACHE_SIZE,
                                         jfs_key_cache_t_hash)
SGLIB_DEFINE_HASHED_CONTAINER_FUNCTIONS(jfs_key_cache_t, JFS_KEY_CACHE_SIZE,
                                        jfs_key_cache_t_hash)

static pthread_rwlock_t cache_lock;

void
jfs_key_cache_init()
{
  pthread_rwlock_init(&cache_lock, NULL);
  sglib_hashed_jfs_key_cache_t_init(hashtable);
}

void
jfs_key_cache_destroy()
{
  struct sglib_hashed_jfs_key_cache_t_iterator it;
  jfs_key_cache_t *item;

  pthread_rwlock_wrlock(&cache_lock);
  for(item = sglib_hashed_jfs_key_cache_t_it_init(&it, hashtable);
      item != NULL; item = sglib_hashed_jfs_key_cache_t_it_next(&it)) {
    free(item->keytext);
    free(item);
  }

  pthread_rwlock_unlock(&cache_lock);
  pthread_rwlock_destroy(&cache_lock);
}

int
jfs_key_cache_get_keyid(const char *keytext)
{
  jfs_key_cache_t  check;
  jfs_key_cache_t *result;

  size_t key_len;

  int keyid;

  key_len = strlen(keytext) + 1;
  check.keytext = malloc(sizeof(*check.keytext) * key_len);
  if(!check.keytext) {
    return -ENOMEM;
  }
  strncpy(check.keytext, keytext, key_len);

  pthread_rwlock_rdlock(&cache_lock);
  result = sglib_hashed_jfs_key_cache_t_find_member(hashtable, &check);
  free(check.keytext);

  if(!result) {
    pthread_rwlock_unlock(&cache_lock);

    return -ENOATTR;
  }
  keyid = result->keyid;
  pthread_rwlock_unlock(&cache_lock);

  return keyid;
}

int
jfs_key_cache_add(int keyid, const char *keytext)
{
  jfs_key_cache_t *item;
  size_t key_len;
  
  item = malloc(sizeof(*item));
  if(!item) {
    return -ENOMEM;
  }
  item->keyid = keyid;

  key_len = strlen(keytext) + 1;
  item->keytext = malloc(sizeof(*item->keytext) * key_len);
  if(!item->keytext) {
    free(item);
    return -ENOMEM;
  }
  strncpy(item->keytext, keytext, key_len);

  pthread_rwlock_wrlock(&cache_lock);
  sglib_hashed_jfs_key_cache_t_add(hashtable, item);
  pthread_rwlock_unlock(&cache_lock);

  return 0;
}

int
jfs_key_cache_remove(const char *keytext)
{
  jfs_key_cache_t check;
  jfs_key_cache_t *elem;
  
  size_t key_len;

  int rc;

  key_len = strlen(keytext) + 1;
  check.keytext = malloc(sizeof(*check.keytext) * key_len);
  if(!check.keytext) {
    return -ENOMEM;
  }
  strncpy(check.keytext, keytext, key_len);

  pthread_rwlock_wrlock(&cache_lock);
  rc = sglib_hashed_jfs_key_cache_t_delete_if_member(hashtable, &check, &elem);
  pthread_rwlock_unlock(&cache_lock);

  free(check.keytext);

  if(rc) {
    free(elem->keytext);
    free(elem);
  }
  
  return 0;
}
