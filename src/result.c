/*
 * joinFS - Query result generation module
 * Matthew Harlan <mharlan@gwmail.gwu.edu>
 */

#if !defined(_REENTRANT)
#define	_REENTRANT
#endif

#include "error_log.h"
#include "jfs_list.h"
#include "sqlitedb.h"
#include "result.h"

#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

static int jfs_do_write_op(sqlite3 *db, sqlite3_stmt *stmt, int *error);
static int jfs_do_datapath_op(jfs_list_t **result, int *error,
							  sqlite3_stmt *stmt, int *size);
static int jfs_s_file_result(jfs_list_t **result, int *error, 
							 sqlite3_stmt *stmt, int *size);
static int jfs_d_file_result(jfs_list_t **result, int *error, 
							 sqlite3_stmt *stmt, int *size);

/*
 * Processes the result of a jfs_db_op.
 */
int
jfs_db_result(struct jfs_db_op *db_op)
{
  int rc;

  switch(db_op->res_t) {
  case(jfs_write_op):
	rc = jfs_do_write_op(db_op->db, db_op->stmt, &db_op->error);
	break;
  case(jfs_datapath_op):
	rc = jfs_do_datapath_op(&db_op->result, &db_op->error, db_op->stmt, &db_op->size);
	break;
  case(jfs_s_file):
	rc = jfs_s_file_result(&db_op->result, &db_op->error, db_op->stmt, &db_op->size);
	break;
  case(jfs_d_file):
	rc = jfs_d_file_result(&db_op->result, &db_op->error, db_op->stmt, &db_op->size);
	break;
  default:
	rc = -1;
	log_error("jfs_t:%d not implemented yet.\n");
  }

  return rc;
}

/*
 * Generate a datapath query result.
 */
static int 
jfs_do_datapath_op(jfs_list_t **result, int *error,
				   sqlite3_stmt *stmt, int *size)
{
  jfs_list_t *row;
  const unsigned char *datapath;
  int path_len;
  int rc;

  row = malloc(sizeof(*row));
  *result = row;

  rc = sqlite3_step(stmt);
  if(rc != SQLITE_ROW) {
    log_error("Failed to get datapath, rc:%d\n", rc);
    row->datapath = 0;
	*error = JFS_QUERY_FAILED;
  }
  else {
	datapath = sqlite3_column_text(stmt, 0);
	path_len = sqlite3_column_bytes(stmt, 0);

    row->datapath = malloc(sizeof(*row->datapath) * path_len);
	strcpy(row->datapath, (const char *)datapath);

	*size = 1;
	*error = JFS_QUERY_SUCCESS;
  }

  rc = sqlite3_step(stmt);
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
 * Result processing for static files.
 */
static int 
jfs_s_file_result(jfs_list_t **result, int *error,
				  sqlite3_stmt *stmt, int *size)
{
  jfs_list_t *row;
  int rc;

  row = malloc(sizeof(*row));
  *result = row;

  rc = sqlite3_step(stmt);
  if(rc != SQLITE_ROW) {
    log_error("Failed to get inode, rc:%d\n", rc);
    row->inode = 0;
	*error = JFS_QUERY_FAILED;
  }
  else {
    row->inode = sqlite3_column_int(stmt, 0);
	*size = 1;
	*error = JFS_QUERY_SUCCESS;
  }
  
  rc = sqlite3_step(stmt);
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
 * Result processing for dynamic files.
 */
static int
jfs_d_file_result(jfs_list_t **result, int *error,
				  sqlite3_stmt *stmt, int *size)
{
  jfs_list_t *row;
  int rows;
  int rc;
  
  rows = 0;
  while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
	row = malloc(sizeof(*row));
	//copy row here
	sglib_jfs_list_t_add(result, row);
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
jfs_do_write_op(sqlite3 *db, sqlite3_stmt *stmt, int *error)
{
  int rc;

  sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);

  rc = sqlite3_step(stmt);
  if(rc != SQLITE_DONE) {
	log_error("Statement didn't finish executing.\n");
	*error = JFS_QUERY_FAILED;
  }
  else {
	*error = JFS_QUERY_SUCCESS;
  }

  rc = sqlite3_finalize(stmt);
  if(rc != SQLITE_OK) {
    log_error("Finalizing failed, rc:%d\n", rc);
  }

  if(*error == JFS_QUERY_SUCCESS) {
	sqlite3_exec(db, "COMMIT TRANSACTION", 0, 0, 0);
  }
  else {
	sqlite3_exec(db, "ROLLBACK TRANSACTION", 0, 0, 0);
  }
  
  return rc;
}
