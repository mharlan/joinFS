#ifndef JOINFS_JFS_SQLITEDB_H
#define JOINFS_JFS_SQLITEDB_H

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
#include "jfs_db_ops.h"

#include <pthread.h>
#include <sqlite3.h>

#define JFS_SQL_SUCCESS     0
#define JFS_SQL_ERROR      -100
#define JFS_SQL_NOMEM      -700
#define JFS_SQL_FULL       -1300
#define JFS_SQL_EMPTY      -1600
#define JFS_SQL_TOOBIG     -1800
#define JFS_SQL_CONSTRAINT -1900
#define JFS_SQL_CANTOPEN   -1400
#define JFS_SQL_BUSY       -500

#define JFS_QUERY_INC     512
#define JFS_QUERY_MAX     1000000  /* SQLITE_SQL_MAX_LENGTH */
#define JFS_SQL_RC_SCALE  100

/*!
 * Structure for thread pool database operations.
 *
 * Errors are stored in rc.
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

/*!
 * Initialize the SQL database.
 */
void jfs_init_db(void);

/*!
 * Creates a database operation.
 * \param op The returned database operation.
 * \param jfs_op The type of database operation.
 * \param format The query format string.
 * \param ... The format variables.
 * \return Error code or 0.
 */
int jfs_db_op_create(struct jfs_db_op **op, enum jfs_db_ops jfs_op, const char *format, ...);


/*!
 * Create a database op with pre-created query.
 * \param op The returned database operation.
 * \param jfs_op The type of database operation.
 * \param query The query to be performed.
 * \return Error code or 0.
 */
int jfs_do_db_op_create(struct jfs_db_op **op, enum jfs_db_ops jfs_op, char *query);

/*!
 * Create a multi-write transaction operation.
 * \param op The returned database operation.
 * \param jfs_op The type of database operation.
 * \param num_queries The number of queries.
 * \param ... The format queries.
 * \return Error code or 0.
 */
int jfs_db_op_create_multi_op(struct jfs_db_op **op, int num_queries,...);

/*!
 * Create a db query using a format string.
 * \param query The returned query.
 * \param format The query format string.
 * \param ... The format variables.
 * \return Error code or 0.
 */
int jfs_db_op_create_query(char **query, const char *format, ...);

/*!
 * Destroy a database operation.
 * \param db_op The database operation.
 */
void jfs_db_op_destroy(struct jfs_db_op *db_op);

/*!
 * Performs a database operation that blocks while waiting
 * for the query result.
 * \param db_op The database operation.
 * \return Error code or 0.
 */
int jfs_db_op_wait(struct jfs_db_op *db_op);

/*!
 * Create a database connection handle. The handle can not
 * be shared accross threads.
 * \param db The new database handle.
 * \param sqlite_attr Sqlite attributes.
 * \return Error code or 0.
 */
int jfs_open_db(sqlite3 **db, int sqlite_attr);

/*!
 * Close a database connection.
 * \param db The new database handle.
 */
void jfs_close_db(sqlite3 *db);

/*!
 * Performs a query using the jfs_db_op struct.
 * \param db_op The database operation.
 * \result Error code or 0.
 */
int jfs_query(struct jfs_db_op *db_op);

#endif
