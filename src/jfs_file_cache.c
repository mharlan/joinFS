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
#include "jfs_file_cache.h"
#include "sqlitedb.h"
#include "sglib.h"
#include "joinfs.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define JFS_FILE_CACHE_MAX  2000
#define JFS_FILE_CACHE_SIZE 1000

typedef struct jfs_file_cache jfs_file_cache_t;
struct jfs_file_cache {
  int               syminode;
  int               datainode;
  char             *sympath;

  jfs_file_cache_t *next;
};

static jfs_file_cache_t *hashtable[JFS_FILE_CACHE_SIZE];

#define JFS_FILE_CACHE_T_CMP(e1, e2) (!(((e1->syminode - e2->syminode) == 0) || ((e1->datainode - e2->datainode) == 0)))

/*
 * Hash function for symlinks.
 */
static unsigned int 
jfs_file_cache_t_hash(jfs_file_cache_t *item)
{
  unsigned int hash;

  hash = item->syminode % JFS_FILE_CACHE_SIZE;

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

static int jfs_file_cache_miss(int syminode, char **sympath, char **datapath, int *datainode);
static int jfs_file_cache_sympath_miss(int datainode, char **sympath, char **datapath, int *syminode);

/*
 * Initialize the jfs_file_cache.
 */
void 
jfs_file_cache_init()
{
  log_msg("JFS_FILE_CACHE_INIT\n");

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
  
  log_msg("JFS_FILE_CACHE_CLEANUP\n");

  for(item = sglib_hashed_jfs_file_cache_t_it_init(&it,hashtable); 
	  item != NULL; item = sglib_hashed_jfs_file_cache_t_it_next(&it)) {
    free(item->sympath);
	free(item);
  }
}

/*
 * Get a datainode from the file cache.
 *
 * Handles the cache miss.
 */
int
jfs_file_cache_get_datainode(int syminode)
{
  jfs_file_cache_t  check;
  jfs_file_cache_t *result;

  int datainode;
  int rc;

  check.syminode = syminode;
  result = sglib_hashed_jfs_file_cache_t_find_member(hashtable, &check);

  if(!result) {
    rc = jfs_file_cache_miss(syminode, NULL, NULL, &datainode);
    if(rc) {
      return rc;
    }

	return datainode;
  }

  return result->datainode;
}

/*
 * Get a datapath from the file cache.
 *
 * Handles the cache miss.
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

  jfs_datapath_cache_get_datapath(result->datainode, datapath);

  return 0;
}

/*
  Get the hardlink path associated with a data file.

  Handles the cache miss.
 */
int
jfs_file_cache_get_sympath(int datainode, char **sympath)
{
  jfs_file_cache_t check;
  jfs_file_cache_t *result;

  check.datainode = datainode;
  result = sglib_hashed_jfs_file_cache_t_find_member(hashtable, &check);

  if(!result) {
    return jfs_file_cache_sympath_miss(datainode, sympath, NULL, NULL);
  }

  *sympath = result->sympath;

  return 0;
}

/*
  Get the datapath and datainode associate with the hardlink inode.

  Handles the cache miss.
 */
int
jfs_file_cache_get_datapath_and_datainode(int syminode, char **datapath, int *datainode)
{
  jfs_file_cache_t check;
  jfs_file_cache_t *result;

  check.syminode = syminode;

  result = sglib_hashed_jfs_file_cache_t_find_member(hashtable, &check);
  if(!result) {
	return jfs_file_cache_miss(syminode, NULL, datapath, datainode);
  }

  *datainode = result->datainode;
  jfs_datapath_cache_get_datapath(result->datainode, datapath);

  return 0;
}

/*
 * Add a hardlink to the jfs_file_cache.
 */
int
jfs_file_cache_add(int syminode, const char *sympath, int datainode, const char *datapath)
{
  jfs_file_cache_t *item;

  item = malloc(sizeof(*item));
  if(!item) {
	log_error("Failed to allocate memory for a file cache symlink.");
	return -ENOMEM;
  }

  item->syminode = syminode;
  item->datainode = datainode;
  item->sympath = (char *)sympath;

  sglib_hashed_jfs_file_cache_t_add(hashtable, item);
  jfs_datapath_cache_add(datainode, (char *)datapath);

  return 0;
}

int
jfs_file_cache_update_sympath(int syminode, const char *sympath)
{
  jfs_file_cache_t check;
  jfs_file_cache_t *result;

  char *new_sympath;
  size_t sympath_len;

  check.syminode = syminode;

  result = sglib_hashed_jfs_file_cache_t_find_member(hashtable, &check);
  if(!result) {
	return -1;
  }

  sympath_len = strlen(sympath) + 1;
  new_sympath = malloc(sizeof(*result->sympath) * sympath_len);
  if(!new_sympath) {
    return -ENOMEM;
  }

  free(result->sympath);
  strncpy(new_sympath, sympath, sympath_len);
  result->sympath = new_sympath;

  return 0;
}

/*
  Remove an item from the file cache. The item may still exist in the db.
 */
int
jfs_file_cache_remove(int syminode)
{
  jfs_file_cache_t check;
  jfs_file_cache_t *elem;
  int rc;

  check.syminode = syminode;

  elem = NULL;
  rc = sglib_hashed_jfs_file_cache_t_delete_if_member(hashtable, &check, &elem);
  if(!rc) {
	return -1;
  }
  
  if(elem) {
    jfs_datapath_cache_remove(elem->datainode);
    free(elem->sympath);
    free(elem); 
  }

  return 0;
}

/*
  Function to handle a regular cache miss.
 */
static int
jfs_file_cache_miss(int syminode, char **sympath, char **datapath, int *datainode)
{
  struct jfs_db_op *db_op;

  char *spath;
  char *dpath;

  size_t sympath_len;
  size_t dpath_len;

  int inode;
  int rc;

  rc = jfs_db_op_create(&db_op);
  if(rc) {
	return rc;
  }

  db_op->op = jfs_file_cache_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
           "SELECT s.sympath, f.datapath, f.inode FROM files AS f, symlinks AS s WHERE f.inode=s.datainode and s.syminode=%d;",
		   syminode);

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
  spath = malloc(sizeof(*sympath) * sympath_len);
  if(!spath) {
	jfs_db_op_destroy(db_op);
	return -ENOMEM;
  }
  strncpy(spath, db_op->result->sympath, sympath_len);

  jfs_db_op_destroy(db_op);

  if(sympath != NULL) {
	*sympath = spath;
  }
  if(datapath != NULL) {
    *datapath = dpath;
  }
  if(datainode != NULL) {
    *datainode = inode;
  }

  return jfs_file_cache_add(syminode, spath, inode, dpath);
}

/*
  Function to handle a sympath lookup cache miss.
 */
static int
jfs_file_cache_sympath_miss(int datainode, char **sympath, char **datapath, int *syminode)
{
  struct jfs_db_op *db_op;

  char *spath;
  char *dpath;

  size_t sympath_len;
  size_t dpath_len;

  int inode;
  int rc;

  printf("----Handling jfs_file_cache miss for datainode:%d\n", datainode);

  rc = jfs_db_op_create(&db_op);
  if(rc) {
	return rc;
  }

  db_op->op = jfs_file_cache_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
           "SELECT s.sympath, f.datapath, s.syminode FROM files AS f, symlinks AS s WHERE f.inode=s.datainode and s.datainode=%d;",
		   datainode);

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
  spath = malloc(sizeof(*sympath) * sympath_len);
  if(!spath) {
	jfs_db_op_destroy(db_op);
	return -ENOMEM;
  }
  strncpy(spath, db_op->result->sympath, sympath_len);

  jfs_db_op_destroy(db_op);

  if(sympath != NULL) {
	*sympath = spath;
  }
  if(datapath != NULL) {
    *datapath = dpath;
  }
  if(syminode != NULL) {
    *syminode = inode;
  }

  return jfs_file_cache_add(inode, spath, datainode, dpath);
}
