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
#include "sqlitedb.h"
#include "result.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sqlite3.h>
#include <sys/types.h>

static int jfs_do_write_op(sqlite3 *db, sqlite3_stmt *stmt);
static int jfs_do_file_cache_op(jfs_list_t **result, sqlite3_stmt *stmt, int *size);
static int jfs_do_key_op(jfs_list_t **result, sqlite3_stmt *stmt, int *size);
static int jfs_do_attr_op(jfs_list_t **result, sqlite3_stmt *stmt, int *size);
static int jfs_do_listattr_op(jfs_list_t **result, sqlite3_stmt *stmt, int *size, size_t *buff_size);
static int jfs_do_dynamic_file_op(jfs_list_t **result, sqlite3_stmt *stmt, int *size);

/*
 * Processes the result of a jfs_db_op.
 */
int
jfs_db_result(struct jfs_db_op *db_op)
{
  int rc;

  switch(db_op->op) {
  case(jfs_write_op):
	rc = jfs_do_write_op(db_op->db, db_op->stmt);
	break;
  case(jfs_attr_op):
	rc = jfs_do_attr_op(&db_op->result, db_op->stmt, &db_op->size);
	break;
  case(jfs_listattr_op):
	rc = jfs_do_listattr_op(&db_op->result, db_op->stmt, &db_op->size, &db_op->buffer_size);
	break;
  case(jfs_file_cache_op):
	rc = jfs_do_file_cache_op(&db_op->result, db_op->stmt, &db_op->size);
	break;
  case(jfs_key_op):
	rc = jfs_do_key_op(&db_op->result, db_op->stmt, &db_op->size);
	break;
  case(jfs_dynamic_file_op):
	rc = jfs_do_dynamic_file_op(&db_op->result, db_op->stmt, &db_op->size);
	break;
  default:
	rc = -EOPNOTSUPP;
	log_error("jfs_t:%d not implemented yet.\n");
  }

  return rc;
}

/*
 * Used for getting the value of an xattr.
 */
int 
jfs_do_attr_op(jfs_list_t **result, sqlite3_stmt *stmt, int *size)
{
  jfs_list_t *row;
  const unsigned char* value;
  int value_len;
  int rc;

  row = malloc(sizeof(*row));
  if(!row) {
	sqlite3_finalize(stmt);
	return -ENOMEM;
  }

  rc = sqlite3_step(stmt);
  if(rc != SQLITE_ROW) {
	free(row);
  }
  else {
	value = sqlite3_column_text(stmt, 0);
	value_len = sqlite3_column_bytes(stmt, 0) + 1;

	row->value = malloc(sizeof(*row->value) * value_len);
	if(!row->value) {
	  sqlite3_finalize(stmt);
	  free(row);
	  return -ENOMEM;
	}
	strncpy(row->value, (const char *)value, value_len);

	*size = 1;
	*result = row;
  }

  return sqlite3_finalize(stmt);
}

/*
 * Used for getting the keyid in the system.
 */
static int 
jfs_do_key_op(jfs_list_t **result, sqlite3_stmt *stmt, int *size)
{
  jfs_list_t *row;
  int keyid;
  int rc;

  row = malloc(sizeof(*row));
  if(!row) {
	sqlite3_finalize(stmt);
	return -ENOMEM;
  }

  rc = sqlite3_step(stmt);
  if(rc != SQLITE_ROW) {
	free(row);
  }
  else {
	keyid = sqlite3_column_int(stmt, 0);

	row->keyid = keyid;
	*result = row;
  }

  return sqlite3_finalize(stmt);
}

/*
 * Generate a datapath query result.
 */
static int 
jfs_do_file_cache_op(jfs_list_t **result, sqlite3_stmt *stmt, int *size, int *error_code)
{
  const unsigned char *datapath;
  jfs_list_t *row;
  int inode;
  int path_len;
  int rc;

  row = malloc(sizeof(*row));
  if(!row) {
	sqlite3_finalize(stmt);
	return -ENOMEM;
  }

  rc = sqlite3_step(stmt);
  if(rc != SQLITE_ROW) {
	free(row);
  }
  else {
	datapath = sqlite3_column_text(stmt, 0);
	path_len = sqlite3_column_bytes(stmt, 0) + 1;
	inode = sqlite3_column_int(stmt, 1);

	row->datapath = malloc(sizeof(*row->datapath) * path_len);
	if(!row->datapath) {
	  sqlite3_finalize(stmt);
	  free(row);
	  return -ENOMEM;
	}

	strncpy(row->datapath, (const char *)datapath, path_len);
	row->inode = inode;
	  
	*size = 1;
	*result = row;
  }

  return sqlite3_finalize(stmt);
}

/* 
 * Returns a linked list of xattr keys.
 */
static int
jfs_do_listattr_op(jfs_list_t **result, sqlite3_stmt *stmt, int *size, size_t *buff_size)
{
  const unsigned char *key;
  size_t buffer_size;

  jfs_list_t *head;
  jfs_list_t *row;

  int key_len;
  int keyid;
  int rows;
  int rc;

  head = NULL;
  buffer_size = 0;
  while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
	row = malloc(sizeof(*row));
	if(!row) {
	  sqlite3_finalize(stmt);
	  jfs_list_destroy(head);
	  return -ENOMEM;
	}

	keyid = sqlite3_column_int(stmt, 0);
	key = sqlite3_column_text(stmt, 1);
	key_len = sqlite3_column_bytes(stmt, 1) + 1;

	row->keyid = keyid;
	row->key = malloc(sizeof(*row->key) * key_len);
	if(!row->key) {
	  sqlite3_finalize(stmt);
	  jfs_list_destroy(head);
	  return -ENOMEM;
	}

	strncpy(row->key, (const char *)key, key_len);
	buffer_size += key_len;

	jfs_list_add(&head, row);
	++rows;
  }

  if(rc == SQLITE_DONE) {
	*size = rows;
	*result = head;
  }
  
  return sqlite3_finalize(stmt);
}

/*
 * Result processing for dynamic files.
 */
static int
jfs_do_dynamic_file_op(jfs_list_t **result, sqlite3_stmt *stmt, int *size, int *error_code)
{
  jfs_list_t *head;
  jfs_list_t *row;

  int rows;
  int rc;
  
  rows = 0;
  while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
	row = malloc(sizeof(*row));
	if(!row) {
	  sqlite3_finalize(stmt);
	  jfs_list_destroy(head);
	  return -ENOMEM;
	}

	//copy row here
	jfs_list_add(&head, row);
	++rows;
  }
  
  if(rc == SQLITE_DONE) {
	*size = rows;
	*result = head;
  }

  return sqlite3_finalize(stmt);
}

/*
 * Performs a database write operation.
 */
static int
jfs_do_write_op(sqlite3 *db, sqlite3_stmt *stmt, int *error_code)
{
  int error;
  int rc;

  printf("Performing write op.\n");

  rc = sqlite3_step(stmt);
  printf("Statement step result:%d\n", rc);
  if(rc != SQLITE_DONE) {
	printf("Statement didn't finish executing.\n");
	error = JFS_QUERY_FAILED;
  }
  else {
	error = JFS_QUERY_SUCCESS;
  }

  rc = sqlite3_finalize(stmt);
  if(rc != SQLITE_OK) {
    printf("Finalizing failed, rc:%d\n", rc);
	*error_code = rc;
  }

  printf("Finished write operation, error:%d\n", error);
  
  return error;
}
