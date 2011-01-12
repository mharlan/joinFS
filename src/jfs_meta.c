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

#include "jfs_list.h"
#include "jfs_util.h"
#include "jfs_meta.h"
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
  int datainode;
  int keyid;

  keyid = jfs_util_get_keyid(key);
  if(keyid < 1) {
	return -1;
  }

  datainode = jfs_util_get_datainode(path);
  if(datainode < 1) {
	return datainode;
  }

  db_op = jfs_db_op_create();
  db_op->res_t = jfs_write_op;

  if(flags == XATTR_CREATE) {
	snprintf(db_op->query, JFS_QUERY_MAX,
			 "INSERT OR ROLLBACK INTO metadata VALUES(%d,%d,\"%s\");",
			 datainode, keyid, value);
  }
  else if(flags == XATTR_REPLACE) {
	snprintf(db_op->query, JFS_QUERY_MAX,
			 "REPLACE INTO metadata VALUES(%d,%d,\"%s\");",
			 datainode, keyid, value);
  }
  else {
	snprintf(db_op->query, JFS_QUERY_MAX,
			 "INSERT OR REPLACE INTO metadata VALUES(%d,%d,\"%s\");",
			 datainode, keyid, value);
  }

  printf("setxattr query:%s\n", db_op->query);

  jfs_write_pool_queue(db_op);
  jfs_db_op_wait(db_op);

  if(db_op->error == JFS_QUERY_FAILED) {
	jfs_db_op_destroy(db_op);
	return -1;
  }
  jfs_db_op_destroy(db_op);

  return 0;
}

int
jfs_meta_getxattr(const char *path, const char *key, void *value,
				  size_t buffer_size)
{
  struct jfs_db_op *db_op;
  size_t size;
  int datainode;

  datainode = jfs_util_get_datainode(path);
  if(datainode < 1) {
	return datainode;
  }
  
  db_op = jfs_db_op_create();
  db_op->res_t = jfs_attr_op;

  snprintf(db_op->query, JFS_QUERY_MAX,
		   "SELECT keyvalue FROM metadata WHERE inode=%d and keyid=(SELECT keyid FROM keys WHERE keytext=\"%s\");",
		   datainode, key);

  printf("Getxattr query:%s\n", db_op->query);

  jfs_read_pool_queue(db_op);
  jfs_db_op_wait(db_op);

  if(db_op->error == JFS_QUERY_FAILED) {
	jfs_db_op_destroy(db_op);
	return -1;
  }
  
  size = strlen(db_op->result->value) + 1;
  if(buffer_size >= size) {
	strncpy(value, db_op->result->value, size);
  }

  jfs_db_op_destroy(db_op);

  return size;
}

int
jfs_meta_listxattr(const char *path, char *list, size_t buffer_size)
{
  struct sglib_jfs_list_t_iterator it;
  struct jfs_db_op *db_op;
  jfs_list_t *item;
  char *list_pos;
  size_t list_size;
  size_t attr_size;
  int datainode;
  int pos;

  printf("Called jfs_meta_listxattr.\n");

  datainode = jfs_util_get_datainode(path);
  if(datainode < 1) {
	return datainode;
  }
  
  db_op = jfs_db_op_create();
  db_op->res_t = jfs_listattr_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "SELECT k.keyid, k.keytext FROM keys AS k, metadata AS m WHERE k.keyid=m.keyid and m.inode=%d;",
		   datainode);

  printf("Listxattr query:%s\n", db_op->query);

  jfs_read_pool_queue(db_op);
  jfs_db_op_wait(db_op);

  if(db_op->error == JFS_QUERY_FAILED) {
	jfs_db_op_destroy(db_op);
	return -1;
  }

  list_size = db_op->buffer_size;
  if(buffer_size < list_size) {
	jfs_db_op_destroy(db_op);
	return list_size;
  }

  pos = 0;
  list_pos = list;
  for(item = sglib_jfs_list_t_it_init(&it, db_op->result); 
	  item != NULL; item = sglib_jfs_list_t_it_next(&it)) {
	printf("Extracting Key:%s\n", item->key);

	attr_size = strlen(item->key) + 1;
	strncpy(list_pos, item->key, attr_size);

	list_pos += attr_size;
	printf("List:%p, List_pos:%p\n", list, list_pos);

	free(item->key);
	free(item);
  }

  return list_size;
}

int
jfs_meta_removexattr(const char *path, const char *key)
{
  struct jfs_db_op *db_op;
  int datainode;
  int keyid;

  datainode = jfs_util_get_datainode(path);
  if(datainode < 1) {
	return datainode;
  }

  keyid = jfs_util_get_keyid(key);
  if(keyid < 1) {
	return -1;
  }
  
  db_op = jfs_db_op_create();
  db_op->res_t = jfs_write_op;

  snprintf(db_op->query, JFS_QUERY_MAX,
		   "DELETE FROM metadata WHERE keyid=%d and inode=%d;",
		   keyid, datainode);

  jfs_write_pool_queue(db_op);
  jfs_db_op_wait(db_op);

  if(db_op->error == JFS_QUERY_FAILED) {
	jfs_db_op_destroy(db_op);
	return -1;
  }
  jfs_db_op_destroy(db_op);

  return 0;
}
