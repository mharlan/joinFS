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

#ifndef JOINFS_JFS_UUID_H
#define JOINFS_JFS_UUID_H

#define JFS_UUID_LEN 37 /* 36 byte uuid + \0 */

/*
 * Generates a new unique joinFS id.
 *
 * Memory for the uuid must already be allocated with
 * jfs_create_uuid().
 */
int jfs_uuid_generate(char **uuid);

/*
  Generates a new unique datapath.
 */
int jfs_uuid_new_datapath(char **datapath);

#endif
