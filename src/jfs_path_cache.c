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
#include "jfs_path_cache.h"
#include "sglib.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define JFS_PATH_CACHE_SIZE 1000

typedef struct jfs_path_cache jfs_path_cache_t;
struct jfs_path_cache {
  char             *path;
  char             *datapath;

  jfs_path_cache_t *next;
};

static jfs_path_cache_t *hashtable[JFS_PATH_CACHE_SIZE];

#define JFS_PATH_CACHE_T_CMP(e1, e2) (strcmp(e1->path, e2->path))

/*
 * Hash function for symlinks.
 */
static unsigned int 
jfs_path_cache_t_hash(jfs_path_cache_t *item)
{
  size_t path_len;
  int hash;

  path_len = strlen(item->path);

  hash = (path_len * (path_len / 2)) % JFS_PATH_CACHE_SIZE;
  printf("--PATH CACHE--Path:%s, hashed to (%d)\n", item->path, hash);

  return hash;
}

/*
 * SGLIB generator macros for jfs_path_cache_t lists.
 */
SGLIB_DEFINE_LIST_PROTOTYPES(jfs_path_cache_t, JFS_PATH_CACHE_T_CMP, next)
SGLIB_DEFINE_LIST_FUNCTIONS(jfs_path_cache_t, JFS_PATH_CACHE_T_CMP, next)

/*
 * SGLIB generator macros for hashtable function prototypes.
 */
SGLIB_DEFINE_HASHED_CONTAINER_PROTOTYPES(jfs_path_cache_t, JFS_PATH_CACHE_SIZE,
										 jfs_path_cache_t_hash)
SGLIB_DEFINE_HASHED_CONTAINER_FUNCTIONS(jfs_path_cache_t, JFS_PATH_CACHE_SIZE,
										jfs_path_cache_t_hash)

/*
 * Initialize the jfs_path_cache.
 */
void 
jfs_path_cache_init()
{
  printf("JFS_FILE_CACHE_INIT\n");

  sglib_hashed_jfs_path_cache_t_init(hashtable);
}

/*
 * Destroy the jfs_path_cache.
 */
void
jfs_path_cache_destroy()
{
  struct sglib_hashed_jfs_path_cache_t_iterator it;
  jfs_path_cache_t *item;
  
  printf("JFS_PATH_CACHE_CLEANUP\n");

  for(item = sglib_hashed_jfs_path_cache_t_it_init(&it,hashtable); 
	  item != NULL; item = sglib_hashed_jfs_path_cache_t_it_next(&it)) {
	free(item->path);
	free(item->datapath);
	free(item);
  }
}

int
jfs_path_cache_add(char *path, char *datapath)
{
  jfs_path_cache_t *item;
  jfs_path_cache_t *exists;

  size_t path_len;
  size_t datapath_len;

  int rc;

  item = malloc(sizeof(*item));
  if(!item) {
	return -ENOMEM;
  }

  path_len = strlen(path) + 1;
  datapath_len = strlen(datapath) + 1;

  item->path = malloc(sizeof(*item->path) * path_len);
  if(!item->path) {
	free(item);
	return -ENOMEM;
  }

  item->datapath = malloc(sizeof(*item->path) * path_len);
  if(!item->path) {
	free(item->path);
	free(item);
	return -ENOMEM;
  }

  strncpy(item->path, path, path_len);
  strncpy(item->datapath, datapath, datapath_len);

  //remove it if path is already in path cache to update datapath
  rc = sglib_hashed_jfs_path_cache_t_delete_if_member(hashtable, item, &exists);
  if(rc) {
	free(exists->path);
	free(exists->datapath);
	free(exists);
  }

  sglib_hashed_jfs_path_cache_t_add(hashtable, item);

  return 0;
}

int
jfs_path_cache_remove(const char *path)
{
  jfs_path_cache_t check;
  jfs_path_cache_t *elem;
  int rc;

  check.path = (char *)path;

  rc = sglib_hashed_jfs_path_cache_t_delete_if_member(hashtable, &check, &elem);
  if(!rc) {
	return -1;
  }

  free(elem->path);
  free(elem->datapath);
  free(elem);

  return 0;
}

int
jfs_path_cache_get_datapath(const char *path, char **datapath)
{
  jfs_path_cache_t check;
  jfs_path_cache_t *result;

  check.path = (char *)path;

  result = sglib_hashed_jfs_path_cache_t_find_member(hashtable, &check);

  if(!result) {
	return -ENOENT;
  }

  *datapath = result->datapath;

  return 0;
}
