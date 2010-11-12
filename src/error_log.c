/*
  joinFS: Error logging utility
  Matthew Harlan <mharlan@gwmail.gwu.edu>

  Sends all errors to a log file.
*/
#include <stdarg.h>
#include <stdio.h>

#include "error_log.h"

static const char *log_path = "/home/matt/joinFS/bootcamp/error_log.txt";
static FILE *log;

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
  vfprintf(log, format, args);
  va_end(args);
}


