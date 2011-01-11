/********************************************************************
 * Copyright 2010, 2011 Matthew Harlan <mharlan@gwmail.gwu.edu>
 *
 * This file is part of joinFS.
 *	 
 * JoinFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * JoinFS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with joinFS.  If not, see <http://www.gnu.org/licenses/>.
 ********************************************************************/

#ifndef JOINFS_JOINFS_H
#define JOINFS_JOINFS_H

#include "sqlitedb.h"
#include "thr_pool.h"

#include <stdio.h>
#include <limits.h>

struct jfs_context {
  FILE *logfile;

  char *querypath;
  char *datapath;
  char *mountpath;

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
