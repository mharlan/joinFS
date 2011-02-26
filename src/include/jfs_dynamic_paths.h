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

#ifndef JFS_DYNAMIC_PATHS_H
#define JFS_DYNAMIC_PATHS_H

int jfs_dynamic_path_init(void);

/*
  Resolves a dynamic path into a datapath.

  Returns 0 on success, -ENOENT on failure.
 */
int jfs_dynamic_path_resolution(const char *path, char **resolved_path, int *datainode);

/*
  Add a dynamic file to the dynamic path hierarchy.

  Returns 0 on success, negative error code on failure.
 */
int jfs_dynamic_hierarchy_add_file(char *path, char *datapath, int datainode);

int jfs_dynamic_hierarchy_add_folder(char *path, char *datapath, int datainode);

#endif
