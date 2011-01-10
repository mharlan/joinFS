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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define JFS_MPATH     "/home/joinfs/git/joinFS/demo/.queries"
#define JFS_MPATH_LEN 37

static char *
jfs_realpath(const char *path)
{
  char *jfs_realpath;
  int path_len;

  printf("Called jfs_realpath, path:%s\n", path);

  path_len = strlen(path) + JFS_MPATH_LEN + 2;
  jfs_realpath = malloc(sizeof(*jfs_realpath) * path_len);
  if(!jfs_realpath) {
	printf("Failed to allocate memory for jfs_realpath.\n");
  }
  else {
	if(path[0] == '/') {
	  snprintf(jfs_realpath, path_len, "%s%s", JFS_MPATH, path);
	}
	else if(path == NULL) {
	  snprintf(jfs_realpath, path_len, "%s/", JFS_MPATH);
	}
	else {
	  snprintf(jfs_realpath, path_len, "%s/%s", JFS_MPATH, path);
	}

	printf("Computed realpath:%s\n", jfs_realpath);
  }

  return jfs_realpath;
}

int main()
{
  char *jfs_path;

  printf("Started joinFS realpath test.\n");

  jfs_path = jfs_realpath("/");
  free(jfs_path);

  jfs_path = jfs_realpath("");
  free(jfs_path);

  printf("Finished joinFS realpath tests.\n");

  return 0;
}
