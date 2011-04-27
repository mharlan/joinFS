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

#ifndef JOINFS_JFS_KEY_CACHE_H
#define JOINFS_JFS_KEY_CACHE_H

/*!
 * Initialize the key cache.
 */
void jfs_key_cache_init();

/*!
 * Destroy the key cache.
 */
void jfs_key_cache_destroy();

/*!
 * Get a kyeid from the key cache.
 * \param keytext The metadata tag name.
 * \return Error code or 0.
 */
int jfs_key_cache_get_keyid(const char *keytext);

/*!
 * Add a key to the key cache.
 * \param keyid The joinFS metadata tag id.
 * \param keytext The joinfs metadata tag name.
 * \return Error code or 0.
 */
int jfs_key_cache_add(int keyid, const char *keytext);

/*!
 * Remove a key from the key cache.
 * \param keytext The metadata tag name.
 * \return Error code or 0.
 */
int jfs_key_cache_remove(const char *keytext);

#endif
