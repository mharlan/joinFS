#ifndef JOINFS_JFS_DIR_QUERY_H
#define JONIFS_JFS_DIR_QUERY_H

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
 * Builds the dynamic folder query from metadata.
 * \param path The joinfs directory path.
 * \param realpath The real file system directory path.
 * \param is_folders Return value that specifies whether or not the query results should be folders.
 * \param query The returned SQL query.
 * \return Error code or 0.
 */
int jfs_dir_query_builder(const char *path, const char *realpath, int *is_folders, char **query);

#endif
