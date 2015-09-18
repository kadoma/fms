#ifndef FMD_LOG_H_
#define FMD_LOG_H_

#include "fmd_event.h"

#define FMD_LOG_ERROR   0
#define FMD_LOG_FAULT   1
#define FMD_LOG_LIST    2

extern int fmd_log_event(fmd_event_t *);
extern void fmd_get_time(char *, time_t);

#endif //fmd_log.h
