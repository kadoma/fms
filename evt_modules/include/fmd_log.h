#ifndef FMD_LOG_H_
#define FMD_LOG_H_

#include "fmd_event.h"

#define FMD_LOG_ERROR   1
#define FMD_LOG_FAULT   2
#define FMD_LOG_LIST    3

void
fmd_get_time(char *date, time_t times);

char *
get_logname(char *path, int type);

int
fmd_log_open(const char *dir, int type);

void
fmd_log_close(int fd);

int
fmd_log_write(int fd, const char *buf, int size);

#endif //fmd_log.h
