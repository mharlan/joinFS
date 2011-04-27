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

#ifndef JOINFS_JFS_META_H
#define JOINFS_JFS_META_H

#include <sys/types.h>

/*!
 * Add a metadata tag and value to a file system item.
 * \param path The file system path.
 * \param key The metadata tag.
 * \param value The metadata value.
 * \param size The metadata value size.
 * \param flags Metadata flags.
 * \return Error code or 0.
 */
int jfs_meta_setxattr(const char *path, const char *key, const char *value,
					  size_t size, int flags);

/*!
 * Get the metadata value tied to a file system item by a metadat tag.
 * \param path The file system path.
 * \param key The metadata tag.
 * \param value The metadata value.
 * \param size The metadata value size.
 * \return A negative error code or the size of the value returned.
 */
int jfs_meta_getxattr(const char *path, const char *key, void *value,
					  size_t size);

/*!
 * Skips processing that jfs_meta_getxattr does.
 * \param path The file system path.
 * \param key The metadata tag.
 * \param value The metadata value.
 * \return A negative error code or the size of the value returned.
 */
int jfs_meta_do_getxattr(const char *path, const char *key, char **value);

/*!
 * A list of all metadata tags tied to a file system item.
 * \param path The file system path.
 * \param list The metadata tag list.
 * \param size The metadata value size.
 * \return A negative error code or the size of the list returned.
 */
int jfs_meta_listxattr(const char *path, char *list, size_t size);

/*!
 * Remove a metadata tag and value from a file system item.
 * \param path The file system path.
 * \param key The metadata tag.
 * \return Error code or 0.
 */
int jfs_meta_removexattr(const char *path, const char *key);

#endif
