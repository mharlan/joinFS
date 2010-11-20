#ifndef JOINFS_ERROR_LOG_H
#define JOINFS_ERROR_LOG_H

void log_init(void);
void log_destroy(void);
void log_error(const char *format, ...);

#endif
