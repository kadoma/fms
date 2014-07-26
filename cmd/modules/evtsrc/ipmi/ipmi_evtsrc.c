/*
 * ipmi_evtsrc.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Dec, 2, 2010
 *      Author: Inspur OS Team
 *  
 * Description: 
 *      ipmi error evtsrc module
 */

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <fmd.h>
#include <fmd_evtsrc.h>
#include <fmd_module.h>
#include <protocol.h>
#include <nvpair.h>

#include <linux/ipmi.h>
#include <OpenIPMI/ipmiif.h>
#include <OpenIPMI/ipmi_msgbits.h>
#include <OpenIPMI/ipmi_addr.h>

#include "fm_ipmi.h"

static fmd_t *p_fmd;

/*
 * add a event to event list
 * 
 * Input
 *  head	- event list
 *  nvl		- container of event, which will be add to head
 *  fullclass	- event's full name
 *  ena		- event location
 */
static int
ipmi_fm_event_post(struct list_head *head, nvlist_t *nvl, 
		char *fullclass, uint64_t ena)
{
	sprintf(nvl->name, FM_CLASS);
	strcpy(nvl->value, fullclass);
	nvl->rscid = ena;
	nvlist_add_nvlist(head, nvl);

	printf("IPMI: fmd_xprt_post(hdl, xprt, nvl, 0);\n");

	return 0;
}

/*
 * ipmi_scan_sensor
 *  scan sensor list. If any senor's status 
 *  is abnormal, post a event to fms
 *
 * Input 
 *  fd		- file descriptor of ipmi device
 *  nvl		- list of event
 *
 * Return value
 *  0		- if success
 *  none zero	- if failed
 */
static int 
ipmi_scan_sensor(int fd, struct list_head *head, nvlist_t *nvl)
{
	int	status;				/* status */
	char	*status_str;			/* status string with will pass to fms */
	char	dev_type[16];			/* CPU/Mem/Ioh/Fan */
	struct	sdr_record_list	*e;
	char	fullclass[PATH_MAX];
	int	ena;

	/* scan the list and check status */
	for ( e = get_sdr_list_head(); e != NULL; e = e->next ) {
		status = ipmi_get_dev_status(fd, e);
		if ( status < 0 )		/* some error occur while getting status*/
			continue;

		status_str = ipmi_sdr_get_status(status);
		if ( strcmp(status_str, "OK") == 0 )/* everything is ok */
			continue;
		else {				/* need to report something */
			switch ( e->type ) {
			case IPMI_SDR_FULL_SENSOR_RECORD:
				if (strstr((const char*)e->record.full->id_string, "Proc" ))
					sprintf(dev_type, "cpu.temp");
				else if (strstr((const char*)e->record.full->id_string, "Mem"))
					sprintf(dev_type, "mem.temp");
				else if (strstr((const char*)e->record.full->id_string, "I/O"))
					sprintf(dev_type, "ioh.temp");
				else if (strstr((const char*)e->record.full->id_string, "I/O"))
					sprintf(dev_type, "fan");
				else		/* unrecoginzed sensor */
					continue;
				break;

			case IPMI_SDR_COMPACT_SENSOR_RECORD:
				if (strstr((const char*)e->record.compact->id_string, "Proc" ))
					sprintf(dev_type, "cpu.temp");
				else if (strstr((const char*)e->record.compact->id_string, "Mem"))
					sprintf(dev_type, "mem.temp");
				else if (strstr((const char*)e->record.compact->id_string, "I/O"))
					sprintf(dev_type, "ioh.temp");
				else if (strstr((const char*)e->record.compact->id_string, "I/O"))
					sprintf(dev_type, "fan");
				else		/* unrecoginzed sensor */
					continue;
				break;
			default:		/* unrecoginzed sensor type */
				continue;
			}/* end of switch */

			/* post error */
			memset(fullclass, 0, sizeof(fullclass));
			fprintf(stdout, "IPMI: Status of %s: %s\n", 
				dev_type, 
				status_str);
			snprintf(fullclass, sizeof(fullclass),
					"%s.ipmi.%s.%s",
					FM_EREPORT_CLASS,
					dev_type,
					status_str);
			ena = 0;/*FIXME*/
			nvl = nvlist_alloc();
			if ( nvl == NULL )
				return -1;
			if ( ipmi_fm_event_post(head, nvl, fullclass, ena) != 0){
				free(nvl);
				nvl = NULL;
			}

		}/* end of status check */
	}/* end of list scan */

	return 0;
}

/*
 * Initial the sensor list for the
 * first time. Then scan the list, 
 * find out abnormal device
 */
struct list_head *
ipmi_probe(evtsrc_module_t *emp)
{
	nvlist_t *nvl;
	struct list_head *head;

	int	i;
	int	fd;
	char	ipmi_dev[16];
	static	int first_time = 1;

	p_fmd = emp->module.mod_fmd;

	/* Open IPMI device */
	sprintf(ipmi_dev, "/dev/ipmi0");
	fd = open(ipmi_dev, O_RDWR);
	if ( fd < 0 ) {
		fprintf(stderr, "IPMI: Could not open device %s\n", ipmi_dev);
		return NULL;
	}

	/* Enable event receiver */
	if ( ioctl(fd, IPMICTL_SET_GETS_EVENTS_CMD, &i) < 0 ){
		fprintf(stderr, "IPMI: Could not enable event receiver\n");
		close(fd);
		return NULL;
	}

	if ( first_time ) {
		if ( ipmi_init_sensor_list(fd) != 0 ) {	/* some error occur FIXME? */	
			fprintf(stderr, "IPMI: Could not init sensor list\n");
			close(fd);
			return NULL;
		}
		first_time = 0;
	}

	if ( ipmi_scan_sensor(fd, head, nvl) != 0 ) {
		fprintf(stdout, "IPMI: error occur while scan sensor\n");
		close(fd);
		return;
	}

	if ( fd >=0 )
		close(fd);

	return head;
}

static evtsrc_modops_t ipmi_mops = {
	.evt_probe = ipmi_probe,
};

fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	return (fmd_module_t *)evtsrc_init(&ipmi_mops, path, pfmd);
}

void
fmd_fini(fmd_module_t *mp)
{
}

