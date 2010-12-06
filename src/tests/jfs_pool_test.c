/*
 * joinFS Unit Tests - Thread Pool, Query+Result Engine
 * Matthew Harlan <mharlan@gwmail.gwu.edu>
 */

#if !defined(_REENTRANT)
#define	_REENTRANT
#endif

#include "thr_pool.h"
#include "error_log.h"
#include "sqlitedb.h"
#include "jfs_list.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sqlite3.h>

static thr_pool_t *pool;

void *thrd_func(void *arg);

int main(void)
{
  pthread_t         pid;
  int               i;

  log_init();
  log_error("START OF LOG FOR JFS_POOL_TEST\n");
  printf("JFS_POOL_TEST: START\n");

  printf("Starting sqlite.");
  sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
  sqlite3_initialize();

  pool = jfs_pool_create(10, 20, 60, NULL, SQLITE_OPEN_READONLY);
  jfs_pool_wait(pool);

  printf("Read pool created.\n");

  for(i = 1; i < 10000; i++) {
    printf("Dispatching job #%d\n", i);
    pthread_create(&pid, NULL, thrd_func, (void *)i);
  }

  jfs_pool_wait(pool);
  jfs_pool_destroy(pool);

  sqlite3_shutdown();
  log_destroy();

  printf("Pool and log destroyed.\n");
 
  return 0;
}

void *thrd_func(void *arg)
{
  struct jfs_db_op *db_op;
  int q_val = (int)arg;
  
  db_op = jfs_db_op_create();
  if(!db_op) return 0;

  db_op->res_t = jfs_s_file;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "SELECT id FROM test_table WHERE name=\"%d\";",
		   (q_val % 30) + 1);
  
  printf("Query set:%s\n", db_op->query);
  printf("Performing job #%d and sleeping.\n", q_val);

  jfs_pool_queue(pool, db_op);
  jfs_db_op_wait(db_op);
  
  printf("Job #%d Waking Up\n", q_val);
  printf("Result size:%d\n", db_op->size);
  printf("Result inode:%d\n", db_op->result->inode);
  
  jfs_db_op_destroy(db_op);

  pthread_exit(0);
}
