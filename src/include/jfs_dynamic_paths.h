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
int jfs_dynamic_path_resolution(const char *path, char **resolved_path, int *jfs_id);

/*
  Add a dynamic file to the dynamic path hierarchy.

  Returns 0 on success, negative error code on failure.
 */
int jfs_dynamic_hierarchy_add_file(const char *path, const char *datapath, int jfs_id);

/*
 * Add a folder to the dynamic hierarchy.
 */
int jfs_dynamic_hierarchy_add_folder(const char *path, const char *datapath);

/*
  Rename the file or folder at path to filename.
 */
int jfs_dynamic_hierarchy_rename(const char *path, const char *filename);

/*
  Destroy the entire dynamic hierarchy.
 */
int jfs_dynamic_hierarchy_destroy(void);

/*
  Removes a file from the dynamic hierarchy.
 */
int jfs_dynamic_hierarchy_unlink(const char *path);

/*
  Removes a directory from the dynamic hierachy.

  The directory must be empty. 
  Returns -ENOTEMPTY, -ENOMEM;

  Use jfs_hierarchy_invalidate_folder to recursively delete the
  contents of a folder.
 */
int jfs_dynamic_hierarchy_rmdir(const char *path);

/*
  Delete the contents of the folder from the hierarchy.

  The folder itself remains.
 */
int jfs_dynamic_hierarchy_invalidate_folder(const char *path);

#endif
