#ifndef JOINFS_JOINFS_H
#define JOINFS_JOINFS_H

#include "sqlitedb.h"

#include <sqlite3.h>

struct jfs_context {
  sqlite3 *db;
  char *rootdir;
};

#define JFS_DATA ((struct jfs_context *) fuse_get_context()->private_data)

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
