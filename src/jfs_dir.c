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
#include "jfs_path_cache.h"
#include "joinfs.h"

#include <fuse.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

static int jfs_dir_db_filler(const char *path, void *buf, fuse_fill_dir_t filler);
static int jfs_dir_get_directory_info(int dirinode, const char *filename, int *has_subquery, int *sub_inode, char **query);

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
		   "INSERT OR ROLLBACK INTO directories VALUES(%d,\"%s\",0,0,0,NULL,NULL);",
		   dirinode, path);

  jfs_write_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
	jfs_db_op_destroy(db_op);

	if(rc == SQLITE_CONSTRAINT) {
	  return -EEXIST;
	}

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
jfs_dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler)
{
  struct stat st;
  struct dirent *de;

  char *datapath;

  DIR *dp;
  int rc;

  dp = opendir(path);
  if(dp == NULL) {
	rc = jfs_path_cache_get_datapath(path, &datapath);
	if(rc) {
	  return -errno;
	}

    dp = opendir(datapath);
	if(dp == NULL) {
	  return -errno;
	}
  }
  else {
	while((de = readdir(dp)) != NULL) {
	  memset(&st, 0, sizeof(st));
	  st.st_ino = de->d_ino;
	  st.st_mode = de->d_type << 12;
	  
	  if(filler(buf, de->d_name, &st, 0) != 0) {
		closedir(dp);
		return -ENOMEM;
	  }
	}
  }

  rc = jfs_dir_db_filler(path, buf, filler);
  if(rc) {
	closedir(dp);
	return rc;
  }

  rc = closedir(dp);
  if(rc) {
	return -errno;
  }

  return 0;
}

static int
jfs_dir_db_filler(const char *path, void *buf, fuse_fill_dir_t filler)
{
  struct sglib_jfs_list_t_iterator it;
  struct jfs_db_op *db_op;
  struct stat st;
  jfs_list_t *item;

  char *filename;
  char *query;
  char *buffer;
  char *datapath;

  size_t buffer_len;
  size_t datapath_len;
  mode_t mode;

  int has_subquery;
  int sub_inode;
  int dirinode;
  int rc;

  filename = jfs_util_get_filename(path);

  rc = jfs_util_get_inode_and_mode(path, &dirinode, &mode);
  if(rc) {
	return rc;
  }

  query = NULL;
  rc = jfs_dir_get_directory_info(dirinode, filename, &has_subquery, &sub_inode, &query);
  if(rc) {
	return rc;
  }

  rc = jfs_do_db_op_create(&db_op, query);
  if(rc) {
	return rc;
  }

  db_op->op = jfs_readdir_op;
  jfs_read_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
	jfs_db_op_destroy(db_op);
	return rc;
  }

  if(db_op->result == NULL) {
	jfs_db_op_destroy(db_op);
	return 0;
  }

  for(item = sglib_jfs_list_t_it_init(&it, db_op->result); 
	  item != NULL; item = sglib_jfs_list_t_it_next(&it)) {
	memset(&st, 0, sizeof(st));

	buffer_len = strlen(path) + strlen(item->filename) + 2;
	buffer = malloc(sizeof(*buffer) * buffer_len);
	if(!buffer) {
	  jfs_list_destroy(item, jfs_readdir_op);
	  jfs_db_op_destroy(db_op);
	  return -ENOMEM;
	}
	snprintf(buffer, buffer_len, "%s/%s", path, item->filename);
	
	if(has_subquery) {
	  st.st_ino = sub_inode;
	  st.st_mode = mode;

	  datapath_len = strlen(path) + strlen(".jfs_sub_query") + 2;
	  datapath = malloc(sizeof(*datapath) * datapath_len);
	  if(!datapath) {
		jfs_list_destroy(item, jfs_readdir_op);
		jfs_db_op_destroy(db_op);
		return -ENOMEM;
	  }
	  snprintf(datapath, datapath_len, "%s\%s", path, ".jfs_sub_query");
	}
	else {
	  rc = stat(item->datapath, &st);
	  if(rc) {
		jfs_list_destroy(item, jfs_readdir_op);
		jfs_db_op_destroy(db_op);
		return rc;
	  }

	  datapath_len = strlen(item->datapath) + 1;
	  datapath = malloc(sizeof(*datapath) * datapath_len);
	  if(!datapath) {
		jfs_list_destroy(item, jfs_readdir_op);
		jfs_db_op_destroy(db_op);
		return -ENOMEM;
	  }
	  strncpy(datapath, item->datapath, datapath_len);
	}

	if(filler(buf, item->filename, &st, 0) != 0) {
	  jfs_list_destroy(item, jfs_readdir_op);
	  jfs_db_op_destroy(db_op);
	  return -ENOMEM;
	}

	rc = jfs_path_cache_add(buffer, datapath);
	if(rc) {
	  jfs_list_destroy(item, jfs_readdir_op);
	  jfs_db_op_destroy(db_op);
	  return rc;
	}

	if(item->datapath != NULL) {
	  free(item->datapath);
	}
	free(item->filename);
	free(item);
  }

  return 0;
}

static int
jfs_dir_get_directory_info(int dirinode, const char *filename, int *has_subquery, int *sub_inode, char **query)
{
  struct jfs_db_op *db_op;
  size_t query_len;

  int is_subquery;
  int rc;
  
  rc = jfs_db_op_create(&db_op);
  if(rc) {
	return rc;
  }

  db_op->op = jfs_directory_cache_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "SELECT has_subquery, is_subquery, sub_inode, sub_key, query FROM directories WHERE inode=%d;",
		   dirinode);

  jfs_read_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
	jfs_db_op_destroy(db_op);
	return rc;
  }

  if(db_op->result == NULL) {
	jfs_db_op_destroy(db_op);
	return -1;
  }

  if(db_op->result->query == NULL) {
	jfs_db_op_destroy(db_op);
	return -1;
  }

  *has_subquery = db_op->result->has_subquery;
  is_subquery = db_op->result->is_subquery;
  *sub_inode = db_op->result->sub_inode;

  if(is_subquery) {
	query_len = strlen(db_op->result->query) + strlen(db_op->result->sub_key) + 1;
  }
  else {
	query_len = strlen(db_op->result->query) + 1;
  }

  *query = malloc(sizeof(**query) * query_len);
  if(!*query) {
	jfs_db_op_destroy(db_op);
	return -ENOMEM;
  }

  if(is_subquery) {
	snprintf(*query, query_len, db_op->result->query, db_op->result->sub_key, filename);
  }
  else {
	strncpy(*query, db_op->result->query, query_len);
  }
  jfs_db_op_destroy(db_op);

  return 0;
}
