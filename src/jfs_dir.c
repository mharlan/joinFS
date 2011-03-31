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
#include "jfs_file.h"
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
static int jfs_dir_db_filler(const char *path, const char *realpath, void *buf, fuse_fill_dir_t filler);
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

  rc = jfs_file_db_add(inode, path, filename);
  if(rc) {
    return rc;
  }
  
  return 0;
}

int 
jfs_dir_rmdir(const char *path)
{
  struct jfs_db_op *db_op;
 
  char *file_query;
  char *metadata_query;
  
  int rc;

  //can't remove dynamic directories!!
  if(!jfs_util_is_realpath(path)) {
    return -EPERM;
  }

  rc = jfs_db_op_create_query(&metadata_query,
                              "DELETE FROM metadata WHERE jfs_id=(SELECT jfs_id FROM links WHERE path=\"%s\");",
                              path);
  if(rc) {
    return rc;
  }

  rc = jfs_db_op_create_query(&file_query, 
                              "DELETE FROM links WHERE path=\"%s\";",
                              path);
  if(rc) {
    free(metadata_query);
	return rc;
  }

  rc = jfs_db_op_create_multi_op(&db_op, 2, metadata_query, file_query);
  if(rc) {
    free(file_query);
    free(metadata_query);
    return rc;
  }

  jfs_write_pool_queue(db_op);
  rc = jfs_db_op_wait(db_op);
  jfs_db_op_destroy(db_op);
  
  if(rc) {
    return rc;
  }

  rc = rmdir(path);
  if(rc) {
	return -errno;
  }

  return rc;
}

int
jfs_dir_opendir(const char *path, DIR **d)
{
  char *datapath;
  DIR *dp;

  int rc;

  rc = jfs_util_get_datapath(path, &datapath);
  if(rc) {
    return rc;
  }

  dp = opendir(datapath);
  free(datapath);

  if(!dp) {
    return -errno;
  }
  *d = dp;

  return 0;
}

int 
jfs_dir_readdir(const char *path, DIR *dp, void *buf, 
                fuse_fill_dir_t filler)
{
  struct stat st;
  struct dirent *de;
 
  char *datapath;
  
  int rc;

  printf("---jfs_dir_readdir, path:%s\n", path);
  
  while((de = readdir(dp)) != NULL) {
    memset(&st, 0, sizeof(st));
    st.st_ino = de->d_ino;
    st.st_mode = de->d_type << 12;
    
    if(filler(buf, de->d_name, &st, 0) != 0) {
      printf("filler fails\n");
      return -errno;
    }
  }
  
  rc = jfs_util_get_datapath(path, &datapath);
  if(rc) {
    return rc;
  }

  if(jfs_dir_is_dynamic(datapath)) {
    rc = jfs_dir_db_filler(path, datapath, buf, filler);
    if(rc) {
      free(datapath);

      return rc;
    }
  }
  else {
    printf("is not dynamic\n");
  }
  free(datapath);

  return 0;
}

/*
  orig_path = path passed in by user
  path = resolved path
 */
static int
jfs_dir_db_filler(const char *path, const char *realpath, void *buf, 
                  fuse_fill_dir_t filler)
{
  struct sglib_jfs_list_t_iterator it;
  struct jfs_db_op *db_op;
  struct stat st;
  struct stat item_st;
  
  jfs_list_t *item;

  char *query;
  char *buffer;
  char *datapath;

  size_t buffer_len;
  size_t datapath_len;

  int is_folders;
  int datainode;
  int mask;
  int rc;

  datapath = NULL;
  is_folders = 0;
  datainode = 0;
  mask = R_OK | F_OK; 
  
  printf("---jfs_db_readder start\n");

  query = NULL;
  rc = jfs_dir_query_builder(path, realpath, &is_folders, &query);
  if(rc) {
	return rc;
  }
 
  printf("---query:%s\n", query);
 
  if(query == NULL) {
	return 0;
  }

  rc = jfs_dynamic_hierarchy_invalidate_folder(path);
  if(rc) {
    return rc;
  }

  rc = jfs_dynamic_hierarchy_add_folder(path, realpath);
  if(rc) {
    return rc;
  }

  if(is_folders) {
    datapath_len = strlen(realpath) + strlen(".jfs_sub_query") + 2;
    datapath = malloc(sizeof(*datapath) * datapath_len);
    if(!datapath) {
      return -ENOMEM;
    }
    snprintf(datapath, datapath_len, "%s/%s", realpath, ".jfs_sub_query");

    memset(&st, 0, sizeof(st));
    rc = stat(datapath, &st);
    if(rc) {
      free(datapath);
      
      return -errno;
    }
    
    datainode = st.st_ino;
  }
  
  rc = jfs_do_db_op_create(&db_op, jfs_readdir_op, query);
  if(rc) {
	return rc;
  }

  printf("---jfs_readdir query:%s\n", query);
  
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
    memset(&item_st, 0, sizeof(item_st));

	buffer_len = strlen(path) + strlen(item->filename) + 2;
	buffer = malloc(sizeof(*buffer) * buffer_len);
	if(!buffer) {
      if(is_folders) {
        free(datapath);
      }
	  jfs_list_destroy(item, jfs_readdir_op);
	  jfs_db_op_destroy(db_op);
      
	  return -ENOMEM;
	}
	snprintf(buffer, buffer_len, "%s/%s", path, item->filename);
	
	if(!is_folders) {
      memset(&st, 0, sizeof(st));

      rc = stat(item->datapath, &st);
      if(rc) {
        if(is_folders) {
          free(datapath);
        }
        free(buffer);
		safe_jfs_list_destroy(&it, item);
		jfs_db_op_destroy(db_op);

        return -errno;
      }
	}
    item_st.st_ino = st.st_ino;
    item_st.st_mode = st.st_mode;

    if(is_folders) {
      //add for display
      if(filler(buf, item->filename, &item_st, 0) != 0) {
        free(datapath);
        free(buffer);
        safe_jfs_list_destroy(&it, item);
        jfs_db_op_destroy(db_op);
        
        return -ENOMEM;
      }
      
      rc = jfs_dynamic_hierarchy_add_folder(buffer, datapath);
    }
    else if(access(item->datapath, mask) == 0) {
      //add for display
      if(filler(buf, item->filename, &item_st, 0) != 0) {
        free(buffer);
        safe_jfs_list_destroy(&it, item);
        jfs_db_op_destroy(db_op);
        
        return -ENOMEM;
      }
      
      rc = jfs_dynamic_hierarchy_add_file(buffer, item->datapath, item->jfs_id);
    }
    
    if(rc) {
      if(is_folders) {
        free(datapath);
      }
      free(buffer);
      safe_jfs_list_destroy(&it, item);
      jfs_db_op_destroy(db_op);
      
      return rc;
    }

    free(item->datapath);
	free(item->filename);
	free(item);
    free(buffer);
  }
  jfs_db_op_destroy(db_op);

  if(is_folders) {
    free(datapath);
  }

  return 0;
}

static int
jfs_dir_is_dynamic(const char *path)
{
  char *is_dynamic;
  int rc;
  
  rc = jfs_meta_do_getxattr(path, JFS_DIR_IS_DYNAMIC, &is_dynamic);
  if(rc) {
    if(rc == -ENOATTR) {
      return 0;
    }
    return rc;
  }

  printf("---jfs_meta is dynamic\n");

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
