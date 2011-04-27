#ifndef JOINFS_JFS_DATAPATH_CACHE_H
#define JOINFS_JFS_DATAPATH_CACHE_H

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
 * Initialize the joinFS data path cache.
 */
void jfs_datapath_cache_init();

/*!
 * Destory the joinFS data path cache.
 */
void jfs_datapath_cache_destroy();

/*!
 * Log the contents of the data path cache.
 */
void jfs_datapath_cache_log();

/*!
 * Add a path to the data path cache.
 * \param jfs_id The joinFS ID for the data path.
 * \param datapath The data path being added.
 * \return Error code or 0.
 */
int jfs_datapath_cache_add(int jfs_id, const char *datapath);

/*!
 * Remove a path from the data path cache.
 * \param jfs_id The joinFS ID for the data path.
 * \return Error code or 0.
 */
int jfs_datapath_cache_remove(int jfs_id);

/*!
 * Get a data path from the data path cache.
 * \param jfs_id The joinFS ID for the data path.
 * \param datapath Where the data path is returned.
 * \return Error code or 0.
 */
int jfs_datapath_cache_get_datapath(int jfs_id, char **datapath);

/*!
 * Add a path to the data path cache.
 * \param jfs_id The old joinFS ID for the data path.
 * \param jfs_id The new joinFS ID for the data path.
 * \return Error code or 0.
 */
int jfs_datapath_cache_change_id(int old_jfs_id, int new_jfs_id);

#endif
