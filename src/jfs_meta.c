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
  if(datainode < 0) {
	return -1;
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
  int syminode;
  int keyid;
  size_t size;

  syminode = jfs_util_get_inode(path);
  if(syminode < 0) {
	return syminode;
  }

  keyid = jfs_util_get_keyid(key);
  if(keyid < 1) {
	return -1;
  }
  
  db_op = jfs_db_op_create();
  db_op->res_t = jfs_attr_op;

  snprintf(db_op->query, JFS_QUERY_MAX,
		   "SELECT keyvalue FROM metadata WHERE keyid=%d and inode=(SELECT datainode FROM symlinks WHERE syminode=%d);",
		   keyid, syminode);

  printf("Getxattr query:%s\n", db_op->query);

  jfs_read_pool_queue(db_op);
  jfs_db_op_wait(db_op);

  if(db_op->error == JFS_QUERY_FAILED) {
	jfs_db_op_destroy(db_op);
	return -1;
  }
  
  size = strlen(db_op->result->value);
  if(buffer_size > size) {
	strcpy(value, db_op->result->value);
  }

  jfs_db_op_destroy(db_op);

  return size;
}

/*
A single extended attribute name is a simple NULL-terminated string. 
The name includes a namespace prefix - there may be several, disjoint namespaces 
associated with an individual inode.

An empty buffer of size zero can be passed into these calls to return the 
current size of the list of extended attribute names, which can be used to 
estimate the size of a buffer which is sufficiently large to hold the list of names.

Examples

The list of names is returned as an unordered array of NULL-terminated character strings 
(attribute names are separated by NULL characters), like this:

    user.name1\0system.name1\0user.name2\0

Filesystems like ext2, ext3 and XFS which implement POSIX ACLs using extended attributes, 
might return a list like this:

    system.posix_acl_access\0system.posix_acl_default\0

Return Value

On success, a positive number is returned indicating the size of the extended 
attribute name list. On failure, -1 is returned and errno is set appropriately.

If the size of the list buffer is too small to hold the result, errno is set to ERANGE.

If extended attributes are not supported by the filesystem, or are disabled, 
errno is set to ENOTSUP.

The errors documented for the stat(2) system call are also applicable here. 
 */
int
jfs_meta_listxattr(const char *path, char *list, size_t size)
{
  struct sglib_jfs_list_t_iterator it;
  struct jfs_db_op *db_op;
  jfs_list_t *item;
  char *list_pos;
  size_t buff_size;
  int datainode;

  datainode = jfs_util_get_datainode(path);
  
  db_op = jfs_db_op_create();
  db_op->res_t = jfs_listattr_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "SELECT keyid, keytext FROM keys WHERE keyid=(SELECT keyid FROM metadata WHERE inode=%d);",
		   datainode);

  jfs_read_pool_queue(db_op);
  jfs_db_op_wait(db_op);

  if(db_op->error == JFS_QUERY_FAILED) {
	jfs_db_op_destroy(db_op);
	return -1;
  }

  buff_size = db_op->buffer_size;
  if(size < buff_size) {
	jfs_db_op_destroy(db_op);
	return buff_size;
  }

  list_pos = list;
  for(item = sglib_jfs_list_t_it_init(&it, db_op->result); 
	  item != NULL; item = sglib_jfs_list_t_it_next(&it)) {
	strcpy(list_pos, item->key);
	list_pos += strlen(item->key) + 1;
  }

  return buff_size;
}

int
jfs_meta_removexattr(const char *path, const char *key)
{
  struct jfs_db_op *db_op;
  int syminode;
  int keyid;

  syminode = jfs_util_get_inode(path);
  if(syminode < 0) {
	return syminode;
  }

  keyid = jfs_util_get_keyid(key);
  if(keyid < 1) {
	return -1;
  }
  
  db_op = jfs_db_op_create();
  db_op->res_t = jfs_write_op;

  snprintf(db_op->query, JFS_QUERY_MAX,
		   "DELETE FROM metadata WHERE keyid=%d and inode=(SELECT datainode FROM symlinks WHERE syminode=%d);",
		   keyid, syminode);

  jfs_write_pool_queue(db_op);
  jfs_db_op_wait(db_op);

  if(db_op->error == JFS_QUERY_FAILED) {
	jfs_db_op_destroy(db_op);
	return -1;
  }
  jfs_db_op_destroy(db_op);

  return 0;
}
