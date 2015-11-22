/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    fmd_log.c
 * Author:      Inspur OS Team 
                wang.leibj@inspur.com
 * Date:        20xx-xx-xx
 * Description: the fault manager system event log function
 *
 ************************************************************/

#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

#include "wrap.h"
#include "fmd_event.h"
#include "fmd_log.h"
#include "logging.h"

void
fmd_get_time(char *date, time_t times)
{
	struct tm *tm = NULL;
	int year = 0, month = 0, day = 0;
	int hour = 0, min = 0, sec = 0;

	time(&times);
	tm = localtime(&times);
	year = tm->tm_year + 1900;
	month = tm->tm_mon + 1;
	day = tm->tm_mday;
	hour = tm->tm_hour;
	min = tm->tm_min;
	sec = tm->tm_sec;
	sprintf(date, "%d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, min, sec);
}

char *
get_logname(char *path, int type)
{
	struct tm *tm = NULL;
	int year = 0, month = 0, day = 0;
	time_t times;
	char date[20];
	memset(date, 0, 20*sizeof(char));

	time(&times);
	tm = gmtime(&times);
	year = tm->tm_year + 1900;
	month = tm->tm_mon + 1;
	day = tm->tm_mday;
	sprintf(date, "%d-%02d-%02d", year, month, day);
	
	switch (type) {
	case FMD_LOG_ERROR:
		sprintf(path, "%s-err", date);
		break;
	case FMD_LOG_FAULT:
		sprintf(path, "%s-flt", date);
		break;
	case FMD_LOG_LIST:
		sprintf(path, "%s-lst", date);
		break;
	default:
		break;
	}

	return path;
}

int
fmd_log_open(const char *dir, int type)
{
	char datename[PATH_MAX];
	char filename[PATH_MAX];
	int fd;
	struct dirent *dp;
	const char *logname;
	DIR *dirp;

	if (access(dir, 0) != 0) {
		wr_log("", WR_LOG_ERROR, 
			"FMD: Log Directory %s isn't exist, so create it.\n", dir);
		mkdir(dir, 0777 );
	}

	if ((dirp = opendir(dir)) == NULL) {
		wr_log("", WR_LOG_ERROR, 
			"FMD: failed to open directory %s\n", dir);
		return (-1);
	}

	logname = get_logname(datename, type);
	snprintf(filename, sizeof(filename), "%s/%s", dir, logname);

	if (access(filename, 0) != 0) {
		if ((fd = open(filename, O_RDWR | O_CREAT | O_SYNC | O_APPEND, S_IREAD | S_IWRITE)) < 0) {
			wr_log("", WR_LOG_ERROR, 
				"FMD: failed to create log file %s\n", filename);
			(void) closedir(dirp);
			return (-1);
		}
        } else {
		if ((fd = open(filename, O_RDWR | O_SYNC | O_APPEND)) < 0) {
			wr_log("", WR_LOG_ERROR, 
				"FMD: failed to open log file %s\n", filename);
			(void) closedir(dirp);
			return (-1);
		}
	}
        (void) closedir(dirp);

	return fd;
}

/**
 * fmd_log_close
 *
 * @param: log file fd
 * @return:
 */
void
fmd_log_close(int fd)
{
	if (fd >= 0 && close(fd) != 0)
		wr_log("", WR_LOG_ERROR, "FMD: failed to close log file: fd=%d.\n", fd);
}

int
fmd_log_write(int fd, const char *buf, int size)
{
	int len;

	if ((len = write(fd, buf, size)) <= 0) {
		wr_log("", WR_LOG_ERROR, 
			"FMD: failed to write log file: fd=%d.\n", fd);
		return (-1);
	}

    fsync(fd);
    return 0;
}
