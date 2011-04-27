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

/*!
 * Create a joinFS file.
 * \param path The joinFS path.
 * \param mode The file creation mode.
 * \param rdev The file dev_t.
 * \return Error code or 0.
 */
int jfs_file_mknod(const char *path, mode_t mode, dev_t rdev);

/*!
 * Deletes a joinFS link.
 * \param path The link path.
 * \return Error code or 0.
 */
int jfs_file_unlink(const char *path);

/*!
 * Perform a rename on a joinFS file.
 * \param from The original path.
 * \param to The new path.
 * \return Error code or 0.
 */
int jfs_file_rename(const char *from, const char *to);

/*!
 * Truncate a joinFS file.
 * \param path The file path.
 * \param size The truncate offset size.
 * \return Error code or 0.
 */
int jfs_file_truncate(const char *path, off_t size);

/*!
 * Open or create a joinFS file.
 * \param path The file path.
 * \param flags The open flags.
 * \param mode The create mode if creating.
 * \return Error code or 0.
 */
int jfs_file_open(const char *path, int flags, mode_t mode);

/*!
 * Get the system attributes for a file.
 * \param path The system path.
 * \param stbuf The attribute buffer.
 * \return Error code or 0.
 */
int jfs_file_getattr(const char *path, struct stat *stbuf);

/*!
 * Get access time information for a file.
 * \param path The system path.
 * \param tv The time value buffer.
 * \return Error code or 0.
 */
int jfs_file_utimes(const char *path, const struct timeval tv[2]);

/*!
 * Get the file system attribute buffer.
 * \param path The system path.
 * \param stbuf The attribute buffer.
 * \return Error code or 0.
 */
int jfs_file_statfs(const char *path, struct statvfs *stbuf);

/*!
 * Read the contents of a link.
 * \param path The system path.
 * \param buf The result buffer.
 * \param size The result buffer size.
 * \return Error code or 0.
 */
int jfs_file_readlink(const char *path, char *buf, size_t size);

/*!
 * Create a symlink.
 * \param from The source path.
 * \param to The new location path.
 * \return Error code or 0.
 */
int jfs_file_symlink(const char *from, const char *to);

/*!
 * Creat a hardlink.
 * \param from The source path.
 * \param to The new location path.
 * \return Error code or 0.
 */
int jfs_file_link(const char *from, const char *to);

/*!
 * Add a file to the database.
 * \param inode The file system inode number.
 * \param path The system path.
 * \param filename The filename.
 * \return Error code or 0.
 */
int jfs_file_db_add(int inode, const char *path, const char *filename);

#endif
