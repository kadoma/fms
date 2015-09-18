#ifndef __FM_DISKSMARTDECT_H
#define __FM_DISKSMARTDECT_H

#ifndef LINE_MAX
#define LINE_MAX 	1024
#endif

#ifndef TEMPERATURE_LIMIT
#define TEMPERATURE_LIMIT 	70
#endif

int DoesSmartWork(const char *path);
int disk_unhealthy_check(const char *path);
int disk_temperature_check(const char *path);
#endif
