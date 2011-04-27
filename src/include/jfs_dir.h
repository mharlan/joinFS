#ifndef _JOINFS_JFS_DIR_H
#define _JOINFS_JFS_DIR_H

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

#include <fuse.h>
#include <dirent.h>
#include <sys/types.h>

/*!
 * Makes a joinFS directory.
 * \param path The directory path.
 * \param mode The mode the directory is created in.
 * \return Error code or 0.
 */
int jfs_dir_mkdir(const char *path, mode_t mode);

/*!
 * Remove a joinFS directory.
 * \param path The directory path.
 * \return Error code or 0.
 */
int jfs_dir_rmdir(const char *path);

/*!
 * Open a joinFS directory.
 * \param path The new directory path.
 * \param d A pointer to the opened directory entry.
 * \return Error code or 0.
 */
int jfs_dir_opendir(const char *path, DIR **d);

/*!
 * Read the contents of a joinFS directory.
 * \param path The directory path.
 * \param dp The directory entry.
 * \param buf The readdir buffer.
 * \param filler The fuse directory filling function.
 * \return Error code or 0.
 */
int jfs_dir_readdir(const char *path, DIR *dp, void *buf, fuse_fill_dir_t filler);

#endif
