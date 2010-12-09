/*
 * joinFS: SQLite interface module
 * Matthew Harlan <mharlan@gwmail.gwu.edu>
 *
 * 30% Demo
 */

#if !defined(_REENTRANT)
#define	_REENTRANT
#endif

#include "error_log.h"
#include "sqlitedb.h"
#include "result.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <pthread.h>

#define JFSDB   "/home/joinfs/git/joinFS/demo/joinfs.db"
#define ERR_MAX 256

//does not need to be public, prepares queries
static int setup_stmt(sqlite3 *db, sqlite3_stmt **stmt, const char* query);

/*
 * Creates a database operation.
 */
struct jfs_db_op *
jfs_db_op_create()
{
  struct jfs_db_op *db_op;

  db_op = malloc(sizeof(*db_op));
  if(!db_op) {
	log_error("Failed to allocate memory for db_op.");
	return 0;
  }

  db_op->db = NULL;
  db_op->stmt = NULL;
  db_op->size = 0;
  db_op->error = 0;

  memset(db_op->query, 0, JFS_QUERY_MAX);

  pthread_cond_init(&db_op->cond, NULL);
  pthread_mutex_init(&db_op->mut, NULL);

  return db_op;
}

/*
 * Destroy a database operation.
 *
 * TODO: Cleanup code for the result.
 */
void
jfs_db_op_destroy(struct jfs_db_op *db_op)
{
  switch(db_op->res_t) {
  case(jfs_write_op):
	break;
  case(jfs_datapath_op):
	free(db_op->result->datapath);
	free(db_op->result);
	break;
  case(jfs_s_file):
	free(db_op->result);
	break;
  case(jfs_d_file):
	free(db_op->result);
	break;
  default:
	break;
  }

  free(db_op);
}

/*
 * Performs a database operation that blocks while waiting
 * for the query result.
 *
 * Returns the size of the result and an error code if the
 * query failed.
 */
int
jfs_db_op_wait(struct jfs_db_op *db_op)
{
  pthread_mutex_lock(&db_op->mut);
  while(!db_op->error) {
	pthread_cond_wait(&db_op->cond, &db_op->mut);
  }
  pthread_mutex_unlock(&db_op->mut);

  return db_op->error;
}

/*
 * Open a connection to joinfs.db
 */
sqlite3 *
jfs_open_db(int sqlite_attr)
{
  const char *dbfile = JFSDB;
  sqlite3 *db;
  int rc;

  printf("Opening db at:%s\n", dbfile);
  rc = sqlite3_open_v2(dbfile, &db, sqlite_attr, NULL);
  printf("RC:%d\n", rc);
  if(rc) {
    log_error("Failed to open database file at: %s\n", dbfile);
    sqlite3_close(db);
    exit(1);
  }
  printf("Opened database connection.\n");

  return db;
}

/*
 * Close a database connection.
 */
void 
jfs_close_db(sqlite3 *db)
{
  sqlite3_close(db);
}

/*
 * Setup a sqlite prepared statement.
 */
static int
setup_stmt(sqlite3 *db, sqlite3_stmt **stmt, const char* query)
{
  const char* zTail;
  int rc;

  rc = sqlite3_prepare_v2(db, query, strlen(query), stmt, &zTail);
  if(rc != SQLITE_OK) {
    log_error("Sqlite prepare failed, query:%s, rc:%d\n",
			  query, rc);
    return rc;
  }

  return 0;
}

/*
 * Perform a query.
 */
int
jfs_query(struct jfs_db_op *db_op)
{
  sqlite3_stmt *stmt;
  int rc;

  rc = setup_stmt(db_op->db, &stmt, db_op->query);
  if(rc) {
	log_error("Setup statement failed for query=%s\n",
			  db_op->query);
	db_op->error = JFS_QUERY_FAILED;
    return rc;
  }

  db_op->stmt = stmt;
  rc = jfs_db_result(db_op);
  if(rc == JFS_QUERY_FAILED) {
	log_error("Get result failed for db_op->jfs_type=%d\n",
			  db_op->res_t);
  }

  return rc;
}
