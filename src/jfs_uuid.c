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

#include <stdlib.h>
#include <uuid/uuid.h>

/*
 * Allocate the memory to store a uuid.
 */
char *
jfs_create_uuid()
{
  char *uuid;

  uuid = malloc(sizeof(*uuid) * JFS_UUID_LEN);
  if(!uuid) {
	log_error("Failed to allocate memory for new uuid.\n");
  }

  return uuid;
}

/*
 * Deallocate the memory for a uuid.
 */
void
jfs_destroy_uuid(char *uuid)
{
  free(uuid);
}

/*
 * Generates a new unique joinFS id.
 *
 * Memory for the uuid must already be allocated with
 * jfs_create_uuid().
 */
void
jfs_generate_uuid(char *uuid)
{
  uuid_t id;

  uuid_generate(id);
  uuid_unparse_lower(id, uuid);
}
