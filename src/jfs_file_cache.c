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
#include "jfs_file_cache.h"
#include "sglib.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define JFS_FILE_CACHE_SIZE 1000

typedef struct jfs_file_cache jfs_file_cache_t;
struct jfs_file_cache {
  int               syminode;
  int               datainode;
  char             *datapath;
  jfs_file_cache_t *next;
};

static jfs_file_cache_t *hashtable[JFS_FILE_CACHE_SIZE];

#define JFS_FILE_CACHE_T_CMP(e1, e2) (e1->syminode - e2->syminode)

/*
 * Hash function for symlinks.
 */
static unsigned int 
jfs_file_cache_t_hash(jfs_file_cache_t *item)
{
  int hash;

  hash = item->syminode % JFS_FILE_CACHE_SIZE;
  printf("Symionde:%d, hashed to (%d)\n", item->syminode, hash);

  return hash;
}

/*
 * SGLIB generator macros for jfs_file_cache_t lists.
 */
SGLIB_DEFINE_LIST_PROTOTYPES(jfs_file_cache_t, JFS_FILE_CACHE_T_CMP, next)
SGLIB_DEFINE_LIST_FUNCTIONS(jfs_file_cache_t, JFS_FILE_CACHE_T_CMP, next)

/*
 * SGLIB generator macros for hashtable function prototypes.
 */
SGLIB_DEFINE_HASHED_CONTAINER_PROTOTYPES(jfs_file_cache_t, JFS_FILE_CACHE_SIZE,
										 jfs_file_cache_t_hash)
SGLIB_DEFINE_HASHED_CONTAINER_FUNCTIONS(jfs_file_cache_t, JFS_FILE_CACHE_SIZE,
										jfs_file_cache_t_hash)

/*
 * Initialize the jfs_file_cache.
 */
void 
jfs_file_cache_init()
{
  printf("JFS_FILE_CACHE_INIT\n");

  sglib_hashed_jfs_file_cache_t_init(hashtable);
}

/*
 * Destroy the jfs_file_cache.
 */
void
jfs_file_cache_destroy()
{
  struct sglib_hashed_jfs_file_cache_t_iterator it;
  jfs_file_cache_t *item;
  
  printf("JFS_FILE_CACHE_CLEANUP\n");

  for(item = sglib_hashed_jfs_file_cache_t_it_init(&it,hashtable); 
	  item != NULL; item = sglib_hashed_jfs_file_cache_t_it_next(&it)) {
	free(item->datapath);
	free(item);
  }
}


/*
 * Get a datainode from the file cache.
 *
 * Returns 0 if not in the cache.
 */
int
jfs_file_cache_get_datainode(int syminode)
{
  jfs_file_cache_t  check;
  jfs_file_cache_t *result;

  check.syminode = syminode;
  result = sglib_hashed_jfs_file_cache_t_find_member(hashtable, &check);

  if(!result) {
	return -1;
  }

  return result->datainode;
}

/*
 * Get a datainode from the file cache.
 *
 * Returns 0 if not in the cache.
 */
int
jfs_file_cache_get_datapath(int syminode, char **datapath)
{
  jfs_file_cache_t check;
  jfs_file_cache_t *result;

  check.syminode = syminode;
  result = sglib_hashed_jfs_file_cache_t_find_member(hashtable, &check);

  if(!result) {
	return -1;
  }

  *datapath = result->datapath;

  return 0;
}

int
jfs_file_cache_get_datapath_and_datainode(int syminode, char **datapath, int *datainode)
{
  jfs_file_cache_t check;
  jfs_file_cache_t *result;

  check.syminode = syminode;

  result = sglib_hashed_jfs_file_cache_t_find_member(hashtable, &check);
  if(!result) {
	return -1;
  }

  *datainode = result->datainode;
  *datapath = result->datapath;

  return 0;
}

/*
 * Add a symlink to the jfs_file_cache.
 */
int
jfs_file_cache_add(int syminode, int datainode, const char *datapath)
{
  jfs_file_cache_t *item;

  item = malloc(sizeof(*item));
  if(!item) {
	printf("Failed to allocate memory for a file cache symlink.");
	return -ENOMEM;
  }

  item->syminode = syminode;
  item->datainode = datainode;
  item->datapath = (char *)datapath;

  printf("--CACHE ADD-- adding item syminode:%d, datainode:%d, datapath:%s\n",
		 item->syminode, item->datainode, item->datapath);
  sglib_hashed_jfs_file_cache_t_add(hashtable, item);

  return 0;
}

int
jfs_file_cache_remove(int syminode)
{
  jfs_file_cache_t check;
  jfs_file_cache_t *elem;
  int rc;

  check.syminode = syminode;

  rc = sglib_hashed_jfs_file_cache_t_delete_if_member(hashtable, &check, &elem);
  if(!rc) {
	return -1;
  }
  
  free(elem->datapath);
  free(elem);

  return 0;
}
