/*
  joinFS: Sqlite database component
  Matthew Harlan <mharlan@gwmail.gwu.edu>

  @version October 4th, 2010

  Uses a query to match a query file to a folder in the VFS.
*/
#include <stdio.h>
#include <sqlite3.h>

#include "error_log.h"
#include "sqlitedb.h"
#include "qf.h"

#define QUERY_MAX 4096

const unsigned char *qf_open(sqlite3 *db, const char *path)
{
  FILE *qf;
  const unsigned char *vfs_path;
  char* res;
  char query[QUERY_MAX];
 
  printf("fasdafds");
  qf = fopen("test.txt", "r+");
  if(!qf) {
    log_error("Failed to open query file at, path:%s\n", path);
    fclose(qf);
    return 0;
  }

  printf("yay");

  //using fgets for now, will use fread in real version
  res = fgets(&query[0], QUERY_MAX, qf);
  if(res != query) {
    log_error("could not extract query, query:%s\n", &query[0]);
    fclose(qf);
    return 0;
  }

  log_error("query:%s\n", query);
  vfs_path = path_query(db, query);

  fclose(qf);
  return vfs_path;;
}
