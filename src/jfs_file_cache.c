/*
 * joinFS - File Module Caching Code
 * Matthew Harlan <mharlan@gwmail.gwu.edu>
 *
 * 30 % Demo
 * Very simple cache. Not intelligent at all.
 */
#include "error_log.h"
#include "jfs_file_cache.h"
#include "sglib.h"

#include <stdlib.h>
#include <string.h>

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
  return item->syminode % JFS_FILE_CACHE_SIZE;
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

  for(item = sglib_hashed_jfs_file_cache_t_it_init(&it,hashtable); 
	  item != NULL; item = sglib_hashed_jfs_file_cache_t_it_next(&it)) {
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
  jfs_file_cache_t check;
  jfs_file_cache_t *result;

  check.syminode = syminode;
  result = sglib_hashed_jfs_file_cache_t_find_member(hashtable, &check);

  if(!result) {
	return 0;
  }

  return result->datainode;
}

/*
 * Get a datainode from the file cache.
 *
 * Returns 0 if not in the cache.
 */
char *
jfs_file_cache_get_datapath(int syminode)
{
  jfs_file_cache_t check;
  jfs_file_cache_t *result;

  check.syminode = syminode;
  result = sglib_hashed_jfs_file_cache_t_find_member(hashtable, &check);

  if(!result) {
	return 0;
  }

  return result->datapath;
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
	log_error("Failed to allocate memory for a file cache symlink.");
	return 1;
  }

  item->syminode = syminode;
  item->datainode = datainode;

  item->datapath = malloc(sizeof(*item->datapath) * strlen(datapath) + 1);
  strcpy(item->datapath, datapath);

  sglib_hashed_jfs_file_cache_t_add(hashtable, item);

  return 0;
}
