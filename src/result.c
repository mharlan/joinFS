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

  switch(db_op->res_t) {
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
	rc = JFS_QUERY_FAILED;
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
  int error;
  int rc;

  row = malloc(sizeof(*row));
  if(!row) {
	printf("Failed to allocate memory for for row.\n");
	return JFS_QUERY_FAILED;
  }
  rc = sqlite3_step(stmt);
  if(rc != SQLITE_ROW) {
    printf("Query failed to get keyid, rc:%d\n", rc);
	row->value = 0;
	error = JFS_QUERY_FAILED;
  }
  else {
	value = sqlite3_column_text(stmt, 0);
	value_len = sqlite3_column_bytes(stmt, 0) + 1;

	if(value_len < 1) {
	  printf("Badpath returned.\n");
	  row->value = 0;
	  error = JFS_QUERY_FAILED;
	}
	else {
	  printf("Size of value in bytes:%d, value:%s\n", value_len, value);
	  row->value = malloc(sizeof(*row->value) * value_len);
	  strncpy(row->value, (const char *)value, value_len);

	  *size = 1;
	  error = JFS_QUERY_SUCCESS;
	}
  }

  rc = sqlite3_finalize(stmt);
  if(rc != SQLITE_OK) {
    printf("Finalizing failed, rc:%d\n", rc);
  }

  if(error == JFS_QUERY_SUCCESS) {
	*result = row;
  }
  else {
	free(row);
  }

  return error;
}

/*
 * Used for getting the keyid in the system.
 */
static int 
jfs_do_key_op(jfs_list_t **result, sqlite3_stmt *stmt, int *size)
{
  jfs_list_t *row;
  int keyid;
  int error;
  int rc;

  row = malloc(sizeof(*row));
  if(!row) {
	printf("Failed to allocate memory for for row.\n");
	return JFS_QUERY_FAILED;
  }

  rc = sqlite3_step(stmt);
  if(rc != SQLITE_ROW) {
    printf("Query failed to get keyid, rc:%d\n", rc);
	row->keyid = 0;
	error = JFS_QUERY_FAILED;
  }
  else {
	keyid = sqlite3_column_int(stmt, 0);

	if(keyid == 0) {
	  printf("Bad keyid returned.\n");
	  row->keyid = 0;
	  error = JFS_QUERY_FAILED;
	}
	else {
	  printf("Returned keyid:%d\n", keyid);
	  row->keyid = keyid;
	  error = JFS_QUERY_SUCCESS;
	}
  }

  rc = sqlite3_finalize(stmt);
  if(rc != SQLITE_OK) {
    printf("Finalizing failed, rc:%d\n", rc);
  }

  if(error == JFS_QUERY_SUCCESS) {
	*result = row;
  }
  else {
	free(row);
  }

  return error;
}

/*
 * Generate a datapath query result.
 */
static int 
jfs_do_file_cache_op(jfs_list_t **result, sqlite3_stmt *stmt, int *size)
{
  jfs_list_t *row;
  const unsigned char *datapath;
  int inode;
  int path_len;
  int error;
  int rc;

  row = malloc(sizeof(*row));
  if(!row) {
	printf("Failed to allocate memory for forw.\n");
	return JFS_QUERY_FAILED;
  }

  rc = sqlite3_step(stmt);
  if(rc != SQLITE_ROW) {
    printf("Query Failed to get datapath, rc:%d\n", rc);
    row->datapath = 0;
	error = JFS_QUERY_FAILED;
  }
  else {
	datapath = sqlite3_column_text(stmt, 0);
	path_len = sqlite3_column_bytes(stmt, 0) + 1;
	inode = sqlite3_column_int(stmt, 1);

	if(!path_len) {
	  printf("Badpath returned.\n");
	  row->datapath = 0;
	  error = JFS_QUERY_FAILED;
	}
	else {
	  printf("Size of datapath in bytes:%d, datapath:%s\n", path_len, datapath);
	  row->datapath = malloc(sizeof(*row->datapath) * path_len);

	  if(!row->datapath) {
		printf("Failed to allocate memory for datapath in file_cache_op.\n");
		error = JFS_QUERY_FAILED;
	  }
	  else {
		strncpy(row->datapath, (const char *)datapath, path_len);
		row->inode = inode;
		
		*size = 1;
		error = JFS_QUERY_SUCCESS;
	  }
	}
  }

  rc = sqlite3_finalize(stmt);
  if(rc != SQLITE_OK) {
    printf("Finalizing failed, rc:%d\n", rc);
  }

  if(error == JFS_QUERY_SUCCESS) {
	*result = row;
  }
  else {
	free(row);
  }

  return error;
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
  int error;
  int rc;

  head = NULL;
  buffer_size = 0;
  while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
	row = malloc(sizeof(*row));
	if(!row) {
	  printf("Failed to allocate memory for row in listattr.\n");
	  error = JFS_QUERY_FAILED;
	  break;
	}

	keyid = sqlite3_column_int(stmt, 0);
	key = sqlite3_column_text(stmt, 1);
	key_len = sqlite3_column_bytes(stmt, 1) + 1;

	row->keyid = keyid;
	row->key = malloc(sizeof(*row->key) * key_len);
	if(!key) {
	  printf("Failed to allocate memory for listattr key.\n");
	  error = JFS_QUERY_FAILED;
	  break;
	}

	strncpy(row->key, (const char *)key, key_len);
	buffer_size += key_len;

	printf("--Adding key:%s to jfs_list.\n", row->key);

	jfs_list_t_add(&head, row);
	++rows;
  }

  if(rc != SQLITE_DONE) {
	error = JFS_QUERY_FAILED;
	*size = 0;
  }
  else {
	error = JFS_QUERY_SUCCESS;
	*size = rows;
  }

  rc = sqlite3_finalize(stmt);
  if(rc != SQLITE_OK) {
    printf("Finalizing failed, rc:%d\n", rc);
  }

  *buff_size = buffer_size;
  *result = head;
  
  return error;
}

/*
 * Result processing for dynamic files.
 */
static int
jfs_do_dynamic_file_op(jfs_list_t **result, sqlite3_stmt *stmt, int *size)
{
  jfs_list_t *head;
  jfs_list_t *row;
  int rows;
  int rc;
  
  rows = 0;
  while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
	row = malloc(sizeof(*row));
	//copy row here
	jfs_list_t_add(&head, row);
	++rows;
  }
  *size = rows;
  
  if(rc != SQLITE_DONE) {
	log_error("A SQLite error has occured while stepping, rc:%d\n", 
			  rc);
  }

  rc = sqlite3_finalize(stmt);
  if(rc != SQLITE_OK) {
    log_error("Finalizing failed, rc:%d\n", rc);
  }

  return rc;
}

/*
 * Performs a database write operation.
 */
static int
jfs_do_write_op(sqlite3 *db, sqlite3_stmt *stmt)
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
  }

  printf("Finished write operation, error:%d\n", error);
  
  return error;
}
