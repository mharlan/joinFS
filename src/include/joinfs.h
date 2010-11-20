#ifndef JOINFS_JOINFS_H
#define JOINFS_JOINFS_H

#include <sqlite3.h>

struct jfs_context {
  sqlite3 *db;
  char *rootdir;
};

#define JFS_DATA ((struct jfs_context *) fuse_get_context()->private_data)

#endif
