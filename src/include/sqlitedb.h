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
#define JFS_QUERY_MAX     100000
//#define JFS_QUERY_MAX     SQLITE_MAX_SQL_LENGTH /* 1,000,000 bytes */

/*
 * Structure for thread pool database operations.
 *
 * Errors are stored in size.
 */
struct jfs_db_op {
  sqlite3         *db;
  sqlite3_stmt    *stmt;
  enum jfs_db_ops  op;
  int              done;
  int              rc;

  pthread_cond_t   cond;
  pthread_mutex_t  mut;

  char            *query;
  jfs_list_t      *result;
  size_t           buffer_size;
};

/*
 * Creates a database operation.
 *
 * Returns a query buffer of size JFS_QUERY_MAX
 */
int jfs_db_op_create(struct jfs_db_op **op);

/*
 * Create a database op with a specified query.
 */
int jfs_do_db_op_create(struct jfs_db_op **op, char *query);

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
sqlite3 *jfs_open_db();

/*
 * Close a jfsdb connection.
 */
void jfs_close_db(sqlite3 *db);

/*
 * Performs a query using the jfs_db_op struct.
 */
int jfs_query(struct jfs_db_op *db_op);

#endif
