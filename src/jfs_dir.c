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
#include "jfs_dynamic_dir.h"
#include "sqlitedb.h"
#include "jfs_util.h"
#include "jfs_meta.h"
#include "jfs_dynamic_paths.h"
#include "joinfs.h"

#include <fuse.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <attr/xattr.h>

static int jfs_dir_is_dynamic(const char *path);
static int jfs_dir_db_filler(const char *path, void *buf, fuse_fill_dir_t filler);
static int jfs_dir_get_directory_info(const char *path, int *has_subquery, int *sub_inode, char **query);
static void safe_jfs_list_destroy(struct sglib_jfs_list_t_iterator *it, jfs_list_t *item);

int 
jfs_dir_mkdir(const char *path, mode_t mode)
{
  struct jfs_db_op *db_op;
  char *filename;
  int inode;
  int rc;

  rc = mkdir(path, mode);
  if(rc) {
	return -errno;
  }

  filename = jfs_util_get_filename(path);
  inode = jfs_util_get_inode(path);
  if(inode < 1) {
    return inode;
  }

  rc = jfs_db_op_create(&db_op);
  if(rc) {
	return rc;
  }

  db_op->op = jfs_write_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "INSERT OR ROLLBACK INTO files VALUES(%d,\"%s\",\"%s\");",
		   inode, path, filename);

  jfs_write_pool_queue(db_op);

  rc = jfs_db_op_wait(db_op);
  if(rc) {
	jfs_db_op_destroy(db_op);
	return rc;
  }
  jfs_db_op_destroy(db_op);

  jfs_meta_setxattr(path, JFS_DIR_IS_DYNAMIC, JFS_DIR_XATTR_FALSE, 1, XATTR_CREATE);

  return 0;
}

int 
jfs_dir_rmdir(const char *path)
{
  struct jfs_db_op *db_op;
  int inode;
  int rc;

  inode = jfs_util_get_inode(path);
  if(inode < 0) {
	return inode;
  }

  rc = rmdir(path);
  if(rc) {
	return -errno;
  }

  rc = jfs_db_op_create(&db_op);
  if(rc) {
	return rc;
  }

  db_op->op = jfs_write_op;
  snprintf(db_op->query, JFS_QUERY_MAX,
		   "DELETE FROM files WHERE inode=%d;",
		   inode);

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
jfs_dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler)
{
  struct stat st;
  struct dirent *de;

  DIR *dp;
  int rc;

  log_error("--jfs_dir_readdir called\n");

  dp = opendir(path);
  if(dp != NULL) {
	log_error("--SYSTEM READDIR EXECUTING\n");

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

  if(jfs_dir_is_dynamic(path)) {
    rc = jfs_dir_db_filler(path, buf, filler);
    if(rc) {
      return rc;
    }
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

  char *query;
  char *buffer;
  char *datapath;

  size_t buffer_len;
  size_t datapath_len;
  mode_t mode;

  int has_subquery;
  int sub_inode;
  int dirinode;
  int inode;
  int mask;
  int rc;

  log_error("--jfs_dir_db_filler called for path:%s\n", path);

  has_subquery = 0;
  sub_inode = 0;
  mask = R_OK | F_OK; 

  rc = jfs_util_get_inode_and_mode(path, &dirinode, &mode);
  if(rc) {
    return rc;
  }

  query = NULL;
  rc = jfs_dir_get_directory_info(path, &has_subquery, &sub_inode, &query);
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

  log_error("---readdir fill query:%s\n", db_op->query);

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

	log_error("---Semantic path is:%s, has_subquery:%d\n", buffer, has_subquery);
	
	if(has_subquery) {
	  log_error("---Dynamic folder results are folders.\n");

	  datapath_len = strlen(path) + strlen(".jfs_sub_query") + 2;
	  datapath = malloc(sizeof(*datapath) * datapath_len);
	  if(!datapath) {
        free(buffer);
		safe_jfs_list_destroy(&it, item);
		jfs_db_op_destroy(db_op);
		return -ENOMEM;
	  }
	  snprintf(datapath, datapath_len, "%s/%s", path, ".jfs_sub_query");

	  st.st_ino = dirinode;
	  st.st_mode = mode;
	}
	else {
	  rc = jfs_util_get_inode_and_mode(item->datapath, &inode, &mode);
	  if(rc) {
        free(buffer);
		safe_jfs_list_destroy(&it, item);
		jfs_db_op_destroy(db_op);
		return rc;
	  }

	  datapath_len = strlen(item->datapath) + 1;
	  datapath = malloc(sizeof(*datapath) * datapath_len);
	  if(!datapath) {
        free(buffer);
		safe_jfs_list_destroy(&it, item);
		jfs_db_op_destroy(db_op);
		return -ENOMEM;
	  }
	  strncpy(datapath, item->datapath, datapath_len);

	  st.st_ino = inode;
	  st.st_mode = mode;
	}

    //check access first
    if(access(datapath, mask) == 0) {
      printf("---Associated path:%s with datapath:%s in path cache.\n", buffer, datapath);

      //add for display
      if(filler(buf, item->filename, &st, 0) != 0) {
        safe_jfs_list_destroy(&it, item);
        free(buffer);
        free(datapath);
        jfs_db_op_destroy(db_op);
        return -ENOMEM;
      }

      //dynamic directory path
      if(has_subquery) {
        rc = jfs_dynamic_hierarchy_add_folder(buffer, datapath, st.st_ino);
      }
      //dynamic file path
      else {
        rc = jfs_dynamic_hierarchy_add_file(buffer, datapath, st.st_ino);
      }

      if(rc) {
        free(buffer);
        free(datapath);
        safe_jfs_list_destroy(&it, item);
        jfs_db_op_destroy(db_op);
        return rc;
      }
    }

	if(item->datapath != NULL) {
	  free(item->datapath);
	}
	free(item->filename);
	free(item);
    free(buffer);
  }

  jfs_db_op_destroy(db_op);

  return 0;
}

static int
jfs_dir_is_dynamic(const char *path)
{
  char *is_dynamic;
  int rc;
  
  rc = jfs_meta_do_getxattr(path, JFS_DIR_IS_DYNAMIC, &is_dynamic);
  if(rc) {
    return 0;
  }

  if(strcmp(is_dynamic, JFS_DIR_XATTR_TRUE) == 0) {
    return 1;
  }

  return 0;
}

static int
jfs_dir_get_directory_info(const char *path, int *has_subquery, int *sub_inode, char **query)
{
  char *filename;
  char *quote_sub_key;
  char *quote_filename;

  char *xattr_has_subquery;
  char *xattr_is_subquery;
  char *xattr_path_items;
  char *xattr_sub_inode;
  char *xattr_sub_key;
  char *xattr_query;

  size_t query_len;
  size_t quote_sub_key_len;
  size_t quote_filename_len;

  int is_subquery;
  int path_items;
  int rc;

  log_error("--jfs_dir_get_directory_info called, is a dynamic directory\n");

  filename = jfs_util_get_filename(path);

  //get the xattrs for dynamic dirs
  rc = jfs_meta_do_getxattr(path, JFS_DIR_HAS_SUBQUERY, &xattr_has_subquery);
  if(rc) {
    return rc;
  }

  log_error("-----xattr_has_subquery:%s\n", xattr_has_subquery);

  rc = jfs_meta_do_getxattr(path, JFS_DIR_IS_SUBQUERY, &xattr_is_subquery);
  if(rc) {
    return rc;
  }

  log_error("-----xattr_is_subquery:%s\n", xattr_is_subquery);

  rc = jfs_meta_do_getxattr(path, JFS_DIR_PATH_ITEMS, &xattr_path_items);
  if(rc) {
    return rc;
  }

  log_error("-----xattr_path_items:%s\n", xattr_path_items);

  rc = jfs_meta_do_getxattr(path, JFS_DIR_SUB_INODE, &xattr_sub_inode);
  if(rc) {
    return rc;
  }

  log_error("-----xattr_sub_inode:%s\n", xattr_sub_inode);

  rc = jfs_meta_do_getxattr(path, JFS_DIR_SUB_KEY, &xattr_sub_key);
  if(rc) {
    return rc;
  }

  log_error("-----xattr_sub_key:%s\n", xattr_sub_key);

  rc = jfs_meta_do_getxattr(path, JFS_DIR_QUERY, &xattr_query);
  if(rc) {
    return rc;
  }

  log_error("-----xattr_query:%s\n", xattr_query);

  *has_subquery = strcmp(xattr_has_subquery, JFS_DIR_XATTR_FALSE);
  is_subquery = strcmp(xattr_is_subquery, JFS_DIR_XATTR_FALSE);
  path_items = atoi(xattr_path_items);
  *sub_inode = atoi(xattr_sub_inode);

  quote_sub_key_len = strlen(xattr_sub_key) + 3;
  query_len = strlen(xattr_query) + quote_sub_key_len + 3;

  log_error("---Building dynamic directory query.\n");

  quote_sub_key = malloc(sizeof(*quote_sub_key) * quote_sub_key_len);
  if(!quote_sub_key) {
	return -ENOMEM;
  }
  snprintf(quote_sub_key, quote_sub_key_len, "\"%s\"", xattr_sub_key);

  log_error("---Building query using format string:%s Using sub_key:%s\n", xattr_query, xattr_sub_key);

  if(path_items > 0) {
	quote_filename_len = strlen(filename) + 3;
	quote_filename = malloc(sizeof(*quote_filename) * quote_filename_len);
	if(!quote_filename) {
	  free(quote_sub_key);
	  return -ENOMEM;
	}
	snprintf(quote_filename, quote_filename_len, "\"%s\"", filename);
	log_error("---Using quote filename:%s\n", quote_filename);

	query_len += quote_filename_len;
	*query = malloc(sizeof(**query) * query_len);
	if(!*query) {
	  free(quote_sub_key);
	  free(quote_filename);
	  return -ENOMEM;
	}

	snprintf(*query, query_len, xattr_query, quote_sub_key, quote_filename);
	free(quote_filename);
  }
  else {
	*query = malloc(sizeof(**query) * query_len);
	if(!*query) {
	  free(quote_sub_key);
	  return -ENOMEM;
	}

	snprintf(*query, query_len, xattr_query, quote_sub_key);
  }
  free(quote_sub_key);

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
