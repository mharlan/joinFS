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

#include "jfs_meta_cache.h"
#include "sglib.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#define JFS_META_CACHE_SIZE 10000

typedef struct jfs_meta_cache jfs_meta_cache_t;
struct jfs_meta_cache {
  int               inode;
  int               keyid;
  char             *value;
  
  jfs_meta_cache_t *next;
};

static jfs_meta_cache_t *hashtable[JFS_META_CACHE_SIZE];

#define JFS_META_CACHE_T_CMP(e1, e2) ((e1->inode - e2->inode) && (e1->keyid - e2->keyid))

static unsigned int
jfs_meta_cache_t_hash(jfs_meta_cache_t *item)
{
  unsigned int hash;

  hash = (item->inode + item->keyid) % JFS_META_CACHE_SIZE;

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

void
jfs_meta_cache_init()
{
  sglib_hashed_jfs_meta_cache_t_init(hashtable);
}

void
jfs_meta_cache_destroy()
{
  struct sglib_hashed_jfs_meta_cache_t_iterator it;
  jfs_meta_cache_t *item;

  for(item = sglib_hashed_jfs_meta_cache_t_it_init(&it, hashtable);
      item != NULL; item = sglib_hashed_jfs_meta_cache_t_it_next(&it)) {
    free(item->value);
    free(item);
  }
}

int
jfs_meta_cache_get_value(int inode, int keyid, char **value)
{
  jfs_meta_cache_t  check;
  jfs_meta_cache_t *result;

  check.inode = inode;
  check.keyid = keyid;
  result = sglib_hashed_jfs_meta_cache_t_find_member(hashtable, &check);

  if(!result) {
	return -1;
  }
  *value = result->value;

  return 0;
}

int
jfs_meta_cache_add(int inode, int keyid, char *value)
{
  jfs_meta_cache_t *item;

  jfs_meta_cache_remove(inode, keyid);

  item = malloc(sizeof(*item));
  if(!item) {
	return -ENOMEM;
  }

  item->inode = inode;
  item->keyid = keyid;
  item->value = value;
  sglib_hashed_jfs_meta_cache_t_add(hashtable, item);

  return 0;
}

int
jfs_meta_cache_remove(int inode, int keyid)
{
  jfs_meta_cache_t check;
  jfs_meta_cache_t *elem;
  int rc;

  check.inode = inode;
  check.keyid = keyid;
  rc = sglib_hashed_jfs_meta_cache_t_delete_if_member(hashtable, &check, &elem);
  if(!rc) {
	return -1;
  }
  
  free(elem->value);
  free(elem);
  
  return 0;
}
