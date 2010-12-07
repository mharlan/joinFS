#ifndef JOINFS_JOINFS_H
#define JOINFS_JOINFS_H

#include "sqlitedb.h"
#include "thr_pool.h"

#include <stdio.h>
#include <limits.h>

struct jfs_context {
  FILE *logfile;

  char querypath[PATH_MAX];
  char datapath[PATH_MAX];
  char mountpath[PATH_MAX];

  int querypath_len;
  int datapath_len;
  int mountpath_len;

  thr_pool_t *read_pool;
  thr_pool_t *write_pool;
};

#define JFS_CONTEXT ((struct jfs_context *) fuse_get_context()->private_data)

/*
 * Queue a database read operation.
 *
 * The caller should call jfs_db_op_wait to wait
 * for the queued result to finish.
 */
int jfs_read_pool_queue(struct jfs_db_op *db_op);

/*
 * Queue a database write operation.
 *
 * The caller should call jfs_db_op_wait to wait
 * for the queued result to finish.
 */
int jfs_write_pool_queue(struct jfs_db_op *db_op);

#endif
