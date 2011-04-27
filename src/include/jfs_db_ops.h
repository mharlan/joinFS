#ifndef JOINFS_JFS_DB_OPS_H
#define JOINFS_JFS_DB_OPS_H

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
 * Enumerator used to determine what kind
 * of database operation is being executed.
 */
enum jfs_db_ops {
  jfs_write_op,
  jfs_multi_write_op,
  jfs_key_cache_op,
  jfs_meta_cache_op,
  jfs_datapath_cache_op,
  jfs_listattr_op,
  jfs_readdir_op,
  jfs_dynamic_file_op
};

#endif
