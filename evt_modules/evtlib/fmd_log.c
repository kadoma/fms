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
	tm = gmtime(&times);
	year = tm->tm_year + 1900;
	month = tm->tm_mon + 1;
	day = tm->tm_mday;
	hour = tm->tm_hour;
	min = tm->tm_min;
	sec = tm->tm_sec;
	sprintf(date, "%d-%02d-%02d_%02d:%02d:%02d", year, month, day, hour, min, sec);
}

static char *
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

/**
 * fmd_log_open
 *
 * @param: /var/log/fm/serd.log
 		/var/log/fm/fault.log
 		/var/log/fm/list.log
 * @return: log file fd
 */
static int
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
static void
fmd_log_close(int fd)
{
	if (fd >= 0 && close(fd) != 0)
		wr_log("", WR_LOG_ERROR, 
			"FMD: failed to close log file: fd=%d.\n", fd);
}

static int
fmd_log_write(int fd, const char *buf, int size)
{
	int len;

	if ((len = write(fd, buf, size)) <= 0) {
		wr_log("", WR_LOG_ERROR, 
			"FMD: failed to write log file: fd=%d.\n", fd);
		return (-1);
	}

	return 0;
}

int
fmd_log_event(fmd_event_t *pevt)
{
	int type, fd;
	int tsize, ecsize, bufsize;
	char *dir = "/var/log/fms";
	char *eclass = NULL;
	char times[26];
	char buf[PATH_MAX];
	time_t evtime;
	uint64_t rscid = 0;
	uint64_t eid = 0;
	uint64_t uuid = 0;
	
	evtime = pevt->ev_create;
	eclass = pevt->ev_class;

	if (strncmp(eclass, "ereport.", 8) == 0) {
		type = FMD_LOG_ERROR;
		dir = "/var/log/fms/serd.log";
	} else if (strncmp(eclass, "fault.", 6) == 0) {
		type = FMD_LOG_FAULT;
		dir = "/var/log/fms/fault.log";
	} else if (strncmp(eclass, "list.", 5) == 0) {
		type = FMD_LOG_LIST;
		dir = "/var/log/fms/list.log";
	}

	if ((fd = fmd_log_open(dir, type)) < 0) {
		wr_log("", WR_LOG_ERROR, 
			"FMD: failed to record log for event: %s\n", eclass);
		return LIST_LOGED_FAILED;
	}

	fmd_get_time(times, evtime);
	memset(buf, 0, PATH_MAX);
	snprintf(buf, sizeof(buf), "\n%s\t%s\t0x%llx\t0x%llx\t0x%llx\n", 
		times, 
		eclass, 
		(long long unsigned int)rscid, 
		(long long unsigned int)eid, 
		(long long unsigned int)uuid);

	tsize = strlen(times) + 1;
	ecsize = strlen(eclass) + 1;
	bufsize = tsize + ecsize + 3*sizeof(uint64_t) + 11;

	if (fmd_log_write(fd, buf, bufsize) != 0) {
		wr_log("", WR_LOG_ERROR, 
			"FMD: failed to write log file for event: %s\n", eclass);
		return LIST_LOGED_FAILED;
	}
	fmd_log_close(fd);

	return LIST_LOGED_SUCCESS;
}
