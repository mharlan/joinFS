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

#ifndef JOINFS_JFS_DATAPATH_CACHE_H
#define JOINFS_JFS_DATAPATH_CACHE_H

void jfs_datapath_cache_init();

void jfs_datapath_cache_destroy();

int jfs_datapath_cache_add(int jfs_id, const char *datapath);

int jfs_datapath_cache_remove(int jfs_id);

int jfs_datapath_cache_get_datapath(int jfs_id, char **datapath);

int jfs_datapath_cache_change_id(int old_jfs_id, int new_jfs_id);

#endif
