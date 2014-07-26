/*
 * fmd_conf.c
 *
 *  Created on: Jan 18, 2011
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	fmd_conf.c
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <dlfcn.h>
#include <syslog.h>
#include <errno.h>
#include <limits.h>
#include <linux/limits.h>
#include <fmd_list.h>
#include <fmd_dispq.h>
#include <fmd_module.h>
#include "conf.h"

/* global defs */
fmd_conf_t *pconf;

/**
 * Get module name
 *
 * @return
 * 	char *
 * 	The last name
 * 	i.g /usr/lib/fm/fmd/plugins/evtsrc.disk.so
 * 	=>evtsrc.disk
 */
static char *
fmd_module_name(const char *path)
{
	char *pslash, *pdot;
	char *str;

	pslash = strrchr(path, '/');
	str = strdup(pslash + 1);
	pdot = strrchr(str, '.');
	*pdot = '\0';

	return str;
}


/**
 * Get canonical conf filename
 *
 * @return
 * 	char *
 * 	The canonical conf name
 *  Null Terminated
 * 	i.g /usr/lib/fm/fmd/plugins/evtsrc.disk.so
 * 	=>/usr/lib/fm/fmd/plugins/evtsrc.disk.conf
 */
static char *
fmd_module_confname(const char *path)
{
	char *pdot, *p;
	int len = strlen(path);

	/**
	 * 2: strlen(conf) - strlen(so)
	 * 1: NULL Terminated
	 */
	p = (char *)malloc(len + 2 + 1);
	assert(p != NULL);
	memset(p, 0, len + 2 + 1);
	memcpy(p, path, len);

	pdot = strrchr(p, '.');
	sprintf(pdot + 1, "conf");

	return p;
}


/**
 * conf_add_sub
 *
 * @param
 * @return
 */
static void
conf_add_sub(char *eclass)
{
	struct subitem *p = (struct subitem *)
			malloc(sizeof(struct subitem));

	assert(p != NULL);
	p->si_eclass = strdup(eclass);
	list_add(&p->si_list, pconf->list_eclass);
}


/**
 * parse_evtsrc_conf
 *
 * @param
 * @return
 */
static int
parse_evtsrc_conf(char *filename)
{
	FILE *fp = NULL;
	char buf[64];
	char buff[64];

	memset(buf, 0, 64);
	memset(buff, 0, 64);

	if ((fp = fopen(filename, "r")) == NULL) {
		printf("FMD: failed to open conf file %s\n", filename);
		return (-1);
	}

	while (fgets(buf, sizeof(buf), fp)) {
		if (strncmp(buf, "interval ", 9) == 0) {
			strcpy(buff, &buf[9]);
			pconf->cf_interval = atoi(buff);

			fclose(fp);
			return 0;
		}
	}

	fclose(fp);
	return (-1);
}


/**
 * parse_agent_conf
 *
 * @param
 * @return
 */
static int
parse_agent_conf(char *filename)
{
	FILE *fp = NULL;
	char buf[LINE_MAX];
	char *eclass = (char *)malloc(LINE_MAX);

	memset(buf, 0, LINE_MAX);
	memset(eclass, 0, LINE_MAX);

	if ((fp = fopen(filename, "r")) == NULL) {
		printf("FMD: failed to open conf file %s\n", filename);
		return (-1);
	}

	while (fgets(buf, sizeof(buf), fp)) {
		if (strncmp(buf, "subscribe ", 10) == 0) {
			buf[strlen(buf) - 1] = 0;	/* clear '\n' */
			strcpy(eclass, &buf[10]);
			conf_add_sub(eclass);
			continue;
		}
	}

	if ( eclass )
		free(eclass);
	fclose(fp);
	return 0;
}


/**
 * Reading module-specific configuration file.
 *
 * @param
 * @return
 */
int
module_conf(fmd_module_t *pm, const char *path, int flag)
{
	char *name = (char *) fmd_module_confname(path);
	pconf = (fmd_conf_t *)malloc(sizeof(fmd_conf_t));

	assert(pconf != NULL);
	memset(pconf, 0, sizeof(fmd_conf_t));
	pconf->list_eclass = &pm->list_eclass;

	/* parse conf file */
	if (flag == EVTSRC_CONF) {
		parse_evtsrc_conf(name);
	} else if (flag == AGENT_CONF) {
		parse_agent_conf(name);
	}

	/* setup */
	pm->mod_name = fmd_module_name(path);
	pm->mod_path = strdup(path);
	pm->mod_vers = pconf->cf_ver;
	pm->mod_interval = pconf->cf_interval;

	return 0;
}


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
	int (*_module_conf)(fmd_module_t *, const char *, int);

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
	if(handle == NULL) {
		syslog(LOG_ERR, "dlopen");
		goto out;
	}

	dlerror();
	_module_conf = dlsym(handle, "module_conf");
	if((error = dlerror()) != NULL) {
		syslog(LOG_ERR, "dlsym");
		goto out;
	}

	ret = (*_module_conf)(pm, path, flag);
	if(ret < 0)
		goto out;
	
	return 0;
out:
	dlclose(handle);
	return -1;
}
