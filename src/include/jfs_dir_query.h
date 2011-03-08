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

/*
  Builds the dynamic directory query.

  Returns -EBADMSG for a badly formatted query.
  Also returns -ENOMEM.
 */
int jfs_dir_query_builder(const char *orig_path, const char *path, int *is_folders, char **query);

#endif
