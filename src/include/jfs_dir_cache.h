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

#ifndef JOINFS_JFS_DIRECTORY_CACHE_H
#define JOINFS_JFS_DIRECTORY_CACHE_H

/*
 * Initialize the jfs_file_cache.
 *
 * Called when joinFS gets mounted.
 */
void jfs_directory_cache_init();

/*
 * Destroy the jfs_file_cache.
 *
 * Called when joinFS gets dismount.
 */
void jfs_directory_cache_destroy();

/*
 * Get a datainode from the file cache.
 *
 * Returns 0 if not in the cache.
 */
int jfs_directory_cache_get(int dirinode);

/*
 * Add a symlink to the jfs_file_cache.
 */
int jfs_directory_cache_add(int keyid, const char *keytext);

/*
 * Remove a symlink reference from the file cache.
 */
int jfs_directory_cache_remove(const char *keytext);

#endif
