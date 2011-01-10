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

#include "jfs_security.h"
#include "jfs_util.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int 
jfs_security_chmod(const char *path, mode_t mode)
{
  char *datapath;

  datapath = jfs_util_get_datapath(path);
  if(!datapath) {
	return -1;
  }

  return chmod(datapath, mode);
}

int 
jfs_security_chown(const char *path, uid_t uid, gid_t gid)
{
  char *datapath;

  datapath = jfs_util_get_datapath(path);
  if(!datapath) {
	return -1;
  }

  return lchown(datapath, uid, gid);
}

int 
jfs_security_access(const char *path, int mask)
{
  char *datapath;

  datapath = jfs_util_get_datapath(path);
  if(!datapath) {
	return -1;
  }

  return access(datapath, mask);
}
