#ifndef JOINFS_JFS_LIST_H
#define JOINFS_JFS_LIST_H

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

#include "sglib.h"
#include "jfs_db_ops.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

/*!
 * The jfs_list node. Used for storing
 * a database result row.
 */
typedef struct jfs_list jfs_list_t;
struct jfs_list {
  jfs_list_t *next;

  int         jfs_id;
  char       *datapath;
  char       *filename;

  int         keyid;
  char       *key;
  char       *value;
};

/*
 * SGLIB list header file macros.
 */
#define JFS_LIST_CMP(a, b) (a->jfs_id - b->jfs_id)
SGLIB_DEFINE_LIST_PROTOTYPES(jfs_list_t, JFS_LIST_CMP, next)

/*!
 *  Add a node to a jfs list.
 * \param head The list head.
 * \param node The node to add.
 */
void jfs_list_add(jfs_list_t **head, jfs_list_t *node);

/*!
 *  Destroy a jfs list
 * \param head The list head.
 * \param op The type result in the jfs list.
 */
void jfs_list_destroy(jfs_list_t *head, enum jfs_db_ops op);

#endif
