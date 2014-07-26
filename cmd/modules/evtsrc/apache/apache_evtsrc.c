/*
 * apache_evtsrc.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 *      Apache service error evtsrc module
 *
 *      This evtsrc module is respon
 */

#include <sys/types.h>
#include <alloca.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#include <fmd.h>
#include <fmd_evtsrc.h>
#include <fmd_module.h>
#include <protocol.h>
#include <nvpair.h>
#include "apache_fm.h"

void
apache_fm_event_post(char *faultname, uint64_t ena, nvlist_t *nvl)
{
	nvlist_t *pnv = NULL;
	int e = 0;
	char *service;
	char fullclass[PATH_MAX];
	int err;

	service = strdup("apache");

	(void) snprintf(fullclass, sizeof (fullclass), "%s.service.network.%s.http.%s",
		FM_EREPORT_CLASS, service, faultname);

	sprintf(nvl->name, FM_CLASS);
	strcpy(nvl->value, fullclass);
	nvl->rscid = ena;

	printf("Apache Module: post %s\n", fullclass);
}

/*
 * Check Apache Services, and generates any ereports as necessary.
 */
static int
apache_service_check(nvlist_t *nvl)
{
	char *fault = NULL;
	uint64_t ena = (uint64_t)0;

	check_http(fault);

	if (fault == NULL)
		printf("Apache Module: No ereport here.\n");
	else
		apache_fm_event_post(fault, ena, nvl);

	return 0;
}

/*
 * Periodic timeout.
 * Calling apache_service_check().
 */
struct list_head *
apache_probe(evtsrc_module_t *emp)
{
//      fmd_debug;
	struct list_head *head = nvlist_head_alloc();
	nvlist_t *nvl = nvlist_alloc();
	nvlist_add_nvlist(head, nvl);

	if(apache_service_check(nvl) != 0) {
		printf("Apache Module: failed to check apache service\n");
		return NULL;
	}

	return head;
}

static evtsrc_modops_t apache_mops = {
	.evt_probe = apache_probe,
};

fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	return (fmd_module_t *)evtsrc_init(&apache_mops, path, pfmd);
}

void
fmd_fini(fmd_module_t *mp)
{
}

