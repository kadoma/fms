/*
 * fmd_api.c
 *
 *  Created on: Jan 19, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	fmd api
 *		add thread timer
 */

#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <syslog.h>
#include <sys/select.h>
#include <linux/limits.h>
#include <fmd.h>
#include <fmd_list.h>
#include <fmd_module.h>


/**
 * Get plugin modules' residence directory.
 *
 * @return
 * 	char *
 * 	NULL-terminated.
 */
const char *
plugindir(void)
{
	char *dirp = (char *)malloc(PATH_MAX);

	assert(dirp != NULL);
	memset(dirp, 0, PATH_MAX);
	sprintf(dirp, "%s/%s/%s", \
			BASE_DIR, FMD_DIR, PLUGIN_DIR);

	return dirp;
}


/**
 * get module pointer through pid
 *
 * @param
 * @return
 */
fmd_module_t *
getmodule(fmd_t *pfmd, pthread_t pid)
{
	fmd_module_t *pm;
	struct list_head *pos;

	list_for_each(pos, &pfmd->fmd_module) {
		pm = list_entry(pos, fmd_module_t, list_fmd);
		syslog(LOG_NOTICE, "pm->mod_pid=%d\n", pm->mod_pid);
		if(pm->mod_pid == pid) {
			return pm;
		}
	}

	return NULL;
}


/**
 * fmd sub-thread's timer
 *
 * @param
 * @return
 */
unsigned
fmd_timer(unsigned nsec)
{
	int n;
	unsigned slept;
	time_t start, end;
	struct timeval tv;
	
	tv.tv_sec = nsec;
	tv.tv_usec = 0;
	time(&start);
	n = select(0, NULL, NULL, NULL, &tv);
	if (n == 0)
		return (0);
	time(&end);
	slept = end - start;
	if (slept >= nsec)
		return (0);
	return (nsec - slept);
}

