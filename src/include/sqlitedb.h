#ifndef JOINFS_SQLITEDB_H
#define JOINFS_SQLITEDB_H

#include "jfs_list.h"

#include <pthread.h>
#include <sqlite3.h>

#define QUERY_MAX 1024

/*
 * Structure for thread pool database operations.
 */
struct jfs_db_op {
  sqlite3          *db;
  sqlite3_stmt     *stmt;

  pthread_cond_t   cond;
  pthread_mutex_t  mut;

  char             query[QUERY_MAX];
  enum jfs_t       res_t;
  jfs_list_t      *result;
  int              size;
};

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
