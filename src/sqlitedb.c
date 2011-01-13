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

#include "jfs_list.h"
#include "error_log.h"
#include "sqlitedb.h"
#include "result.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sqlite3.h>
#include <pthread.h>

#define JFSDB   "/home/joinfs/git/joinFS/demo/joinfs.db"
#define ERR_MAX 256

//does not need to be public, prepares queries
static int setup_stmt(sqlite3 *db, sqlite3_stmt **stmt, const char* query);

/*
 * Creates a database operation.
 */
int
jfs_db_op_create(struct jfs_db_op **op)
{
  char *query;

  query = malloc(sizeof(*query) * JFS_QUERY_MAX);

  return jfs_do_db_op_create(op, query);
}

/*
 * Creates a database operation.
 */
int
jfs_do_db_op_create(struct jfs_db_op **op, char *query)
{
  struct jfs_db_op *db_op;

  db_op = malloc(sizeof(*db_op));
  if(!db_op) {
	log_error("Failed to allocate memory for db_op.\n");
	free(query);
	return -ENOMEM;
  }

  db_op->query = query;
  db_op->db = NULL;
  db_op->stmt = NULL;
  db_op->result = NULL;
  db_op->done = 0;
  db_op->rc = 0;

  pthread_cond_init(&db_op->cond, NULL);
  pthread_mutex_init(&db_op->mut, NULL);

  *op = db_op;

  return 0;
}

/*
 * Destroy a database operation.
 */
void
jfs_db_op_destroy(struct jfs_db_op *db_op)
{
  printf("--PERFORMING DB_OP CLEANUP\n");

  free(db_op->query);

  if(!db_op->rc) {
	switch(db_op->op) {
	case(jfs_file_cache_op):
	  free(db_op->result->datapath);
	  free(db_op->result);
	  break;
	case(jfs_key_op):
	  free(db_op->result);
	  break;
	case(jfs_attr_op):
	  free(db_op->result->value);
	  free(db_op->result);
	  break;
	case(jfs_directory_cache_op):
	  if(db_op->result->query != NULL) {
		free(db_op->result->query);
	  }
	  
	  if(db_op->result->sub_key != NULL) {
		free(db_op->result->sub_key);
	  }
	  free(db_op->result);
	  break;
	case(jfs_listattr_op):
	  jfs_list_destroy(db_op->result, jfs_listattr_op);
	  break;
	case(jfs_readdir_op):
	  jfs_list_destroy(db_op->result, jfs_readdir_op);
	  break;
	case(jfs_dynamic_file_op):
	  free(db_op->result);
	  break;
	default:
	  break;
	}
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
  while(!db_op->done) {
	pthread_cond_wait(&db_op->cond, &db_op->mut);
  }
  pthread_mutex_unlock(&db_op->mut);

  printf("--QUERY FINISHED, RC:%d\n", db_op->rc);

  return db_op->rc;
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

  printf("--JFS QUERY STARTED--\n");

  rc = setup_stmt(db_op->db, &stmt, db_op->query);
  if(rc) {
	log_error("Setup statement failed for query=%s\n",
			  db_op->query);

    return rc;
  }

  printf("--QUERY STATEMENT IS READY--\n");

  db_op->stmt = stmt;
  rc = jfs_db_result(db_op);
  if(rc) {
	log_error("Query:%s failed for db_op=%d\n", db_op->query, db_op->op);
	return rc;
  }

  printf("--QUERY RESULT IS READY--\n");

  return 0;
}
