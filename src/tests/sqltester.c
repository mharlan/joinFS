/*
  joinFS: SQLite Testing Module
  Matthew Harlan <mharlan@gwmail.gwu.edu>

  A test module for the sqlitedb files.
  gcc -o sqltester qf.c error_log.c sqlitedb.c sqltester.c  -Wall -W -O2 -Wl,-R/usr/local/lib -lsqlite3
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

#include "error_log.h"
#include "qf.h"
#include "sqlitedb.h"

/*
  Test driver for sqlite.
 */
int main(int argc, const char **argv)
{
  char *file = "/home/matt/joinFS/bootcamp/test.txt";
  const unsigned char *path;
  sqlite3 *db;
  int rc;

  log_init();
  log_error("ERROR LOG STARTS\n");
  db = start_db();

  rc = sqlite3_exec(db, "DROP TABLE IF EXISTS files;", 0,0,0);
  if(rc != SQLITE_OK) {
    printf("Drop table failed, rc:%d\n", rc);
  }

  rc = sqlite3_exec(db, "CREATE TABLE files (id INTEGER PRIMARY KEY, name TEXT, path TEXT);",
		    0,0,0);
  if(rc != SQLITE_OK) {
    printf("Create table failed, rc:%d\n", rc);
  }

  rc = sqlite3_exec(db, "INSERT INTO files VALUES(1, 'test.txt', '/home/matt/joinfs/bootcamp/jfs/.data/test.txt')",
		    0,0,0);
  if(rc != SQLITE_OK) {
    printf("Insert failed, rc:%d\n", rc);
  }

  printf("Calling qf_open on %s\n", file);
  path = qf_open(db, file);
  printf("Resulting path:%s\n", path);

  close_db(db);
  log_destroy();

  return 0;
}
