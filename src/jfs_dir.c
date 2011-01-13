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
static void safe_jfs_list_destroy(struct sglib_jfs_list_t_iterator *it, jfs_list_t *item);

int 
jfs_dir_mkdir(const char *path, const char *jfs_path, mode_t mode)
{
  struct jfs_db_op *db_op;

  int dirinode;
  int rc;

  rc = mkdir(jfs_path, mode);
  if(rc) {
	return -errno;
  }

  dirinode = jfs_util_get_inode(jfs_path);
  if(dirinode < 0) {
	return dirinode;
  }

  rc = jfs_db_op_create(&db_op);
  if(rc) {
	return rc;
  }

  db_op->op = jfs_write_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "INSERT OR ROLLBACK INTO directories VALUES(%d,\"%s\",0,0,0,0,NULL,NULL);",
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

  DIR *dp;
  int rc;

  printf("--jfs_dir_readdir called\n");

  dp = opendir(path);
  if(dp != NULL) {
	printf("--SYSTEM READDIR EXECUTING\n");

	while((de = readdir(dp)) != NULL) {
	  memset(&st, 0, sizeof(st));
	  st.st_ino = de->d_ino;
	  st.st_mode = de->d_type << 12;
	  
	  if(filler(buf, de->d_name, &st, 0) != 0) {
		closedir(dp);
		return -ENOMEM;
	  }
	}

	rc = closedir(dp);
	if(rc) {
	  return -errno;
	}
  }

  rc = jfs_dir_db_filler(path, buf, filler);
  if(rc) {
	return rc;
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

  printf("--jfs_dir_db_filler called for path:%s\n", path);

  has_subquery = 0;
  sub_inode = 0;
  filename = jfs_util_get_filename(path);

  rc = jfs_util_get_inode_and_mode(path, &dirinode, &mode);
  if(rc) {
	rc = jfs_path_cache_get_datapath(path, &datapath);
	if(rc) {
	  return -errno;
	}

	rc = jfs_util_get_inode_and_mode(datapath, &dirinode, &mode);
	if(rc) {
	  return rc;
	}
  }

  query = NULL;
  rc = jfs_dir_get_directory_info(dirinode, filename, &has_subquery, &sub_inode, &query);
  if(rc) {
	return rc;
  }

  if(query == NULL) {
	return 0;
  }

  rc = jfs_do_db_op_create(&db_op, query);
  if(rc) {
	return rc;
  }

  printf("---readdir fill query:%s\n", db_op->query);

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
	printf("---Adding filename item->filename:%s\n", item->filename);

	memset(&st, 0, sizeof(st));

	buffer_len = strlen(path) + strlen(item->filename) + 2;
	buffer = malloc(sizeof(*buffer) * buffer_len);
	if(!buffer) {
	  jfs_list_destroy(item, jfs_readdir_op);
	  jfs_db_op_destroy(db_op);
	  return -ENOMEM;
	}
	snprintf(buffer, buffer_len, "%s/%s", path, item->filename);

	printf("---Semantic path is:%s, has_subquery:%d\n", buffer, has_subquery);
	
	if(has_subquery) {
	  printf("---Dynamic folder results are folders.\n");

	  datapath_len = strlen(path) + strlen(".jfs_sub_query") + 2;
	  datapath = malloc(sizeof(*datapath) * datapath_len);
	  if(!datapath) {
		safe_jfs_list_destroy(&it, item);
		jfs_db_op_destroy(db_op);
		return -ENOMEM;
	  }
	  snprintf(datapath, datapath_len, "%s/%s", path, ".jfs_sub_query");

	  st.st_ino = dirinode;
	  st.st_mode = mode;
	}
	else {
	  printf("---Dynamic file datapath:%s.\n", item->datapath);

	  rc = stat(item->datapath, &st);
	  if(rc) {
		safe_jfs_list_destroy(&it, item);
		jfs_db_op_destroy(db_op);
		return rc;
	  }

	  datapath_len = strlen(item->datapath) + 1;
	  datapath = malloc(sizeof(*datapath) * datapath_len);
	  if(!datapath) {
		safe_jfs_list_destroy(&it, item);
		jfs_db_op_destroy(db_op);
		return -ENOMEM;
	  }
	  strncpy(datapath, item->datapath, datapath_len);

	  st.st_ino = item->inode;
	  st.st_mode = mode;
	}

	if(filler(buf, item->filename, &st, 0) != 0) {
	  safe_jfs_list_destroy(&it, item);
	  jfs_db_op_destroy(db_op);
	  return -ENOMEM;
	}

	printf("---Associated path:%s with datapath:%s in path cache.\n", buffer, datapath);

	rc = jfs_path_cache_add(buffer, datapath);
	if(rc) {
	  safe_jfs_list_destroy(&it, item);
	  jfs_db_op_destroy(db_op);
	  return rc;
	}

	if(item->datapath != NULL) {
	  free(item->datapath);
	}
	free(item->filename);
	free(item);

	free(buffer);
	free(datapath);
  }

  jfs_db_op_destroy(db_op);

  return 0;
}

static int
jfs_dir_get_directory_info(int dirinode, const char *filename, int *has_subquery, int *sub_inode, char **query)
{
  struct jfs_db_op *db_op;
  char *quote_sub_key;
  char *quote_filename;

  size_t query_len;
  size_t quote_sub_key_len;
  size_t quote_filename_len;

  int is_subquery;
  int uses_filename;
  int rc;

  printf("--jfs_dir_get_directory_info called\n");
  
  rc = jfs_db_op_create(&db_op);
  if(rc) {
	return rc;
  }

  db_op->op = jfs_directory_cache_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "SELECT has_subquery, is_subquery, uses_filename, sub_inode, sub_key, query FROM directories WHERE inode=%d;",
		   dirinode);

  printf("--DIRECTORY QUERY:%s\n", db_op->query);

  jfs_read_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
	jfs_db_op_destroy(db_op);
	return rc;
  }

  printf("--Checking results...\n");

  if(db_op->result == NULL) {
	db_op->rc = 1;
	jfs_db_op_destroy(db_op);
	return 0;
  }

  if(db_op->result->query == NULL) {
	printf("--STATIC DIRECTORY\n");

	jfs_db_op_destroy(db_op);
	return 0;
  }

  if(db_op->result->sub_key == NULL) {
	jfs_db_op_destroy(db_op);
	return -1;
  }

  printf("--DYNAMIC DIRECTORY\n");

  *has_subquery = db_op->result->has_subquery;
  is_subquery = db_op->result->is_subquery;
  uses_filename = db_op->result->uses_filename;
  *sub_inode = db_op->result->sub_inode;

  quote_sub_key_len = strlen(db_op->result->sub_key) + 3;
  query_len = strlen(db_op->result->query) + quote_sub_key_len + 3;

  quote_sub_key = malloc(sizeof(*quote_sub_key) * quote_sub_key_len);
  if(!quote_sub_key) {
	jfs_db_op_destroy(db_op);
	return -ENOMEM;
  }

  snprintf(quote_sub_key, quote_sub_key_len, "\"%s\"", db_op->result->sub_key);

  printf("---Building query using format string:%s Using sub_key:%s\n", db_op->result->query, db_op->result->sub_key);

  if(uses_filename) {
	quote_filename_len = strlen(filename) + 3;
	quote_filename = malloc(sizeof(*quote_filename) * quote_filename_len);
	if(!quote_filename) {
	  free(quote_sub_key);
	  jfs_db_op_destroy(db_op);
	  return -ENOMEM;
	}
	snprintf(quote_filename, quote_filename_len, "\"%s\"", filename);
	printf("---Using quote filename:%s\n", quote_filename);

	query_len += quote_filename_len;
	*query = malloc(sizeof(**query) * query_len);
	if(!*query) {
	  free(quote_sub_key);
	  free(quote_filename);
	  jfs_db_op_destroy(db_op);
	  return -ENOMEM;
	}

	snprintf(*query, query_len, db_op->result->query, quote_sub_key, quote_filename);
	free(quote_filename);
  }
  else {
	*query = malloc(sizeof(**query) * query_len);
	if(!*query) {
	  free(quote_sub_key);
	  jfs_db_op_destroy(db_op);
	  return -ENOMEM;
	}

	snprintf(*query, query_len, db_op->result->query, quote_sub_key);
  }
  free(quote_sub_key);

  jfs_db_op_destroy(db_op);

  return 0;
}

static void
safe_jfs_list_destroy(struct sglib_jfs_list_t_iterator *it, jfs_list_t *item)
{
  if(item->datapath != NULL) {
	free(item->datapath);
  }
  free(item->filename);
  free(item);

  item = sglib_jfs_list_t_it_next(it);
  for(;item != NULL; item = sglib_jfs_list_t_it_next(it)) {
	if(item->datapath != NULL) {
	  free(item->datapath);
	}
	free(item->filename);
	free(item);
  }
}
