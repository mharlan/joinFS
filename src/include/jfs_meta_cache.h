#ifndef JOINFS_JFS_META_CACHE_H
#define JOINFS_JFS_META_CACHE_H

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

/*!
 * Initialize the jfs_meta_cache.
 *
 * Called when joinFS gets mounted.
 */
void jfs_meta_cache_init();

/*!
 * Destroy the jfs_meta_cache.
 *
 * Called when joinFS gets dismount.
 */
void jfs_meta_cache_destroy();

/*!
 * Get metadata stored in the metadata cache.
 * \param path The file system path.
 * \param keyid The metadata tag id.
 * \param value The value returned.
 * \return Error code or 0.
 */
int jfs_meta_cache_get_value(const char *path, int keyid, char **value);

/*!
 * Add metadata to the metadata cache.
 * \param path The file system path.
 * \param keyid The metadata tag id.
 * \param value The metadata value.
 * \return Error code or 0.
 */
int jfs_meta_cache_add(const char *path, int keyid, const char *value);

/*!
 * Remove an item the metadata cache.
 * \param path The file system path.
 * \param keyid The metadata tag id.
 * \return Error code or 0.
 */
int jfs_meta_cache_remove(const char *path, int keyid);

#endif
