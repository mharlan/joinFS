/*
  joinFS: Sqlite database component
  Matthew Harlan <mharlan@gwmail.gwu.edu>

  @version October 4th, 2010

  Uses a query to match a query file to a folder in the VFS.
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include "error_log.h"
#include "sqlitedb.h"

#define DB_FILE "/home/matt/joinFS/bootcamp/joinfs.db"
#define ERR_MAX 256

static int setup_stmt(sqlite3 *db, sqlite3_stmt **stmt, const char* query);

/*
  Open a database connection.
 */
sqlite3 *start_db()
{
  sqlite3 *db;
  int rc;

  rc = sqlite3_open(DB_FILE, &db);
  if(rc) {
    log_error("Failed to open database file at: %s\n", DB_FILE);
    sqlite3_close(db);
    exit(1);
  }

  return db;
}

/*
  Close a database connection.
 */
void close_db(sqlite3 *db)
{
  sqlite3_close(db);
}

/*
  Setup a sqlite prepared statement.
 */
static int setup_stmt(sqlite3 *db, sqlite3_stmt **stmt, const char* query)
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
  Get the path from a query.
 */
const unsigned char* path_query(sqlite3 *db, const char* query)
{
  sqlite3_stmt *stmt;
  unsigned char* path = 0;
  int rc;

  rc = setup_stmt(db, &stmt, query);
  if(rc) {
    return 0;
  }

  rc = sqlite3_step(stmt);

  if(rc == SQLITE_ROW) {
    memcpy(path, sqlite3_column_text(stmt, 0), sqlite3_column_bytes(stmt, 0));
  }
  else {
    log_error("No file path returned from query:%s, rc:%d\n",
	      query, rc);
  }

  rc = sqlite3_finalize(stmt);
  if(rc != SQLITE_OK) {
    log_error("Finalizing failed, rc:%d\n", rc);
  }

  return path;
}
