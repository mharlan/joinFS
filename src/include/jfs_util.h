#ifndef JOINFS_JFS_UTIL_H
#define JOINFS_JFS_UTIL_H

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

#include <sys/types.h>

/*!
 * Get the inode number associated with a path.
 * \param path The system path.
 * \return The inode number or a negative error code.
 */
int jfs_util_get_inode(const char *path);

/*!
 * Returns both the inode and mode for the path.
 * \param path The system path.
 * \param inode The inode number returned.
 * \param mode The file mode returned.
 * \return Error code or 0.
 */
int jfs_util_get_inode_and_mode(const char *path, int *inode, mode_t *mode);

/*!
 * Does the path exist on the file system?
 * \param path The system path.
 * \return 1 if it exists, 0 if it does not.
 */
int jfs_util_is_realpath(const char *path);

/*!
 * Is the path a dynamic path?
 * \param path The system path.
 * \return 1 if dynamic, 0 if not
 */
int jfs_util_is_path_dynamic(const char *path);

/*!
 * Get the datapath associated with a path.
 * \param path The system path.
 * \param datapath The data path returned.
 * \return Errror code or 0.
 */
int jfs_util_get_datapath(const char *path, char **datapath);

/*!
 * Get both the datapath and datainode associated with a file.
 * \param path The system path.
 * \param datapath The data path returned.
 * \param datainode The data inode returned.
 * \return Errror code or 0.
 */
int jfs_util_get_datapath_and_datainode(const char *path, char **datapath, int *datainode);

/*!
 * Retrieve the datainode associated with a path.
 * \param path The system path.
 * \return The inode number or a negative error code.
 */
int jfs_util_get_datainode(const char *path);

/*! 
 * Returns a pointer to the filename portion of a path.
 * \param path The system path.
 * \return NULL if there is no filename, otherwise a pointer.
 */
char *jfs_util_get_filename(const char *path);

/*!
 * Allocate and copies the new subpath.
 * \param path The system path.
 * \param subpath The sub path returned.
 * \return Error code or 0.
 */
int jfs_util_get_subpath(const char *path, char **subpath);

/*!
 * Get the full path for a new filename.
 * \param path The system path.
 * \param filename The file name.
 * \param newpath The resulting path.
 * \return Error code or 0.
 */
int jfs_util_change_filename(const char *path, const char *filename, char **newpath);

/*!
 * Returns the id for the specified metadata tag.
 * \param key The metadata tag name.
 * \return Negative error code or the metadata tag id.
 */
int jfs_util_get_keyid(const char *key);

/*!
 * Remove the last path item from a path.
 * \param path The system path.
 * \return Error code or 0.
 */
int jfs_util_strip_last_path_item(char *path);

/*!
 * Returns a pointer to the last path item.
 * \param path The system path.
 * \return A pointer or NULL.
 */
char *jfs_util_get_last_path_item(const char *path);

/*!
 * Resolves dynamic paths for new filesystem items.
 * \param path The system path.
 * \param new_path Returns the resolved new path.
 * \return Error code or 0.
 */
int jfs_util_resolve_new_path(const char *path, char **new_path);

#endif
