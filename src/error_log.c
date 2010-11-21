/*
  joinFS: Error logging utility
  Matthew Harlan <mharlan@gwmail.gwu.edu>

  Sends all errors to a log file.
*/

#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>

#include "include/error_log.h"

static const char *log_path = "/home/joinfs/demo/error_log.txt";
static FILE *log;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_init(void)
{
  log = fopen(log_path, "w+");
  if(log == 0) {
    printf("Open log file failed, path:%s\n", log_path);
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
  vfprintf(log, format, args);
  pthread_mutex_unlock(&log_mutex);

  va_end(args);
}


