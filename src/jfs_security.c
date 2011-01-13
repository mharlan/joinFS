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

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int 
jfs_security_chmod(const char *path, mode_t mode)
{
  char *datapath;
  int rc;

  rc = jfs_util_get_datapath(path, &datapath);
  if(rc) {
	return rc;
  }

  rc = chmod(datapath, mode);
  if(rc) {
	return -errno;
  }

  return 0;
}

int 
jfs_security_chown(const char *path, uid_t uid, gid_t gid)
{
  char *datapath;
  int rc;

  rc = jfs_util_get_datapath(path, &datapath);
  if(rc) {
	return rc;
  }

  rc = lchown(datapath, uid, gid);
  if(rc) {
	return -errno;
  }

  return 0;
}

int 
jfs_security_access(const char *path, int mask)
{
  char *datapath;
  int rc;

  rc = jfs_util_get_datapath(path, &datapath);
  if(rc) {
	return rc;
  }

  rc = access(datapath, mask);
  if(rc) {
	return -errno;
  }

  return 0;
}
