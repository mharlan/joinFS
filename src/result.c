/*
 * joinFS - Query result generation module
 * Matthew Harlan <mharlan@gwmail.gwu.edu>
 */

#include "include/sqlitedb.h"
#include "include/result.h"
#include "include/sglib.h"

SGLIB_DEFINE_LIST_FUNCTIONS(jfs_list_t, JFS_LIST_CMP, next)

int jfs_result(struct jfs_db_op *db_op)
{
  int rc;

  db_op->result = NULL;
  switch(db_op->res_t) {
  case(jfs_s_file):
	rc = jfs_s_file_result(db_op->result, db_op->stmt, &db_op->size);
	break;
  default:
	log_error("jfs_t:%d not implemented yet.\n");
  }
}

int jfs_s_file_result(jfs_list_t *result, sqlite3 *stmt, int *size)
{
  jfs_list_t *row;
  int rc;

  rc = sqlite3_step(stmt);
  if(rc != SQLITE_ROW) {
	log_error("Failed to get inode, rc:%d\n", rc);
  }
  else {
	row = malloc(sizeof(*row));
	row->inode = sqlite3_column_int(stmt, 0);
	sglib_jfs_list_t_add(&result, row);
  }
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

int jfs_d_file_result(jfs_list_t *result, sqlite3_stmt *stmt, int *size)
{
  jfs_list_t *row;
  int rc;

  rows = 0;
  while((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
	row = malloc(sizeof(*row));
	//copy row here
	sglib_jfs_list_t_add(&result, row);
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
