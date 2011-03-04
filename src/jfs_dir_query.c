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

#include "jfs_meta.h"
#include "jfs_util.h"
#include "jfs_dir_query.h"
#include "jfs_dynamic_dir.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define JFS_FILE_QUERY   "SELECT m.keyvalue FROM keys AS k, metadata AS m WHERE "
#define JFS_FOLDER_QUERY "SELECT f.inode, f.filename, f.datapath FROM files AS f, metadata AS, keys AS k WHERE "

static int jfs_dir_query_get_parent_key_pairs(char *path, char **parent_key_pairs);

int 
jfs_dir_query_builder(const char *path, int inode, int *is_folders, char **query)
{
  char *copy_path;
  char *is_folder;
  char *key_pairs;
  char *path_items;

  size_t path_len;

  int items;
  int rc;

  rc = jfs_meta_do_getxattr(path, JFS_DIR_KEY_PAIRS, &key_pairs);
  if(rc) {
    return rc;
  }
  
  rc = jfs_meta_do_getxattr(path, JFS_DIR_IS_FOLDER, &is_folder);
  if(rc) {
    *is_folders = 0;
  }
  else {
    *is_folders = 1;
  }

  rc = jfs_meta_do_getxattr(path, JFS_DIR_PATH_ITEMS, &path_items);
  if(rc) {
    items = 0;
  }
  else {
    items = atoi(path_items);
  }

  //build query beginning
  if(*is_folder) {
    
  }
  else {
    
  }

  //copy the path so we can modify it
  path_len = strlen(path) + 1;
  copy_path = malloc(sizeof(*copy_path) * path_len);
  if(!copy_path) {
    return -ENOMEM;
  }
  strncpy(copy_path, path, path_len);

  //build the query

  return 0;
}

static int
jfs_dir_query_get_parent_key_pairs(char *path, char **parent_key_pairs)
{
  char *datapath;

  int rc;

  rc = jfs_util_get_datapath(path, &datapath);
  if(rc) {
    return rc;
  }
  
  //get the parent key_pairs

  return 0;
}
