/*
 * fmd_module.c
 *
 *  Created on: Feb 25, 2010
 *      Author: Inspur OS Team
 * 
 *  Description:
 *  	fmd_module.c
 *		Add module lock
 */

#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <syslog.h>
#include <linux/limits.h>
#include <fmd_module.h>

/**
 * fmd_module_conf
 *
 * @param
 * @return
 */
int
fmd_module_conf(fmd_module_t *pm, const char *path, int flag)
{
	int ret = 0;
	char buf[PATH_MAX], *error;
	void *handle;
	int	(*_module_conf)(fmd_module_t *, const char *, int);

	/* basic setup */
	INIT_LIST_HEAD(&pm->list_fmd);
	INIT_LIST_HEAD(&pm->list_dispq);
	INIT_LIST_HEAD(&pm->list_eclass);

	/* path */
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%s/%s", BASE_DIR, "libfmd_module.so");

	/* dlopen */
	dlerror();
	handle = dlopen(buf, RTLD_LAZY);
	if(handle == NULL)
		goto fail;
	
	dlerror();
	_module_conf = dlsym(handle, "module_conf");
	if((error = dlerror()) != NULL)
		goto out;

	ret = (*_module_conf)(pm, path, flag);
	if(ret < 0)
		goto out;
	return 0;

out:
	dlclose(handle);
fail:
	return -1;
}

/**
 * fmd_module_lock
 *
 * @param
 * @return
 */
int
fmd_module_lock(pthread_mutex_t *mutex)
{
	if (pthread_mutex_lock(mutex) == 0)
		return 1;
	else
		return 0;
}

/**
 * fmd_module_unlock
 *
 * @param
 * @return
 */
int
fmd_module_unlock(pthread_mutex_t *mutex)
{   
	if (pthread_mutex_unlock(mutex) == 0)
		return 1;
	else
		return 0;
}   

