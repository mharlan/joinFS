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

static thr_pool_t *pool;

void *thrd_func(void *arg);

int main(void)
{
  pthread_t         pid;
  int               i;

  log_init();
  log_error("START OF LOG FOR JFS_POOL_TEST\n");
  printf("JFS_POOL_TEST: START\n");

  pool = jfs_pool_create(10, 20, 60, NULL);

  printf("Read pool created.\n");

  for(i = 1; i < 30; i++) {
	pthread_create(&pid, NULL, thrd_func, (void *)&i);
  }

  jfs_pool_wait(pool);
  jfs_pool_destroy(pool);

  log_destroy();

  printf("Pool and log destroyed.\n");
 
  return 0;
}

void *thrd_func(void *arg)
{
  struct jfs_db_op *db_op;
  int *q_val = (int *)arg;

  db_op = malloc(sizeof(*db_op));
  if(!db_op) {
	printf("Failed to allocate db_op.");
	return 0;
  }
  
  memset(db_op->query, 0, QUERY_MAX);
  snprintf(db_op->query, QUERY_MAX,
		   "SELECT id FROM test_table WHERE name=\"%d\";",
		   *q_val);
  
  printf("Query set:%s\n", db_op->query);
  
  db_op->db = NULL;
  db_op->stmt = NULL;
  db_op->result = NULL;
  db_op->size = 0;
  db_op->res_t = jfs_s_file;
  
  pthread_cond_init(&db_op->cond, NULL);
  pthread_mutex_init(&db_op->mut, NULL);
  
  jfs_pool_queue(pool, db_op);
  
  printf("Result size:%d\n", db_op->size);
  printf("Dispatched job:%d, sleeping\n", *q_val);

  pthread_mutex_lock(&db_op->mut);
  while(db_op->size < 1) {
	pthread_cond_wait(&db_op->cond, &db_op->mut);
  }
  pthread_mutex_unlock(&db_op->mut);
  
  printf("Thread Waking Up\n");
  printf("Result size:%d\n", db_op->size);
  printf("Result inode:%d\n", db_op->result->inode);
  
  free(db_op->result);
  free(db_op);

  pthread_exit(0);
}
