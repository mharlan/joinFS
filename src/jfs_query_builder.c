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

#include "sqlitedb.h"
#include "jfs_query_builder.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define JFS_FORMAT_ADJ 2

int
jfs_query_builder(char **query, const char *format, ...)
{
  va_list args;

  char *new_query;
  char *np;
  
  int query_size;
  int rc;

  query_size = JFS_QUERY_INC;
  new_query = malloc(sizeof(*new_query) * query_size);
  if(!new_query) {
    return -ENOMEM;
  }
  
  while(1) {
    va_start(args, format);
    rc = vsnprintf(new_query, query_size, format, args);
    va_end(args);

    printf("RC is:%d\n", rc);

    if(rc > -1 && rc < query_size) {
      break;
    }
    else if(rc > -1) {
      query_size = rc + 1;
    }
    else {
      if(errno == EILSEQ) {
        return -errno;
      }
      
      query_size += JFS_QUERY_INC;
    }

    printf("New query_size:%d\n", query_size);
    
    np = realloc(new_query, sizeof(*new_query) * query_size);
    if(!np) {
      free(new_query);
      
      return -ENOMEM;
    }
    else {
      new_query = np;
    }
  }

  *query = new_query;
  
  return 0;
}
