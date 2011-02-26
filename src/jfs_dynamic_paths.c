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

#include "jfs_dynamic_paths.h"
#include "jfs_datapath_cache.h"
#include "sglib.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*
  List of files.
 */
typedef struct jfs_filelist jfs_filelist_t;
struct jfs_filelist {
  char           *name;
  int             datainode;

  jfs_filelist_t *next;
};

#define JFS_FILELIST_T_CMP(e1, e2) (strcmp(e1->name, e2->name))

SGLIB_DEFINE_LIST_PROTOTYPES(jfs_filelist_t, JFS_FILELIST_T_CMP, next)
SGLIB_DEFINE_LIST_FUNCTIONS(jfs_filelist_t, JFS_FILELIST_T_CMP, next)

/*
  Directory list.
 */
typedef struct jfs_dirlist jfs_dirlist_t;
typedef struct jfs_dentry jfs_dentry_t;

struct jfs_dirlist {
  char           *name;
  int             datainode;

  jfs_filelist_t *files;
  jfs_dirlist_t  *folders;

  jfs_dirlist_t  *next;
};

#define JFS_DIRLIST_T_CMP(e1, e2) (strcmp(e1->name, e2->name))

SGLIB_DEFINE_LIST_PROTOTYPES(jfs_dirlist_t, JFS_DIRLIST_T_CMP, next)
SGLIB_DEFINE_LIST_FUNCTIONS(jfs_dirlist_t, JFS_DIRLIST_T_CMP, next)

static jfs_dirlist_t jfs_root;

int
jfs_dynamic_path_init(void)
{
  jfs_root.files = NULL;
  jfs_root.folders = NULL;
  jfs_root.next = NULL;

  return 0;
}

/*
  Resolves a dynamic path into a datapath.

  Returns 0 on success, -ENOENT or -ENOMEM on failure.
 */
int 
jfs_dynamic_path_resolution(const char *path, char **resolved_path, int *datainode)
{
  size_t path_len;

  char *path_copy;
  char *last_token;
  char *token;

  jfs_dirlist_t *current_dir;
  jfs_dirlist_t *result_dir;
  jfs_dirlist_t  check_dir;

  jfs_filelist_t *result_file;
  jfs_filelist_t  check_file;
  
  path_len = strlen(path) + 1;
  if(path_len < 3 || path[0] != '/') {
    return -1;
  }

  //have to copy the path, can't modify a const
  path_copy = malloc(sizeof(*path_copy) * path_len);
  if(!path_copy) {
    return -ENOMEM;
  }
  strncpy(path_copy, path, path_len);

  last_token = strrchr(path_copy, '/') + 1;
  current_dir = &jfs_root;
  token = strtok(&path_copy[1], "/");

  while(token != NULL) {
    //last token, check folders and files
    if(token == last_token) {
      check_dir.name = token;
      result_dir = sglib_jfs_dirlist_t_find_member(current_dir->folders, &check_dir);

      if(result_dir) {
        *datainode = result_dir->datainode;
        return jfs_datapath_cache_get_datapath(result_dir->datainode, resolved_path);
      }
      else {
        check_file.name = token;
        result_file = sglib_jfs_filelist_t_find_member(current_dir->files, &check_file);
        
        if(result_file) {
          *datainode = result_file->datainode;
          return jfs_datapath_cache_get_datapath(result_file->datainode, resolved_path);
        }
        else {
          return -ENOENT;
        }
      }

      break;
    }
    //check folders only
    else {
      check_dir.name = token;
      current_dir = sglib_jfs_dirlist_t_find_member(current_dir->folders, &check_dir);
      if(!current_dir) {
        return -ENOENT;
      }
    }
    token = strtok(NULL, "/");
  }

  return 0;
}

/*
  Add a dynamic file to the dynamic path hierarchy.

  Returns 0 on success, negative error code on failure.
 */
int
jfs_dynamic_hierarchy_add_file(char *path, char *datapath, int datainode)
{
  jfs_filelist_t *file;

  jfs_dirlist_t *current_dir;
  jfs_dirlist_t *result_dir;
  jfs_dirlist_t *check_dir;

  size_t token_len;

  char *last_token;
  char *token;

  int rc;

  if(strlen(path) < 2 || path[0] != '/') {
    return -1;
  } 

  last_token = strrchr(path, '/') + 1;
  token = strtok(&path[1], "/");
  current_dir = &jfs_root;

  while(token != NULL) {
    token_len = strlen(token) + 1;

    //resolve the path and add the file
    if(token == last_token) {
      file = malloc(sizeof(*file));
      if(!file) {
        return -ENOMEM;
      }

      file->name = malloc(sizeof(*file->name) * token_len);
      if(!file->name) {
        free(file);
        return -ENOMEM;
      }
      strncpy(file->name, token, token_len);
      file->datainode = datainode;
      file->next = NULL;

      sglib_jfs_filelist_t_add(&current_dir->files, file);

      return jfs_datapath_cache_add(datainode, datapath);
    }
    //find the directory the file is in
    else {
      check_dir = malloc(sizeof(*check_dir));
      if(!check_dir) {
        return -ENOMEM;
      }
      
      check_dir->name = malloc(sizeof(*check_dir->name) * token_len);
      if(!check_dir->name) {
        free(check_dir);
        return -ENOMEM;
      }
      strncpy(check_dir->name, token, token_len);
      
      check_dir->datainode = 0;
      check_dir->files = NULL;
      check_dir->folders = NULL;
      check_dir->next = NULL;
      
      rc = sglib_jfs_dirlist_t_add_if_not_member(&current_dir->folders, check_dir, &result_dir);
      if(rc) { //item was inserted
        current_dir = check_dir;
      }
      else { //was already in the list
        current_dir = result_dir;

        free(check_dir->name);
        free(check_dir);
      }
    }

    token = strtok(NULL, "/");
  }

  //shouldn't get here
  return -1;
}

/*
  Add a folder to the dynamic hierarchy.
 */
int
jfs_dynamic_hierarchy_add_folder(char *path, char *datapath, int datainode)
{
  jfs_dirlist_t *check_dir;
  jfs_dirlist_t *current_dir;
  jfs_dirlist_t *result_dir;

  size_t token_len;
  char *token;
  char *last_token;

  int rc;

  if(strlen(path) < 2 || path[0] != '/') {
    return -1;
  }
  
  last_token = strrchr(path, '/') + 1;
  token = strtok(&path[1], "/");
  current_dir = &jfs_root;

  while(token != NULL) {
    token_len = strlen(token) + 1;

    check_dir = malloc(sizeof(*check_dir));
    if(!check_dir) {
      return -ENOMEM;
    }

    check_dir->name = malloc(sizeof(*check_dir->name) * token_len);
    if(!check_dir->name) {
      free(check_dir);
      return -ENOMEM;
    }
    strncpy(check_dir->name, token, token_len);

    check_dir->files = NULL;
    check_dir->folders = NULL;
    check_dir->next = NULL;

    if(token == last_token) {
      check_dir->datainode = datainode;
      sglib_jfs_dirlist_t_add(&current_dir->folders, check_dir);

      return jfs_datapath_cache_add(datainode, datapath);
    }
    else {
      check_dir->datainode = 0;
      rc = sglib_jfs_dirlist_t_add_if_not_member(&current_dir->folders, check_dir, &result_dir);

      if(rc) { //item was inserted
        current_dir = check_dir;
      }
      else { //was already in the list
        current_dir = result_dir;

        free(check_dir->name);
        free(check_dir);
      }
    }

    token = strtok(NULL, "/");
  }
  
  //shouldn't get here
  return -1;
}
