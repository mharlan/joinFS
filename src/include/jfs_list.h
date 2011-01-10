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

#ifndef JOINFS_JFS_LIST_H
#define JOINFS_JFS_LIST_H

#include "sglib.h"

#include <sys/types.h>

/*
 * Enumerator used to determine what fields 
 * are set in a jfs_list node.
 */
enum jfs_t {
  jfs_write_op,
  jfs_file_cache_op,
  jfs_key_op,
  jfs_attr_op,
  jfs_listattr_op,
  jfs_dynamic_file_op,
  jfs_folder_cache_op,
  jfs_dynamic_folder_op,
  jfs_search_op
};

/*
 * The jfs_list node. Used for storing
 * a database result row.
 */
typedef struct jfs_list jfs_list_t;
struct jfs_list {
  jfs_list_t *next;
  int         inode;
  char       *datapath;
  char       *filename;
  int         keyid;
  char       *key;
  char       *value;
};

/*
 * SGLIB list header file macros.
 */
#define JFS_LIST_CMP(a, b) (a->inode - b->inode)
SGLIB_DEFINE_LIST_PROTOTYPES(jfs_list_t, JFS_LIST_CMP, next)

void jfs_list_t_add(jfs_list_t **head, jfs_list_t *node);

#endif
