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
#include "sglib.h"

#include <stdlib.h>
#include <string.h>

/*
  List of files.
 */
typedef struct jfs_filelist jfs_filelist_t;
struct jfs_filelist {
  int             datainode;
  char           *name;

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

  jfs_filelist_t *files;
  jfs_dirlist_t  *folders;

  jfs_dirlist_t *next;  
};

#define JFS_DIRLIST_T_CMP(e1, e2) (strcmp(e1->name, e2->name))

SGLIB_DEFINE_LIST_PROTOTYPES(jfs_dirlist_t, JFS_DIRLIST_T_CMP, next)
SGLIB_DEFINE_LIST_FUNCTIONS(jfs_dirlist_t, JFS_DIRLIST_T_CMP, next)

/*
  Resolves a dynamic path into a datapath.

  Returns 0 on success, -ENOENT or -ENOMEM on failure.
 */
int 
jfs_dynamic_path_resolution(const char *path, char **resolved_path)
{
  char *path_copy;
  char *token;

  size_t path_len;
  
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

  token = strtok(&path_copy[1], "/");
  while(token != null) {
    //resolve the path and add the file
    
    token = strtok(NULL, "/");
  }

  return 0;
}

/*
  Add a dynamic file to the dynamic path hierarchy.

  Returns 0 on success, negative error code on failure.
 */
int
jfs_dynamic_hierarchy_add_file(char *path, const char *datapath, int datainode)
{
  char *token;

  if(strlen(path) < 2 || path[0] != '/') {
    return -1;
  } 

  token = strtok(&path[1], "/");
  while(token != null) {
    //resolve the path and add the file
    
    token = strtok(NULL, "/");
  }

  return 0;
}
