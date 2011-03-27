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
#include "jfs_list.h"
#include "jfs_util.h"
#include "jfs_meta.h"
#include "jfs_meta_cache.h"
#include "jfs_key_cache.h"
#include "sqlitedb.h"
#include "joinfs.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <attr/xattr.h>

int
jfs_meta_setxattr(const char *path, const char *key, const char *value,
				  size_t size, int flags)
{
  struct jfs_db_op *db_op;
  char *safe_value;
  
  int keyid;
  int rc;
  
  safe_value = malloc(sizeof(*safe_value) * (size + 1));
  if(!safe_value) {
	return -ENOMEM;
  }
  memcpy(safe_value, value, size);
  safe_value[size] = '\0';
  
  keyid = jfs_util_get_keyid(key);
  if(keyid < 1) {
    free(safe_value);
	return keyid;
  }
  
  if(flags == XATTR_CREATE) {
	rc = jfs_db_op_create(&db_op, jfs_write_op,
                          "INSERT OR ROLLBACK INTO metadata VALUES((SELECT jfs_id FROM links WHERE path=\"%s\"),%d,\"%s\");",
                          path, keyid, safe_value);
  }
  else if(flags == XATTR_REPLACE) {
	rc = jfs_db_op_create(&db_op, jfs_write_op,
                          "REPLACE INTO metadata VALUES((SELECT jfs_id FROM links WHERE path=\"%s\"),%d,\"%s\");",
                          path, keyid, safe_value);
  }
  else {
	rc = jfs_db_op_create(&db_op, jfs_write_op,
                          "INSERT OR REPLACE INTO metadata VALUES((SELECT jfs_id FROM links WHERE path=\"%s\"),%d,\"%s\");",
                          path, keyid, safe_value);
  }
  
  if(rc) {
    free(safe_value);
    return rc;
  }
  
  jfs_write_pool_queue(db_op);
  rc = jfs_db_op_wait(db_op);
  jfs_db_op_destroy(db_op);

  if(rc) {
    free(safe_value);
	return rc;
  }
  
  rc = jfs_meta_cache_add(path, keyid, safe_value);
  free(safe_value);

  return rc;
}

int
jfs_meta_getxattr(const char *path, const char *key, void *value,
				  size_t buffer_size)
{
  char *cache_value;
  size_t size;
  int rc;
  
  rc = jfs_meta_do_getxattr(path, key, &cache_value);
  if(rc) {
    return rc;
  }

  size = strlen(cache_value) + 1;
  if(buffer_size >= size) {
    strncpy(value, cache_value, size);
  }
  free(cache_value);

  return size;
}

/*
  Consolidate cache miss into metadata cache.
 */
int
jfs_meta_do_getxattr(const char *path, const char *key, char **value)
{
  struct jfs_db_op *db_op;
  
  char *cache_value;
  
  size_t size;
  
  int keyid;
  int rc;

  keyid = jfs_util_get_keyid(key);
  if(keyid < 1) {
    return keyid;
  }
  
  //try the cache first
  rc = jfs_meta_cache_get_value(path, keyid, &cache_value);
  if(!rc) {
    *value = cache_value;
    return 0;    
  }
  
  //cache miss, go out to the db
  rc = jfs_db_op_create(&db_op, jfs_meta_cache_op,
                        "SELECT keyvalue FROM metadata WHERE jfs_id=(SELECT jfs_id FROM links WHERE path=\"%s\") and keyid=%d;",
                        path, keyid);
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

	return -ENOATTR;
  }
  
  size = strlen(db_op->result->value) + 1;
  cache_value = malloc(sizeof(*cache_value) * size);
  if(!cache_value) {
    jfs_db_op_destroy(db_op);

    return -ENOMEM;
  }
  strncpy(cache_value, db_op->result->value, size);
  jfs_db_op_destroy(db_op);

  rc = jfs_meta_cache_add(path, keyid, cache_value);
  if(rc) {
    free(cache_value);

    return rc;
  }
  *value = cache_value;

  return 0;
}

int
jfs_meta_listxattr(const char *path, char *list, size_t buffer_size)
{
  struct sglib_jfs_list_t_iterator it;
  struct jfs_db_op *db_op;
  
  jfs_list_t *item;

  size_t list_size;
  size_t attr_size;

  char *list_pos;
  int rc;
  
  rc = jfs_db_op_create(&db_op, jfs_listattr_op,
                        "SELECT k.keyid, k.keytext FROM keys AS k, metadata AS m WHERE k.keyid=m.keyid and m.jfs_id=(SELECT jfs_id FROM links WHERE path=\"%s\");",
                        path);
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
	return 0;
  }

  list_size = db_op->buffer_size;
  if(buffer_size < list_size) {
	jfs_db_op_destroy(db_op);
	return list_size;
  }

  list_pos = list;
  for(item = sglib_jfs_list_t_it_init(&it, db_op->result); 
	  item != NULL; item = sglib_jfs_list_t_it_next(&it)) {
	attr_size = strlen(item->key) + 1;
	strncpy(list_pos, item->key, attr_size);

	list_pos += attr_size;

    jfs_key_cache_add(item->keyid, item->key);
	
	free(item->key);
	free(item);
  }
  
  return list_size;
}

int
jfs_meta_removexattr(const char *path, const char *key)
{
  struct jfs_db_op *db_op;

  int keyid;
  int rc;
  
  keyid = jfs_util_get_keyid(key);
  if(keyid < 1) {
    return keyid;
  }
  
  rc = jfs_db_op_create(&db_op, jfs_write_op,
                        "DELETE FROM metadata WHERE jfs_id=(SELECT jfs_id FROM links WHERE path=\"%s\") and keyid=%d;",
                        path, keyid);
  if(rc) {
	return rc;
  }
  
  jfs_write_pool_queue(db_op);
  rc = jfs_db_op_wait(db_op);
  jfs_db_op_destroy(db_op);

  if(rc) {
	return rc;
  }

  rc = jfs_meta_cache_remove(path, keyid);
  if(rc) {
    return rc;
  }

  return 0;
}
