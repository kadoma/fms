/*
 * fmd_log.c
 *
 *  Created on: Feb 25, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	fmd_log.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <fmd_event.h>
#include <fmd_log.h>

#if 0
static fmd_log_t *
fmd_log_xopen(const char *root, const char *name, const char *tag, int oflags)
{
	fmd_log_t *lp = fmd_zalloc(sizeof (fmd_log_t), FMD_SLEEP);

	char buf[PATH_MAX];
	char *slash = "/";
	size_t len;
	int err;

	(void) pthread_mutex_init(&lp->log_lock, NULL);
	(void) pthread_cond_init(&lp->log_cv, NULL);
	(void) pthread_mutex_lock(&lp->log_lock);

	if (strcmp(root, "") == 0)
		slash = "";
	len = strlen(root) + strlen(name) + strlen(slash) + 1; /* for "\0" */
	lp->log_name = fmd_alloc(len, FMD_SLEEP);
	(void) snprintf(lp->log_name, len, "%s%s%s", root, slash, name);
	lp->log_tag = fmd_strdup(tag, FMD_SLEEP);
	(void) fmd_conf_getprop(fmd.d_conf, "log.minfree", &lp->log_minfree);

	if (strcmp(lp->log_tag, FMD_LOG_ERROR) == 0)
		lp->log_flags |= FMD_LF_REPLAY;

	if (strcmp(lp->log_tag, FMD_LOG_XPRT) == 0)
		oflags &= ~O_SYNC;

top:
	if ((lp->log_fd = open64(lp->log_name, oflags, 0644)) == -1 ||
	    fstat64(lp->log_fd, &lp->log_stat) == -1) {
		fmd_error(EFMD_LOG_OPEN, "failed to open log %s", lp->log_name);
		fmd_log_close(lp);
		return (NULL);
	}

	/*
	 * If our open() created the log file, use libexacct to write a header
	 * and position the file just after the header (EO_TAIL).  If the log
	 * file already existed, use libexacct to validate the header and again
	 * position the file just after the header (EO_HEAD).  Note that we lie
	 * to libexacct about 'oflags' in order to achieve the desired result.
	 */
	if (lp->log_stat.st_size == 0) {
		err = fmd_log_open_exacct(lp, EO_VALID_HDR | EO_TAIL,
		    O_CREAT | O_WRONLY) || fmd_log_write_hdr(lp, tag);
	} else {
		err = fmd_log_open_exacct(lp, EO_VALID_HDR | EO_HEAD,
		    O_RDONLY) || fmd_log_check_hdr(lp, tag);
	}

	/*
	 * If ea_fdopen() failed and the log was pre-existing, attempt to move
	 * it aside and start a new one.  If we created the log but failed to
	 * initialize it, then we have no choice but to give up (e.g. EROFS).
	 */
	if (err) {
		fmd_error(EFMD_LOG_OPEN,
		    "failed to initialize log %s", lp->log_name);

		if (lp->log_flags & FMD_LF_EAOPEN) {
			lp->log_flags &= ~FMD_LF_EAOPEN;
			(void) ea_close(&lp->log_ea);
		}

		(void) close(lp->log_fd);
		lp->log_fd = -1;

		if (lp->log_stat.st_size != 0 && snprintf(buf,
		    sizeof (buf), "%s-", lp->log_name) < PATH_MAX &&
		    rename(lp->log_name, buf) == 0) {
			TRACE((FMD_DBG_LOG, "mv %s to %s", lp->log_name, buf));
			if (oflags & O_CREAT)
				goto top;
		}

		fmd_log_close(lp);
		return (NULL);
	}

	lp->log_refs++;
	(void) pthread_mutex_unlock(&lp->log_lock);

	return (lp);
}
#endif

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
 * @param: /var/log/fm/errlog or /var/log/fm/fltlog or /var/log/fm/lstlog
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
		printf("FMD: Log Directory %s isn't exist, so create it.\n", dir);
		mkdir(dir, 0777 );
	}

	if ((dirp = opendir(dir)) == NULL) {
		printf("FMD: failed to open directory %s\n", dir);
		return (-1);
	}

	logname = get_logname(datename, type);
	snprintf(filename, sizeof(filename), "%s/%s", dir, logname);

	if (access(filename, 0) != 0) {
		if ((fd = open(filename, O_RDWR | O_CREAT | O_SYNC | O_APPEND, S_IREAD | S_IWRITE)) < 0) {
			printf("FMD: failed to create log file %s\n", filename);
			(void) closedir(dirp);
			return (-1);
		}
        } else {
		if ((fd = open(filename, O_RDWR | O_SYNC | O_APPEND)) < 0) {
			printf("FMD: failed to open log file %s\n", filename);
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
		printf("FMD: failed to close log file: fd=%d.\n", fd);
}

/**
 * fmd_log_write
 *
 * @param
 * @return
 */
static int
fmd_log_write(int fd, const char *buf, int size)
{
	int len;

	if ((len = write(fd, buf, size)) <= 0) {
		printf("FMD: failed to write log file: fd=%d.\n", fd);
		return (-1);
	}

	return 0;
}

/**
 * fmd_log_event
 *
 * @param
 * @return
 */
void
fmd_log_event(fmd_event_t *pevt)
{
	int type, fd;
	int tsize, ecsize, bufsize;
	char *dir = "/var/log/fm";
	char *eclass = NULL;
	char times[26];
	char buf[PATH_MAX];
	time_t evtime;
	uint64_t rscid;
	uint64_t eid;
	uint64_t uuid;
	
	evtime = pevt->ev_create;
	eclass = pevt->ev_class;
	rscid = pevt->ev_rscid;
	eid = pevt->ev_eid;
	uuid = pevt->ev_uuid;
	
	if (strncmp(eclass, "ereport.", 8) == 0) {
		type = FMD_LOG_ERROR;
		dir = "/var/log/fm/errlog";
	} else if (strncmp(eclass, "fault.", 6) == 0) {
		type = FMD_LOG_FAULT;
		dir = "/var/log/fm/fltlog";
	} else if (strncmp(eclass, "list.", 5) == 0) {
		type = FMD_LOG_LIST;
		dir = "/var/log/fm/lstlog";
	}

	if ((fd = fmd_log_open(dir, type)) < 0) {
		printf("FMD: failed to record log for event: %s\n", eclass);
		return;
	}

	fmd_get_time(times, evtime);
	snprintf(buf, sizeof(buf), "\n%s\t%s\t0x%llx\t0x%llx\t0x%llx\n", times, eclass, rscid, eid, uuid);

	tsize = strlen(times) + 1;
	ecsize = strlen(eclass) + 1;
	bufsize = tsize + ecsize + 3*sizeof(uint64_t) + 11;

	if (fmd_log_write(fd, buf, bufsize) != 0) {
		printf("FMD: failed to write log file for event: %s\n", eclass);
		return;
	}

	fmd_log_close(fd);
}

#if 0
/**
 * read log file
 *
 * @param
 * @return
 */
fmd_log_t *
fmd_read_logfile()
{




}
#endif
