/*
 * network_evtsrc.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Nov, 29, 2010
 *      Author: Inspur OS Team
 *  
 * Description: 
 *      network error evtsrc module
 */
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include <fmd.h>
#include <fmd_evtsrc.h>
#include <fmd_module.h>
#include <protocol.h>
#include <nvpair.h>

#include "fm_netdect.h"		/* for interface error */
#include "fm_arpdect.h"		/* for ip conflict error */
#include "fm_rtdect.h"		/* for route table error */
#include "fm_dnsdect.h"		/* for dns server error */

static fmd_t *p_fmd;

/*
 * Input
 *  head	- list of all event
 *  nvl		- current event
 *  fullclass
 *  ena
 *  hwaddr	- MAC address of some host in network which
 		  IP conflict with us
 */
int
network_fm_event_post(struct list_head *head, nvlist_t *nvl,
		char *fullclass, uint64_t ena, char *hwaddr)
{
	sprintf(nvl->name, FM_CLASS);
	strcpy(nvl->value, fullclass);
	nvl->rscid = ena;
	if ( hwaddr != NULL )
		nvl->data=strdup(hwaddr);
	nvlist_add_nvlist(head, nvl);

	printf("Network module: post %s\n", fullclass);

	return 0;
}

struct list_head *
network_probe(evtsrc_module_t *emp)
{
	uint64_t	ena;
	char		fullclass[PATH_MAX];
	char		*fault;
	nvlist_t 	*nvl;
	struct list_head *head = nvlist_head_alloc();

	int 		i;
	FILE 		*fp_net;			/* /proc/net/dev */
	static char	linebuf[LINE_MAX];
	static struct 	net_dev *devp[LINE_MAX];	/* hold 1023 max devname */

	p_fmd = emp->module.mod_fmd;

	/* Read info from /proc/net/dev */
	if ((fp_net = fopen("/proc/net/dev", "r")) == NULL) {
		perror("/proc/net/dev");
		syslog(LOG_USER|LOG_INFO, "open /proc/net/dev error\n");
		return NULL;
	}
	fgets(linebuf, LINE_MAX, fp_net);	/* remove the first line */
	fgets(linebuf, LINE_MAX, fp_net);	/* remove the second line */
	while (fgets(linebuf, LINE_MAX, fp_net) != NULL)
		get_ethinfo(linebuf, devp);	/* collect the error info */
	
	/* ////////////////////
	 * interface error
	 * /////////////////// */
	for (i=0; devp[i] != NULL; ++i) {
		if (devp[i]->need_report) {
			do {			/* error type */
				if ( devp[i]->link_state == NETLINK_DOWN ) {
					fault = strdup("linkdown");
					continue;
				}
				if ( devp[i]->rx_err > devp[i]->prev_rx_err ) {
					fault = strdup("rxerr");
					continue;
				}
				if ( devp[i]->tx_err > devp[i]->prev_tx_err ) {
					fault = strdup("txerr");
					continue;
				}
				fault = strdup("unknown");
			} while(0);

			/* report error */
			ena = topo_get_pcid(p_fmd, devp[i]->name);
			snprintf(fullclass, sizeof(fullclass),
					"%s.io.network.%s",
					FM_EREPORT_CLASS, fault);
			nvl = nvlist_alloc();
			if (nvl == NULL) {
				fprintf(stderr, "NETWORK: out of memory\n");
				fclose(fp_net);
				return NULL;
			}
			if (network_fm_event_post(head, nvl, fullclass, ena, NULL) != 0 ) {
				nvlist_free(nvl);
				nvl = NULL;
			}
		}
	} /* end of scan all devp */
	fclose(fp_net);

	/* ///////////////////////
	 * IP conflict error
	 * /////////////////////// */
	int 		err;		/* return val of check_ip() */
	int		msglen;
	unsigned char	*mac;
	in_addr_t	dst_ip;
	struct ip_conflict_msg	msg;
	
	/* 
	   For each interface, check if there are any 
	   host in network that has the same IP addr
	   but has different MAC addr.
	   */
	for (i=0; devp[i] != NULL; ++i) {
		err =  check_ip(devp[i]->name); 
		if ( err < 0 ) {	/* some error happened */
			fprintf(stderr, "NETWROK: Couldn't not check IP\n");
			continue;	
		} else if ( err != 0 ) {/* ip address conflict */	
			memset(fullclass, 0, sizeof(fullclass));
			sprintf(fullclass, "%s.io.network.ipconflict", FM_EREPORT_CLASS);

			/* report error */
			ena = topo_get_pcid(p_fmd, devp[i]->name);
			nvl = nvlist_alloc();
			if (nvl == NULL) {
				fprintf(stderr, "NETWORK: out of memory\n");
				return NULL;
			}
			if ( network_fm_event_post(head, nvl, fullclass, 
						ena, msg.c_haddr/*TODO!!*/) != 0 ) {
				nvlist_free(nvl);
				nvl = NULL;
			}
		}
		
	}

	/* ////////////////////
	 * DNS error
	 * /////////////////// */
	if ( dns_check() == 1 ) {
		memset(fullclass, 0, sizeof(fullclass));
		sprintf(fullclass, "%s.io.network.dns-error", FM_EREPORT_CLASS);
		ena = 0;
		nvl = nvlist_alloc();
		if (nvl == NULL) {
			fprintf(stderr, "NETWORK: out of memory\n");
			return NULL;
		}
		if ( network_fm_event_post(head, nvl, fullclass, ena, NULL) != 0 ) {
			nvlist_free(nvl);
			nvl = NULL;
		}
	}


	/* ////////////////////
	 * route error
	 * /////////////////// */
	if ( route_check() == 1 ) {
		memset(fullclass, 0, sizeof(fullclass));
		sprintf(fullclass, "%s.io.network.rt-error", FM_EREPORT_CLASS);
		ena = 0;
		nvl = nvlist_alloc();
		if (nvl == NULL) {
			fprintf(stderr, "NETWORK: out of memory\n");
			return NULL;
		}
		if ( network_fm_event_post(head, nvl, fullclass, ena, NULL) != 0 ) {
			nvlist_free(nvl);
			nvl = NULL;
		}
	}

	return head;
}

static evtsrc_modops_t network_mops = {
	.evt_probe = network_probe,
};

fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	return (fmd_module_t *)evtsrc_init(&network_mops, path, pfmd);
}

void
fmd_fini(fmd_module_t *mp)
{
}
