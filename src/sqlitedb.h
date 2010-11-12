#ifndef JOINFS_SQLITEDB_H
#define JOINFS_SQLITEDB_H

#include <sqlite3.h>

/*
  Create a db handle. Called once per thread.
 */
sqlite3 *start_db();
void close_db(sqlite3 *db);
const unsigned char* path_query(sqlite3 *db, const char* query);


#endif
