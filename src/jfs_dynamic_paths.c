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

#include "jfs_util.h"
#include "jfs_dynamic_paths.h"
#include "jfs_datapath_cache.h"
#include "sglib.h"

#include <stdio.h>
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

static void jfs_dynamic_hierarchy_folder_cleanup(jfs_dirlist_t *root);
static int jfs_dynamic_hierarchy_get_node(const char *path, jfs_filelist_t **file, 
                                          jfs_dirlist_t **dir, jfs_filelist_t **file_list, 
                                          jfs_dirlist_t **dir_list, jfs_dirlist_t **parent);

int
jfs_dynamic_path_init(void)
{
  jfs_root.name = NULL;
  jfs_root.files = NULL;
  jfs_root.folders = NULL;
  jfs_root.next = NULL;
  jfs_root.datainode = 0;

  return 0;
}

/*
  Resolves a dynamic path into a datapath.

  Returns 0 on success, -ENOENT or -ENOMEM on failure.
 */
int
jfs_dynamic_path_resolution(const char *path, char **resolved_path, int *datainode)
{
  jfs_dirlist_t *dir;
  jfs_filelist_t *file;

  int rc;

  dir = NULL;
  file = NULL;
  rc = jfs_dynamic_hierarchy_get_node(path, &file, &dir, NULL, NULL, NULL);
  if(rc) {
    return rc;
  }

  if(file) {
    *datainode = file->datainode;
  }
  else {
    *datainode = dir->datainode;
  }

  rc = jfs_datapath_cache_get_datapath(*datainode, resolved_path);
  if(rc) {
    return rc;
  }

  return 0;
}

static int 
jfs_dynamic_hierarchy_get_node(const char *path, jfs_filelist_t **file, 
                               jfs_dirlist_t **dir, jfs_filelist_t **file_list, 
                               jfs_dirlist_t **dir_list, jfs_dirlist_t **parent)
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
  if(path_len < 1) {
    return -ENOENT;
  }
  else if(path[0] != '/') {
    return -ENOENT;
  }

  //root node?
  if(path_len == 2) {
    if(!dir) {
      return -ENOENT;
    }
    *dir = &jfs_root;

    if(dir_list) {
      *dir_list = &jfs_root;
    }

    return 0;
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
        if(!dir) {
          return -ENOENT;
        }
        *dir = result_dir;

        if(dir_list) {
          *dir_list = current_dir->folders;
        }

        return 0;
      }
      else {
        check_file.name = token;
        result_file = sglib_jfs_filelist_t_find_member(current_dir->files, &check_file);
        
        if(result_file) {
          if(!file) {
            return -ENOENT;
          }
          *file = result_file;

          if(file_list) {
            *file_list = current_dir->files;
          }

          return 0;
        }
        else {
          if(parent) {
            *parent = current_dir;

            return 0;
          }

          return -ENOENT;
        }
      }
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

  return -ENOENT;
}

/*
Add a dynamic file to the dynamic path hierarchy.

Returns 0 on success, negative error code on failure.
*/
int
jfs_dynamic_hierarchy_add_file(const char *path, char *datapath, int datainode)
{
  jfs_filelist_t *file;

  jfs_dirlist_t *current_dir;
  jfs_dirlist_t *result_dir;
  jfs_dirlist_t *check_dir;

  size_t copy_path_len;
  size_t token_len;

  char *copy_path;
  char *last_token;
  char *token;

  int rc;

  if(strlen(path) < 2 || path[0] != '/') {
    return -ENOENT;
  }

  copy_path_len = strlen(path) + 1;
  copy_path = malloc(sizeof(*copy_path) * copy_path_len);
  if(!copy_path) {
    return -ENOMEM;
  }
  strncpy(copy_path, path, copy_path_len);

  last_token = strrchr(copy_path, '/') + 1;
  token = strtok(&copy_path[1], "/");
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
  return -ENOENT;
}

/*
Add a folder to the dynamic hierarchy.
*/
int
jfs_dynamic_hierarchy_add_folder(const char *path, char *datapath, int datainode)
{
  jfs_dirlist_t *check_dir;
  jfs_dirlist_t *current_dir;
  jfs_dirlist_t *result_dir;

  size_t copy_path_len;
  size_t token_len;

  char *token;
  char *last_token;
  char *copy_path;

  int rc;

  if(strlen(path) < 2 || path[0] != '/') {
    return -ENOENT;
  }

  copy_path_len = strlen(path) + 1;
  copy_path = malloc(sizeof(*copy_path) * copy_path_len);
  if(!copy_path) {
    return -ENOMEM;
  }
  strncpy(copy_path, path, copy_path_len);
  
  last_token = strrchr(copy_path, '/') + 1;
  token = strtok(&copy_path[1], "/");
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
  return -ENOENT;
}

int 
jfs_dynamic_hierarchy_rename(const char *path, const char *filename)
{
  jfs_filelist_t *file;
  jfs_dirlist_t  *dir;

  char *new_filename;

  size_t filename_len;

  int rc;

  dir = NULL;
  file = NULL;
  rc = jfs_dynamic_hierarchy_get_node(path, &file, &dir, NULL, NULL, NULL);
  if(rc) {
    return rc;
  }

  filename_len = strlen(filename) + 1;
  new_filename = malloc(sizeof(*new_filename) * filename_len);
  if(!new_filename) {
    return -ENOMEM;
  }
  strncpy(new_filename, filename, filename_len);

  if(file) {
    free(file->name);
    file->name = new_filename;
  }
  else {
    free(dir->name);
    dir->name = new_filename;
  }

  return 0;
}

int
jfs_dynamic_hierarchy_destroy(void)
{
  jfs_dynamic_hierarchy_folder_cleanup(&jfs_root);

  return 0;
}

int
jfs_dynamic_hierarchy_unlink(const char *path)
{
  jfs_filelist_t *file_list;
  jfs_filelist_t *file;

  int rc;

  file = NULL;
  file_list = NULL;
  rc = jfs_dynamic_hierarchy_get_node(path, &file, NULL, &file_list, NULL, NULL);
  if(rc) {
    return rc;
  }

  sglib_jfs_filelist_t_delete(&file_list, file);

  return 0;
}

int
jfs_dynamic_hierarchy_rmdir(const char *path)
{
  jfs_dirlist_t *dir_list;
  jfs_dirlist_t *dir;

  int not_empty;
  int rc;

  dir = NULL;
  dir_list = NULL;
  rc = jfs_dynamic_hierarchy_get_node(path, NULL, &dir, NULL, &dir_list, NULL);
  if(rc) {
    return rc;
  }

  not_empty = 0;
  if(dir->folders) {
    not_empty += sglib_jfs_dirlist_t_len(dir->folders);
    if(not_empty) {
      return -ENOTEMPTY;
    }
  }

  if(dir->files) {
    not_empty += sglib_jfs_filelist_t_len(dir->files);
    if(not_empty) {
      return -ENOTEMPTY;
    }
  }
  
  sglib_jfs_dirlist_t_delete(&dir_list, dir);

  return 0;
}

int
jfs_dynamic_hierarchy_invalidate_folder(const char *path)
{
  jfs_dirlist_t  *root;

  int rc;

  printf("Invalidating folder:%s\n", path);

  root = NULL;
  rc = jfs_dynamic_hierarchy_get_node(path, NULL, &root, NULL, NULL, NULL);
  if(rc == -ENOENT) {
    return 0;
  }
  else if(rc) {
    return rc;
  }

  jfs_dynamic_hierarchy_folder_cleanup(root);

  printf("--Invalidated folder:%s\n", path);

  return 0;
}

/*
  Recursively delete. Root is not deleted.
 */
static void
jfs_dynamic_hierarchy_folder_cleanup(jfs_dirlist_t *root)
{
  struct sglib_jfs_filelist_t_iterator file_it;
  struct sglib_jfs_dirlist_t_iterator  dir_it;

  jfs_dirlist_t  *dir;
  jfs_filelist_t *file;

  if(root->files) {
    for(file = sglib_jfs_filelist_t_it_init(&file_it, root->files);
        file != NULL; file = sglib_jfs_filelist_t_it_next(&file_it)) {
      sglib_jfs_filelist_t_delete(&root->files, file);

      free(file->name);
      free(file);
    }
  }

  //recursive
  if(root->folders) {
    for(dir = sglib_jfs_dirlist_t_it_init(&dir_it, root->folders);
        dir != NULL; dir = sglib_jfs_dirlist_t_it_next(&dir_it)) {
      sglib_jfs_dirlist_t_delete(&root->folders, dir);
      jfs_dynamic_hierarchy_folder_cleanup(dir);

      free(dir->name);
      free(dir);
    }
  }
}
