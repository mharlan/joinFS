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
#include <sqlite3.h>

static int jfs_s_file_result(jfs_list_t **result, 
							 sqlite3_stmt *stmt, int *size);
static int jfs_d_file_result(jfs_list_t **result, 
							 sqlite3_stmt *stmt, int *size);

/*
 * Processes the result of a jfs_db_op.
 */
int
jfs_db_result(struct jfs_db_op *db_op)
{
  int rc;

  switch(db_op->res_t) {
  case(jfs_s_file):
	rc = jfs_s_file_result(&db_op->result, db_op->stmt, &db_op->size);
	if(!db_op->result->inode) {
	  log_error("FAILED: Query='%s'\n", db_op->query);
	}
	break;
  case(jfs_d_file):
	rc = jfs_d_file_result(&db_op->result, db_op->stmt, &db_op->size);
	break;
  default:
	rc = -1;
	log_error("jfs_t:%d not implemented yet.\n");
  }

  return rc;
}

/*
 * Result processing for static files.
 */
static int 
jfs_s_file_result(jfs_list_t **result, 
				  sqlite3_stmt *stmt, int *size)
{
  jfs_list_t *row;
  int rc;

  row = malloc(sizeof(*row));

  rc = sqlite3_step(stmt);
  if(rc != SQLITE_ROW) {
    log_error("Failed to get inode, rc:%d\n", rc);
    row->inode = 0;
  }
  else {
    row->inode = sqlite3_column_int(stmt, 0);
  }
  *result = row;
  *size = 1;
  
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
jfs_d_file_result(jfs_list_t **result, 
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
