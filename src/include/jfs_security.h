#ifndef JOINFS_JFS_SECURITY_H
#define JOINFS_JFS_SECURITY_H

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

#include <sys/types.h>

/*!
 * The file system chmod function.
 * \param path The system path.
 * \param mode The file mode.
 * \return Error code or 0.
 */
int jfs_security_chmod(const char *path, mode_t mode);

/*!
 * The file system chown function.
 * \param path The system path.
 * \param uid The user id.
 * \param gid The group id.
 * \return Error code or 0.
 */
int jfs_security_chown(const char *path, uid_t uid, gid_t gid);

/*!
 * The file system access function.
 * \param path The system path.
 * \param mask The access mask.
 * \return Error code or 0.
 */
int jfs_security_access(const char *path, int mask);

#endif
