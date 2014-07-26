/*
 * fmd_main.c
 *
 *  Created on: Jan 14, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	main
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include <syslog.h>
#include <pthread.h>
#include <errno.h>
#include <linux/limits.h>

#include <fmd.h>
#include <fmd_api.h>
#include <fmd_ckpt.h>
#include <fmd_fmadm.h>
#include <fmd_module.h>

/* This is the most important global data holder.*/
/**
 * All global variables which share between dlls defined here.
 *
 */
fmd_t fmd __attribute__ ((section ("global")));

int fmd_count, dispq_count;

/**
 * @param
 * @return
 */
fmd_module_t *
fmd_module_init(const char *path, fmd_t *pfmd)
{
	void *handle;
	fmd_module_t * (*fmd_init)(const char *, fmd_t *);
	char *error;

	handle = dlopen(path, RTLD_LAZY);
	if(!handle) {
		error = dlerror();
		syslog(LOG_ERR, "dlopen %s\n", error);
		exit(EXIT_FAILURE);
	}

	dlerror();
	fmd_init = dlsym(handle, "fmd_init");
	error = dlerror();
	if(error != NULL) {
		syslog(LOG_ERR, "dlsym");
		exit(EXIT_FAILURE);
	}

	/* TODO */
	/* dlclose(handle); */

	/* execute */
	return (*fmd_init)(path, pfmd);
}


/**
 * fmd_module.h#fmd_init
 * fmd_init#
 * fmd_init#
 *
 */
fmd_module_t *
fmd_module_load(const char *path, fmd_t *pfmd)
{
	char *p = NULL;
	fmd_module_t *mp = NULL;

	/* basic checkup */
	p = strrchr(path, '.');
	if(p == NULL) {
		syslog(LOG_ERR, "Invalid path:%s", path);
		exit(EXIT_FAILURE);
	}

	/* load fmd_init */
	mp = fmd_module_init(path, pfmd);
	if(mp == NULL)
		goto out;
	list_add(&mp->list_fmd, &pfmd->fmd_module);

out:
	return (mp);
}


static void
fmd_module_loaddir(const char *dir, const char *suffix)
{
	char path[PATH_MAX];
	struct dirent *dp;
	const char *p;
	DIR *dirp;
	fmd_module_t *pm;

	if ((dirp = opendir(dir)) == NULL)
		return; /* failed to open directory; just skip it */

	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_name[0] == '.')
			continue; /* skip "." and ".." */

		p = strrchr(dp->d_name, '.');

		if (p != NULL && strcmp(p, ".conf") == 0)
			continue; /* skip .conf files */

		if (suffix != NULL && (p == NULL || strcmp(p, suffix) != 0))
			continue; /* skip files with the wrong suffix */

		(void) snprintf(path, sizeof (path), "%s/%s", dir, dp->d_name);
		(void) fmd_module_load(path, &fmd);
	}

	(void) closedir(dirp);
}


/**
 * fmd_load_esc
 *
 * @param
 * @return
 */
void
fmd_load_eversholt()
{
	char path[PATH_MAX], *error;
	void *handle;
	int (*_esc_main)(fmd_t *), ret;
	void (*__stat_esc)(void);

	memset(path, 0, sizeof(path));
	sprintf(path, "%s/%s", BASE_DIR, "libfmd_eversholt.so");

	dlerror();
	handle = dlopen(path, RTLD_LAZY);
	if(handle == NULL) {
		syslog(LOG_ERR, "dlopen");
		exit(-1);
	}

	dlerror();
	_esc_main = dlsym(handle, "esc_main");
	if((error = dlerror()) != NULL) {
		syslog(LOG_ERR, "dlsym");
		exit(-1);
	}

	ret = (*_esc_main)(&fmd);
	if(ret < 0) {
		syslog(LOG_ERR, "_esc_main");
		exit(-1);
	}

	fmd_debug;
	dlerror();
	__stat_esc = dlsym(handle, "_stat_esc");
	if((error = dlerror()) != NULL) {
		syslog(LOG_ERR, "dlsym");
		exit(-1);
	}
	(*__stat_esc)();

	dlclose(handle);
}


/**
 * fmd_load_topo
 *
 * @param
 * @return
 */
void
fmd_load_topo()
{
	char path[PATH_MAX], *error;
	void *handle;
	int (*fmd_topo)(fmd_t *), ret;
	void (*stat_topo)(void);
	int (*topo_tree_create)(fmd_t *);

	memset(path, 0, sizeof(path));
	sprintf(path, "%s/%s", BASE_DIR, "libfmd_topo.so");

	handle = dlopen(path, RTLD_LAZY);
	if(handle == NULL) {
		syslog(LOG_ERR, "dlopen");
		exit(-1);
	}

	dlerror();
	fmd_topo = dlsym(handle, "_fmd_topo");
	if((error = dlerror()) != NULL) {
		syslog(LOG_ERR, "dlsym");
		exit(-1);
	}

	ret = (*fmd_topo)(&fmd);
	if(ret < 0) {
		syslog(LOG_ERR, "_fmd_topo");
		exit(-1);
	}

	fmd_debug;
	dlerror();
	stat_topo = dlsym(handle, "_stat_topo");
	if((error = dlerror()) != NULL) {
		syslog(LOG_ERR, "dlsym");
		exit(-1);
	}
	(*stat_topo)();

	dlerror();
	topo_tree_create = dlsym(handle, "topo_tree_create");
	if((error = dlerror()) != NULL) {
		syslog(LOG_ERR, "dlsym");
		exit(-1);
	}

	ret = (*topo_tree_create)(&fmd);
	if(ret < 0) {
		syslog(LOG_ERR, "topo_tree_create");
		exit(-1);
	}

	dlclose(handle);
}


/**
 * fmd_timer_fire
 *
 * @param
 * @return
 */
void
fmd_timer_fire(fmd_timer_t *ptimer)
{
	pthread_t pid = ptimer->tid;

	pthread_mutex_lock(&ptimer->mutex);

	fmd_debug;

	pthread_cond_signal(&ptimer->cv);
	pthread_mutex_unlock(&ptimer->mutex);
}


/**
 * slave_fmd_run
 *
 * @param
 * @return
 */
static void *
slave_fmd_run(void *arg)
{
	/* slave fmd thread test whether fmd is alive */
	while(1) {
		fmd_timer(1);

		fmd_count++;
//		dispq_count++;

		printf("SLAVE FMD: fmd_count = %d, dispq_count = %d\n",
				fmd_count, dispq_count);

		if (fmd_count == 10 || dispq_count == 10) {
			printf("SLAVE FMD: EXIT! fmd_count = %d, dispq_count = %d\n",
				fmd_count, dispq_count);
			printf("SLAVE FMD: fmd/dispq thread not exist or already exit.\n");
			/* fms need restart */
			/* TODO */

			return NULL;
		}
		/* fmd & dispq thread still alive */
	}
}


/**
 * Create a slave fmd thread.
 *      
 */     
static void
slave_pthread_init(void)
{
	pthread_t tid;
	int ret;
	pthread_attr_t attr;

	/* Initialize with default attribute */
	ret = pthread_attr_init(&attr);
	if(ret < 0) {
		printf("FMD: failed to initialize the slave fmd thread.\n");
		syslog(LOG_ERR, "%s\n", strerror(errno));
		exit(errno);
	}

	/* create new thread */
	ret = pthread_create(&tid, &attr, slave_fmd_run, NULL);
	if(ret < 0) {
		printf("FMD: failed to create the slave fmd thread.\n");
		syslog(LOG_ERR, "pthread_create");
		exit(errno);
	}

	pthread_attr_destroy(&attr);

	return;
}


int
main(int argc, char* argv[])
{
	int ret = 0;
	int count = 0;
	uint64_t ckpt_interval = 0;
	const char *dirp = NULL;
	struct list_head *pos;
	pthread_t pid;
	fmd_timer_t *ptimer;
	fmd_count = 0;
	dispq_count = 0;

	/* Initialize the logging interface */
	openlog(DAEMON_NAME, LOG_PID, LOG_LOCAL1);
#ifdef FMD_DEBUG
	setlogmask(LOG_UPTO(LOG_DEBUG));
#else
	setlogmask(LOG_UPTO(LOG_INFO));
#endif
	syslog(LOG_INFO, "starting");

	/* Daemonize */
	daemonize( "/var/lock/subsys/" DAEMON_NAME );

	/* fmd global variable */
	INIT_LIST_HEAD(&fmd.fmd_module);
	INIT_LIST_HEAD(&fmd.fmd_timer);
	INIT_LIST_HEAD(&fmd.list_case);
	fmd_dispq_init(&fmd.fmd_dispq);

	/* TSD */
	pthread_key_create(&t_module, NULL);
	pthread_key_create(&t_fmd, NULL);

	/* execute esc & topo helper routines */
	fmd_load_topo();
	fmd_load_eversholt();

	/* load built-in modules */
	fmd_dispq_load();

	/* load plugin modules */
	dirp = (const char *)plugindir();
	fmd_module_loaddir(dirp, ".so");

	/* read ckpt.conf & restore to last checkpoint */
	ckpt_interval = fmd_ckpt_conf();
	fmd_ckpt_restore();

	/* start slave fmd thread */
	slave_pthread_init();

	/* load fmd adm module */
//	fmd_adm_init();

	while(1) {
		/**
		 * The granulaity of scheduling
		 * 1second
		 */
		sleep(1);

		fmd_count = 0;
		count++;
		if (count == ckpt_interval) {
			fmd_ckpt_save();
			count = 0;
		}

		list_for_each(pos, &fmd.fmd_timer) {
			ptimer = list_entry(pos, fmd_timer_t, list);
			ptimer->ticks++;

			if(ptimer->ticks == ptimer->interval) {
				fmd_timer_fire(ptimer);
				ptimer->ticks = 0;
			}
		}
	}

	return 0;
}

void
_fmd_fini()
{
	syslog(LOG_NOTICE, "terminated");
	closelog();
}
