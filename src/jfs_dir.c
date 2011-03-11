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

#if !defined(_REENTRANT)
#define	_REENTRANT
#endif

#include "error_log.h"
#include "jfs_dir.h"
#include "jfs_dir_query.h"
#include "jfs_dynamic_dir.h"
#include "sqlitedb.h"
#include "jfs_util.h"
#include "jfs_meta.h"
#include "jfs_dynamic_paths.h"
#include "jfs_file_cache.h"
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
static int jfs_dir_do_mkdir(const char *path, mode_t mode);
static int jfs_dir_db_filler(const char *orig_path, const char *path, void *buf, fuse_fill_dir_t filler);
static void safe_jfs_list_destroy(struct sglib_jfs_list_t_iterator *it, jfs_list_t *item);

int
jfs_dir_mkdir(const char *path, mode_t mode)
{
  char *realpath;

  int rc;

  rc = jfs_util_resolve_new_path(path, &realpath);
  if(rc) {
    return rc;
  }

  rc = jfs_dir_do_mkdir(realpath, mode);
  free(realpath);

  if(rc) {
    return rc;
  }

  return 0;
}

static int 
jfs_dir_do_mkdir(const char *path, mode_t mode)
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
  if(inode < 0) {
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
  jfs_db_op_destroy(db_op);

  if(rc) {
    return rc;
  }

  rc = jfs_meta_setxattr(path, JFS_DIR_IS_DYNAMIC, JFS_DIR_XATTR_FALSE, 1, XATTR_CREATE);
  if(rc) {
    log_error("Unable to set default directory metadata for:%s\n", path);
  }

  return rc;
}

int 
jfs_dir_rmdir(const char *path)
{
  struct jfs_db_op *db_op;
  
  int inode;
  int rc;

  //can't remove dynamic directories!!
  if(!jfs_util_is_realpath(path)) {
    return 0;
  }

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
  jfs_db_op_destroy(db_op);

  return rc;
}

int 
jfs_dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler)
{
  char *datapath;
  
  struct stat st;
  struct dirent *de;

  DIR *dp;
  int rc;
  
  rc = jfs_util_get_datapath(path, &datapath);
  if(rc) {
    return rc;
  }

  dp = opendir(datapath);
  if(dp != NULL) {
	while((de = readdir(dp)) != NULL) {
	  memset(&st, 0, sizeof(st));
	  st.st_ino = de->d_ino;
	  st.st_mode = de->d_type << 12;
	  
	  if(filler(buf, de->d_name, &st, 0) != 0) {
		closedir(dp);
        free(datapath);

		return -ENOMEM;
	  }
	}

	rc = closedir(dp);
	if(rc) {
      free(datapath);

	  return -errno;
	}
  }

  if(jfs_dir_is_dynamic(datapath)) {
    rc = jfs_dir_db_filler(path, datapath, buf, filler);
    if(rc) {
      free(datapath);

      return rc;
    }
  }
  free(datapath);

  return 0;
}

/*
  orig_path = path passed in by user
  path = resolved path
 */
static int
jfs_dir_db_filler(const char *orig_path, const char *path, void *buf, fuse_fill_dir_t filler)
{
  struct sglib_jfs_list_t_iterator it;
  struct jfs_db_op *db_op;
  struct stat st;
  
  jfs_list_t *item;

  char *query;
  char *buffer;
  char *datapath;
  char *hardlink;

  size_t buffer_len;
  size_t datapath_len;
  mode_t mode;

  int is_folders;
  int dirinode;
  int inode;
  int mask;
  int rc;

  is_folders = 0;
  mask = R_OK | F_OK; 
  
  rc = jfs_util_get_inode_and_mode(path, &dirinode, &mode);
  if(rc) {
    return rc;
  }

  query = NULL;
  rc = jfs_dir_query_builder(orig_path, path, &is_folders, &query);
  if(rc) {
	return rc;
  }

  if(query == NULL) {
	return 0;
  }

  rc = jfs_dynamic_hierarchy_invalidate_folder(path);
  if(rc) {
    log_error("Dynamic hierarchy cleanup failed.\n");
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
	printf("---Adding filename item->filename:%s\n", item->filename);

	memset(&st, 0, sizeof(st));

	buffer_len = strlen(orig_path) + strlen(item->filename) + 2;
	buffer = malloc(sizeof(*buffer) * buffer_len);
	if(!buffer) {
	  jfs_list_destroy(item, jfs_readdir_op);
	  jfs_db_op_destroy(db_op);
	  return -ENOMEM;
	}
	snprintf(buffer, buffer_len, "%s/%s", orig_path, item->filename);
	
	if(is_folders) {
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

      hardlink = datapath;
	}
	else {
	  datapath_len = strlen(item->datapath) + 1;
	  datapath = malloc(sizeof(*datapath) * datapath_len);
	  if(!datapath) {
        free(buffer);
		safe_jfs_list_destroy(&it, item);
		jfs_db_op_destroy(db_op);
        
		return -ENOMEM;
	  }
	  strncpy(datapath, item->datapath, datapath_len);

      rc = jfs_util_get_inode_and_mode(datapath, &inode, &mode);
	  if(rc) {
        free(buffer);
        free(datapath);
		safe_jfs_list_destroy(&it, item);
		jfs_db_op_destroy(db_op);

        log_error("Failed to get inode and mode or:%s\n", datapath);

		return rc;
	  }

	  st.st_ino = inode;
	  st.st_mode = mode;

      rc = jfs_file_cache_get_sympath(inode, &hardlink);
      if(rc) {
        free(buffer);
        free(datapath);

        return rc;
      }
	}

    //check access to the hardlink
    if(access(hardlink, mask) == 0) {
      //add for display
      if(filler(buf, item->filename, &st, 0) != 0) {
        safe_jfs_list_destroy(&it, item);
        if(hardlink != datapath) {
          free(hardlink);
        }
        free(buffer);
        free(datapath);
        jfs_db_op_destroy(db_op);
        
        return -ENOMEM;
      }

      //dynamic directory path
      if(is_folders) {
        rc = jfs_dynamic_hierarchy_add_folder(buffer, datapath, st.st_ino);
      }
      //dynamic file path
      else {
        rc = jfs_dynamic_hierarchy_add_file(buffer, datapath, st.st_ino);
      }

      if(rc) {
        if(hardlink != datapath) {
          free(hardlink);
        }
        free(buffer);
        free(datapath);
        safe_jfs_list_destroy(&it, item);
        jfs_db_op_destroy(db_op);
        
        return rc;
      }
    }
    else {
      log_msg("Not allowed to access:%s\n", buffer);
    }

    free(item->datapath);
	free(item->filename);
	free(item);
    if(hardlink != datapath) {
      free(hardlink);
    }
    free(buffer);
    free(datapath);
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
    free(is_dynamic);
    return 1;
  }
  free(is_dynamic);

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
    free(item->datapath);
	free(item->filename);
	free(item);
  }
}
