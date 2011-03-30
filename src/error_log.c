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

#if !defined(_REENTRANT)
#define	_REENTRANT
#endif

#include "error_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static FILE *log;
static const char *log_path = "/home/matt/git/joinFS/demo/error_log.txt";
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_init(void)
{
  log = fopen(log_path, "w+");
  if(!log) {
	printf("---ERROR---Open log file failed, path:%s\n", log_path);
    exit(EXIT_FAILURE);
  }
}

void log_destroy(void)
{
  fclose(log);
}

void log_error(const char * format, ...)
{
  va_list args;

  va_start(args, format);

  pthread_mutex_lock(&log_mutex);
  fprintf(log, "---ERROR---");
  vfprintf(log, format, args);
  pthread_mutex_unlock(&log_mutex);

  va_end(args);
}

void log_msg(const char * format, ...)
{
  va_list args;

  va_start(args, format);

  pthread_mutex_lock(&log_mutex);
  fprintf(log, "----MSG----");
  vfprintf(log, format, args);
  pthread_mutex_unlock(&log_mutex);

  va_end(args);
}
