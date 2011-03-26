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

#ifndef JOINFS_JFS_SQLITEDB_H
#define JOINFS_JFS_SQLITEDB_H

#include "jfs_list.h"
#include "jfs_db_ops.h"

#include <pthread.h>
#include <sqlite3.h>

#define JFS_QUERY_SUCCESS -1
#define JFS_QUERY_FAILED  -2
#define JFS_QUERY_INC     512
#define JFS_QUERY_MAX     1000000  /* SQLITE_SQL_MAX_LENGTH */
#define JFS_SQL_RC_SCALE  100

/*
 * Structure for thread pool database operations.
 *
 * Errors are stored in size.
 */
struct jfs_db_op {
  sqlite3         *db;
  sqlite3_stmt    *stmt;
  enum jfs_db_ops  op;

  int              num_queries;
  int              done;
  int              rc;

  pthread_cond_t   cond;
  pthread_mutex_t  mut;

  char            *query;
  char           **multi_query;

  jfs_list_t      *result;
  size_t           buffer_size;
};

/*
 * Initialize the SQL database.
 */
void jfs_init_db(void);

/*
 * Creates a database operation.
 */
int jfs_db_op_create(struct jfs_db_op **op, enum jfs_db_ops jfs_op, const char *format, ...);


/*
 * Create a database op with pre-created query.
 */
int jfs_do_db_op_create(struct jfs_db_op **op, enum jfs_db_ops jfs_op, char *query);

/*
 * Create a multi-write transaction operation.
 */
int jfs_db_op_create_multi_op(struct jfs_db_op **op, int num_queries,...);

/*
 * Create a db query using a format string.
 */
int jfs_db_op_create_query(char **query, const char *format, ...);

/*
 * Destroy a database operation.
 */
void jfs_db_op_destroy(struct jfs_db_op *db_op);

/*
 * Performs a database operation that blocks while waiting
 * for the query result.
 *
 * Returns the size of the result or an error code if the
 * query failed.
 */
int jfs_db_op_wait(struct jfs_db_op *db_op);

/*
 * Create a jfsdb handle. The handle can not
 * be shared accross threads.
 */
int jfs_open_db(sqlite3 **db, int sqlite_attr);

/*
 * Close a jfsdb connection.
 */
void jfs_close_db(sqlite3 *db);

/*
 * Performs a query using the jfs_db_op struct.
 */
int jfs_query(struct jfs_db_op *db_op);

#endif
