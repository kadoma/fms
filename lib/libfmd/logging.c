/*
 * logging.c
 * date 2015-7
 */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>

#include "logging.h"

static pthread_mutex_t logfile_mutex;
static FILE *logfile = NULL;
static char *filename = NULL;
static uint8_t log_to_file = 1;
static wr_loglevel_t loglevel = WR_LOG_MAX;

wr_loglevel_t wr_log_loglevel;

FILE *wr_log_logfile;

void
wr_log_logrotate(int debug)
{
    log_to_file = !debug;
}

wr_loglevel_t
wr_log_get_loglevel(void)
{
    return loglevel;
}

void
wr_log_set_loglevel(wr_loglevel_t level)
{
    wr_log_loglevel = loglevel = level;
}

int32_t
wr_log_init(const char *file)
{
    if(!file){
        fprintf(stderr, "wr_log_init: no filename specified\n");
        return -1;
    }

    pthread_mutex_init (&logfile_mutex, NULL);
    filename = strdup(file);
    if(!filename){
        fprintf (stderr, "wr_log_init: strdup error\n");
        return -1;
    }

    logfile = fopen (file, "a");
    if (!logfile){
        fprintf (stderr,"wr_log_init: failed to open logfile \"%s\" (%s)\n",
                                                        file, strerror (errno));
        return -1;
    }
    wr_log_logfile = logfile;
    return 0;
}

int32_t
_wr_log(const char *domain,
        const char *file,
        const char *function,
        int32_t line,
        wr_loglevel_t level, const char *fmt, ...)
{
    static char *level_strings[] = {"N", /* NONE */
                "C", /* CRITICAL */
                "E", /* ERROR */
                "W", /* WARNING */
                "T", /* TRACE (NB_LOG_NORMAL) */
                "D", /* DEBUG */
                ""};
    const char *basename;

    va_list ap;

    if(!logfile){
        fprintf(stderr, "no logfile set\n");
        return (-1);
    }

	if(!log_to_file)
	{
		pthread_mutex_lock(&logfile_mutex);
		
		va_start (ap, fmt);
		vfprintf(stdout, fmt, ap);
        va_end(ap);
		fprintf(stdout, "\n");
		
		fflush(stdout);
		
		pthread_mutex_unlock(&logfile_mutex);
		return 0;
	}
	
    if(!domain || !fmt)
        return (-1);

    if(log_to_file)
    {
        fclose(logfile);
        logfile = NULL;
		logfile = fopen(filename, "a");
			
        wr_log_logfile = logfile;
        if(!logfile){
            fprintf(stderr,"wr_log: failed to open logfile \"%s\" (%s) while logrotating\n",
                                                                filename,strerror (errno));
        return -1;
        }
    }

    if(level <= loglevel)
    {
        pthread_mutex_lock(&logfile_mutex);
        va_start (ap, fmt);
        time_t utime = time(NULL);
        struct tm *tm = localtime(&utime);
        char timestr[256];

        strftime(timestr, 256, "%Y-%m-%d %H:%M:%S", tm);
        /* strftime (timestr, sizeof(timestr), nl_langinfo (D_T_FMT), tm); */

        basename = strrchr(file, '/');
        if(basename)
            basename++;
        else
            basename = file;
        fprintf(logfile, "%s %s [%s:%d:%s] %s: ",
            timestr,
            level_strings[level],
            basename,
            line,
            function,
            domain);
        vfprintf(logfile, fmt, ap);
        va_end(ap);
        fprintf(logfile, "\n");
        fflush(logfile);

        pthread_mutex_unlock(&logfile_mutex);
    }
    return (0);
}
