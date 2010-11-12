#ifndef JOINFS_QF_H
#define JOINFS_QF_H

#include <sqlite3.h>

const unsigned char *qf_open(sqlite3 *db, const char *path);

#endif
