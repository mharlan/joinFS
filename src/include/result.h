#ifndef JOINFS_RESULT_H
#define JOINFS_RESULT_H

#include "sqlitedb.h"

#define JFS_FILENAME_MAX 1024
#define JFS_KEY_MAX      1024
#define JFS_VALUE_MAX    1024

/*
 * Function to return the result of a query.
 * The result will be set based on the jfs_t.
 */
int jfs_db_result(struct jfs_db_op *db_op);

#endif
