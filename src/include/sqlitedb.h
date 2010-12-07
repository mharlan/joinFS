#ifndef JOINFS_SQLITEDB_H
#define JOINFS_SQLITEDB_H

#include "jfs_list.h"

#include <pthread.h>
#include <sqlite3.h>

#define JFS_QUERY_SUCCESS 1
#define JFS_QUERY_FAILED  2
#define JFS_QUERY_MAX     100000
//#define JFS_QUERY_MAX     SQLITE_MAX_SQL_LENGTH /* 1,000,000 bytes */

/*
 * Structure for thread pool database operations.
 *
 * Errors are stored in size.
 */
struct jfs_db_op {
  sqlite3          *db;
  sqlite3_stmt     *stmt;
  int               error;

  pthread_cond_t   cond;
  pthread_mutex_t  mut;

  char             query[JFS_QUERY_MAX];
  enum jfs_t       res_t;
  jfs_list_t      *result;
  int              size;
};

/*
 * Creates a database operation.
 */
struct jfs_db_op *jfs_db_op_create();

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
