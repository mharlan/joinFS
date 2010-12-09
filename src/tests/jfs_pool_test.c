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

#define TEST_SIZE 1000

static thr_pool_t *read_pool;
static thr_pool_t *write_pool;

void *read_thrd_func(void *arg);
void *write_thrd_func(void *arg);

int main(void)
{
  struct sched_param param;
  pthread_attr_t wattr;

  pthread_t         pid;
  int               i;

  log_init();
  log_error("START OF LOG FOR JFS_POOL_TEST\n");
  printf("JFS_POOL_TEST: START\n");

  printf("Starting sqlite in multithreaded mode.\n");
  sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
  sqlite3_initialize();

  i = sqlite3_threadsafe();
  if(!i) {
	printf("SQLITE is not in thread safe mode. Abort.\n");
	exit(EXIT_FAILURE);
  }
  else {
	printf("SQLITE is THREAD SAFE.\n");
  }

  read_pool = jfs_pool_create(20, 40, 60, NULL, SQLITE_OPEN_READONLY);

  printf("Setting higher thrad priority for writes.\n");

  pthread_attr_init(&wattr);
  pthread_attr_setschedpolicy(&wattr, SCHED_RR);
  pthread_attr_getschedparam(&wattr, &param);
  param.sched_priority = 1;
  pthread_attr_setschedparam(&wattr, &param);

  write_pool = jfs_pool_create(1, 1, 60, &wattr, SQLITE_OPEN_READWRITE);

  printf("Read pool created.\n");

  for(i = 1; i < TEST_SIZE; i++) {
    printf("Dispatching job #%d\n", i);
    pthread_create(&pid, NULL, read_thrd_func, (void *)i);
	if((i > 30) && ((i % 10) == 1)) {
	  pthread_create(&pid, NULL, write_thrd_func, (void *)i);
	}
  }

  jfs_pool_wait(read_pool);
  jfs_pool_destroy(read_pool);

  printf("Read pool destroyed.\n");

  jfs_pool_wait(write_pool);
  jfs_pool_destroy(write_pool);

  printf("Write pool destroyed.\n");

  sqlite3_shutdown();
  log_destroy();

  printf("Pool and log destroyed.\n");
 
  return 0;
}

void *read_thrd_func(void *arg)
{
  struct jfs_db_op *db_op;
  int q_val = (int)arg;
  
  db_op = jfs_db_op_create();
  if(!db_op) return 0;

  db_op->res_t = jfs_s_file;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "SELECT id FROM test_table WHERE name=\"%d\";",
		   (q_val % 30) + 1);
  
  printf("--READ--Query set:%s\n", db_op->query);
  printf("--READ--Performing job #%d and sleeping.\n", q_val);

  jfs_pool_queue(read_pool, db_op);
  jfs_db_op_wait(db_op);

  if(db_op->error == JFS_QUERY_SUCCESS) {
	printf("--READ--Job #%d Waking Up\n", q_val);
	printf("--READ--Result size:%d\n", db_op->size);
	printf("--READ--Result inode:%d\n", db_op->result->inode);
  }
  else {
	printf("Read query failed for job %d\n", q_val);
  }
  
  jfs_db_op_destroy(db_op);

  pthread_exit(0);
}

void *write_thrd_func(void *arg)
{
  struct jfs_db_op *db_op;
  int q_val = (int)arg;
  
  db_op = jfs_db_op_create();
  if(!db_op) return 0;

  db_op->res_t = jfs_write_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "INSERT OR ROLLBACK INTO test_table VALUES(%d,\"%d\");",
		   q_val, q_val);
  
  printf("--WRITE--Query set:%s\n", db_op->query);
  printf("--WRITE--Performing job #-%d and sleeping.\n", q_val);

  jfs_pool_queue(write_pool, db_op);
  jfs_db_op_wait(db_op);
  
  printf("--WRITE--Job #-%d Waking Up\n", q_val);
  if(db_op->error == JFS_QUERY_SUCCESS) {
	printf("Write was successful.\n");
  }
  else {
	printf("Write query failed for job %d.\n", q_val);
  }
  
  jfs_db_op_destroy(db_op);

  pthread_exit(0);
}
