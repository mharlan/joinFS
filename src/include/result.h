#ifndef JOINFS_RESULT_H
#define JOINFS_RESULT_H

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

#include "sqlitedb.h"

#define JFS_FILENAME_MAX 1024
#define JFS_KEY_MAX      1024
#define JFS_VALUE_MAX    1024

/*!
 * Function to return the result of a query.
 * The result will be set based on the jfs_db_op enum value.
 * \param db_op The database operation.
 * \return Error code or 0.
 */
int jfs_db_result(struct jfs_db_op *db_op);

#endif
