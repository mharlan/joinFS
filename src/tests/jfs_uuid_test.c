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

#include "jfs_uuid.h"

#include <stdio.h>

/*
 * Tests UUID generation.
 */
int main()
{
  char *uuid;
  int i;

  printf("JoinFS UUID test start.\n");

  uuid = jfs_create_uuid();

  for(i = 0; i < 100; ++i) {
	jfs_generate_uuid(uuid);
	printf("UUID(%d):%s\n", i, uuid);
  }

  jfs_destroy_uuid(uuid);

  return 0;
}
