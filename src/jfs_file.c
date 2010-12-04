/*
 * joinFS - File Module
 * Matthew Harlan <mharlan@gwmail.gwu.edu>
 *
 * 30 % Demo
 */

#include "jfs_file.h"
#include "thr_pool.h"
#include "sqlitedb.h"

#include <fuse.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int get_inode(int query_inode);

int jfs_file_open(const char *path, struct fuse_file_info *fi)
{

}

int jfs_file_read(const char *path, char *buf, size_t size, off_t offset,
				  struct fuse_file_info *fi)
{

}

int jfs_file_write(const char *path, const char *buf, size_t size,
				   off_t offset, struct fuse_file_info *fi)
{

}

int jfs_file_truncate(const char *path, off_t size)
{

}

int jfs_file_access(const char *path, int mask)
{

}

static int get_inode(int q_inode)
{
  struct jfs_db_op *db_op;

  if(jfs_file_cache_has(q_inode)) {
	return jfs_file_cache_value(q_inode);
  }
  else {
	db_op = jfs_db_op_create();
	db_op->res_t = jfs_s_file;
	snprintf(db_op->query, QUERY_MAX,
			 "SELECT inode FROM symlinks WHERE linknode=\"%d\";",
			 q_inode);	
  }
}
