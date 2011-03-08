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

#ifndef JOINFS_JFS_FILE_H
#define JOINFS_JFS_FILE_H

#include <fuse.h>
#include <sys/time.h>

/*
 * Create a joinFS static file. The file is added
 * to the Linux VFS and the database.
 *
 * Returns the inode of the new file or -1;
 */
int jfs_file_create(const char *path, mode_t mode);

/*
 * Create a joinFS static file. The file is added
 * to the Linux VFS and the database.
 *
 * Returns 0 on success, -1 on failure.
 */
int jfs_file_mknod(const char *path, mode_t mode);

/*
 * Delets a joinFS static file.
 */
int jfs_file_unlink(const char *path);

/*
 * Perform a rename on a joinFS file.
 */
int jfs_file_rename(const char *from, const char *to);

/*
 * Truncate a joinFS file.
 */
int jfs_file_truncate(const char *path, off_t size);

/*
 * Open a joinFS file.
 */
int jfs_file_open(const char *path, int flags);

/*
 * Get the system attributes for a file.
 */
int jfs_file_getattr(const char *path, struct stat *stbuf);

int jfs_file_utimes(const char *path, const struct timeval tv[2]);

int jfs_file_statfs(const char *path, struct statvfs *stbuf);

#endif
