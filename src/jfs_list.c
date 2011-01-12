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

#include "sglib.h"
#include "jfs_list.h"

#include <stdlib.h>

SGLIB_DEFINE_LIST_FUNCTIONS(jfs_list_t, JFS_LIST_CMP, next)

void
jfs_list_add(jfs_list_t **head, jfs_list_t *node)
{
  sglib_jfs_list_t_add(head, node);
}

void
jfs_list_destroy(jfs_list_t *head, enum jfs_db_ops op)
{
  struct sglib_jfs_list_t_iterator it;
  jfs_list_t *item;
  
  for(item = sglib_jfs_list_t_it_init(&it, head);
	  item != NULL; item = sglib_jfs_list_t_it_next(&it)) {
	switch(op) {
	case(jfs_listattr_op):
	  free(item->key);
	  free(item);
	  break;
	default:
	  free(item);
	  break;
	}
  }
}
