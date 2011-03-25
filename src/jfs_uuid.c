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
#include "jfs_uuid.h"
#include "joinfs.h"

#include <fuse.h>
#include <errno.h>
#include <stdlib.h>
#include <uuid/uuid.h>

/*
 * Generates a new unique joinFS id.
 *
 * Memory for the uuid must already be allocated with
 * jfs_create_uuid().
 */
int
jfs_uuid_generate(char **uuid)
{
  char *the_uuid;
  uuid_t id;

  the_uuid = malloc(sizeof(*the_uuid) * JFS_UUID_LEN);
  if(!the_uuid) {
    return -ENOMEM;
  }

  uuid_generate(id);
  uuid_unparse_lower(id, the_uuid);
  uuid_clear(id);

  *uuid = the_uuid;

  return 0;
}

int
jfs_uuid_new_datapath(char **datapath)
{
  char *uuid;
  char *d_path;

  size_t path_len;

  int rc;

  rc = jfs_uuid_generate(&uuid);
  if(rc) {
    return rc;
  }

  path_len = JFS_CONTEXT->datapath_len + JFS_UUID_LEN + 1;
  d_path = malloc(sizeof(*d_path) * path_len);
  if(!d_path) {
	free(uuid);
	return -ENOMEM;
  }
  snprintf(d_path, path_len, "%s/%s", JFS_CONTEXT->datapath, uuid);
  free(uuid);
  
  *datapath = d_path;

  return 0;
}
