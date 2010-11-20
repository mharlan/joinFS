#ifndef JOINFS_SQLITEDB_H
#define JOINFS_SQLITEDB_H

#include "result.h"
#include <sqlite3.h>

struct jfs_db_op {
  sqlite3 *db;
  char *query;
  jfs_t res_t;
};

/*
  Create a db handle. Called once per thread.
 */
sqlite3* open_jfs_db();
void close_jfs_db(sqlite3 *db);
void jfs_query(jfs_db_op *db_op);

#endif
