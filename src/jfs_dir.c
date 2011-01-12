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

#include "error_log.h"
#include "jfs_dir.h"

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

int 
jfs_dir_mkdir(const char *path, mode_t mode)
{
  int rc;

  rc = mkdir(path, mode);

  return rc;
}

int 
jfs_dir_rmdir(const char *path)
{
  int rc;

  rc = rmdir(path);

  return rc;
}

int 
jfs_dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler)
{
  DIR *dp;
  struct dirent *de;
  
  dp = opendir(path);
  if (dp == NULL) {
	log_error("Error occured, errno:%d\n", -errno);
    return -errno;
  }

  while ((de = readdir(dp)) != NULL) {
    struct stat st;
    memset(&st, 0, sizeof(st));
    st.st_ino = de->d_ino;
    st.st_mode = de->d_type << 12;

    if (filler(buf, de->d_name, &st, 0)) {
	  break;
	}
  }
  closedir(dp);
  
  return 0;
}
