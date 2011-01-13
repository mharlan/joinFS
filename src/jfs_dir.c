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

#include "error_log.h"
#include "jfs_dir.h"
#include "sqlitedb.h"
#include "jfs_util.h"
#include "joinfs.h"

#include <fuse.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

int 
jfs_dir_mkdir(const char *path, mode_t mode)
{
  struct jfs_db_op *db_op;
  int dirinode;
  int rc;

  rc = mkdir(path, mode);
  if(rc) {
	return -errno;
  }

  dirinode = jfs_util_get_inode(path);
  if(dirinode < 0) {
	return dirinode;
  }

  rc = jfs_db_op_create(&db_op);
  if(rc) {
	return rc;
  }

  db_op->op = jfs_write_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "INSERT OR ROLLBACK INTO directories VALUES(%d, 0, NULL);",
		   dirinode);

  jfs_write_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
	jfs_db_op_destroy(db_op);
	return rc;
  }
  jfs_db_op_destroy(db_op);

  return 0;
}

int 
jfs_dir_rmdir(const char *path)
{
  struct jfs_db_op *db_op;
  int dirinode;
  int rc;

  dirinode = jfs_util_get_inode(path);
  if(dirinode < 0) {
	return dirinode;
  }

  rc = jfs_db_op_create(&db_op);
  if(rc) {
	return rc;
  }

  db_op->op = jfs_write_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "DELETE FROM directories WHERE inode=%d;",
		   dirinode);

  jfs_write_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
	jfs_db_op_destroy(db_op);
	return rc;
  }
  jfs_db_op_destroy(db_op);

  rc = rmdir(path);
  if(rc) {
	return -errno;
  }

  return 0;
}

int 
jfs_dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler, struct fuse_file_info *fi)
{
  struct dirent *de;
  DIR *dp;
  int rc;

  dp = opendir(path);
  if(dp == NULL) {
    return -errno;
  }
  
  while((de = readdir(dp)) != NULL) {
    struct stat st;
    memset(&st, 0, sizeof(st));
    st.st_ino = de->d_ino;
    st.st_mode = de->d_type << 12;

    if(filler(buf, de->d_name, &st, 0)) {
	  break;
	}
  }

  rc = closedir(dp);
  if(rc) {
	return -errno;
  }

  return 0;
}
