#ifndef JOINFS_ERROR_LOG_H
#define JOINFS_ERROR_LOG_H

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
 * Opens the joinFS log file.
 */
void log_init(void);

/*!
 * Closes the joinFS log file.
 */
void log_destroy(void);

/*!
 * Log an error message to the log file.
 * \param format the format string
 * \param ... the variables to format
 */
void log_error(const char *format, ...);

/*!
 * Log a message to the log file.
 * \param format the format string
 * \param ... the variables to format
 */
void log_msg(const char *format, ...);

#endif
