/*
 * sysmon_evtsrc.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Dec, 27, 2010
 *      Author: Inspur OS Team
 *  
 * Description: 
 *      sysmon error evtsrc module
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include <linux/types.h>
#include <linux/fm/fm.h>

#include <fmd.h>
#include <fmd_evtsrc.h>
#include <fmd_module.h>
#include <protocol.h>
#include <nvpair.h>

#include "get_fm_event.h"

#define FM_IS_UNKNOWN_ERROR	0x0
#define FM_IS_MCA_ERROR		0x1
#define FM_IS_SATAEH_ERROR	0x2
#define FM_IS_PCIEAER_ERROR	0x3

static fmd_t *p_fmd;

static int
sysmon_fm_event_post(struct list_head *head, 
		nvlist_t *nvl, char *fullclass, uint64_t ena)
{
	sprintf(nvl->name, FM_CLASS);
	nvl->rscid = ena;
	strcpy(nvl->value, fullclass);
	nvlist_add_nvlist(head, nvl);

	printf("SYSMON: fmd_xprt_post(hdl, xprt, nvl, 0);\n");

	return 0;
}

/* 
 * replace matchStr with replaceStr in srcStr 
 * return 0 when success
 * return non-zero when failed
 */
static int
str_replace(char *srcStr, char *matchStr, char *replaceStr)
{
	int	len;
	int	matchLen;
	char	*pos;
	char	newStr[PATH_MAX];

	if ( !srcStr || !matchStr || !replaceStr )
		return -1;

	matchLen = strlen(matchStr);
	memset(newStr, 0, sizeof(newStr));
	pos = strstr(srcStr, matchStr);
	while (pos) {
		len = pos - srcStr;
		strncpy(newStr, srcStr, len);
		strcat(newStr, replaceStr);
		strcat(newStr, pos + matchLen);
		strcpy(srcStr, newStr);

		pos = strstr(srcStr, matchStr);
	}

	return 0;
}

/*
 * kernel_event_format
 *  classify the kernel fm event as mca event, 
 *  sata-eh event or pcie-aer event
 *
 * Input 
 *  evt		fm event recieved from kernel
 *
 * Return val
 *  kernel event type: MCA, SATA, PCIE_AER, or unknown
 *  return -1 on error
 */
static int
kernel_event_classify(struct fm_kevt* evt)
{
	if (evt == NULL)
		return -1;

	if ( strstr(evt->eclass, "ereport.cpu") ) 		/* MCA error */
		return FM_IS_MCA_ERROR;
	else if ( strstr(evt->eclass, "ereport.io.sata.disk") )	/* sata-eh error */
		return FM_IS_SATAEH_ERROR;
	else if ( strstr(evt->ev_data, "PCIEAER") )		/* pcie-aer error */
		return FM_IS_PCIEAER_ERROR;
	else
		return FM_IS_UNKNOWN_ERROR;
}

/* format_mca_msg
 *  extract fullclass from mca kernel event
 *
 * Input
 *  evt		event from kernel 
 *
 * Output
 *  string "fullclass" which post to fms
 *  NULL if failed
 */
static char*
format_mca_msg(struct fm_kevt *evt)
{

	char	*fullclass;

	fullclass = strdup(evt->eclass);
	return fullclass;
}

/* format_sataeh_msg
 *  extract fullclass from sataeh kernel event
 *
 * Input
 *  evt		event from kernel 
 *
 * Output
 *  string "fullclass" which post to fms
 *  NULL if failed
 */
static char*
format_sataeh_msg(struct fm_kevt *evt)
{
	char	*sp;
	char	*fullclass;

	fullclass = strdup(evt->eclass);
	sp = fullclass;
	while ( *sp != '\0' ) {		
		if ( isupper(*sp) )	/* convert to lowercase */
			*sp = tolower(*sp) ;
		sp++;
	}

	/* slightly modify the describe for fms */
	str_replace(fullclass, " ", "_");
	str_replace(fullclass, "error", "err");
	str_replace(fullclass, "device", "dev");

	return fullclass;
}

/* format_pcieaer_msg
 *  extract fullclass from sataeh kernel event
 *
 * Input
 *  evt		evt->buf contain raw pcie-aer message, typically format is: 
 *  		PCIEAER.[ERR|MSG].[BUS_UA|BUS_PL|BUS_DL|BUS_TL].DOMAIN:BUSNO:DEVNO.FUNNO
 *
 * Output
 *  pcie_id	XXXX:XX:XX.XX identify the pcie location
 *
 * Return value
 *  string "fullclass" which post to fms
 *  NULL if failed
 */
static char*
format_pcieaer_msg(struct fm_kevt *evt, char *pcie_id)
{
	char	*raw_msg;
	char	*err_type;
	char	*fullclass;
	char	*severity;
	int	ret;

	raw_msg	  = strdup(evt->ev_data);
	err_type  = (char*)malloc(64);
	severity  = (char*)malloc(64);
	fullclass = (char*)malloc(PATH_MAX);
	memset(err_type, 0, 64);
	memset(severity, 0, 64);
	memset(fullclass, 0, PATH_MAX);
	
	/* Scan the raw message */
	ret = sscanf(raw_msg, 
		/*"PCIEAER.%s.%s.%*04x:%*02x:%*02x.%*02x", */
		"PCIEAER.%s.%s.%s", 
		severity,
		err_type,
		pcie_id);
	if ( ret < FIELDS_TO_FILL || ret == EOF )
		return NULL;

	/* Check type */
	if ( strcmp(err_type, PCIEAER_INFO_BUSUA) == 0)
		sprintf(fullclass, "%s.io.pcie.bus_unaccessible",
				FM_EREPORT_CLASS);
	else if ( strcmp(err_type, PCIEAER_INFO_BUSDL) == 0)
		sprintf(fullclass, "%s.io.pcie.bus_datalink_err",
				FM_EREPORT_CLASS);
	else if ( strcmp(err_type, PCIEAER_INFO_BUSTL) == 0)
		sprintf(fullclass, "%s.io.pcie.bus_data_trans_link_err",
				FM_EREPORT_CLASS);
	else if ( strcmp(err_type, PCIEAER_INFO_BUSPL) == 0)
		sprintf(fullclass, "%s.io.pcie.bus_phycial_link_err",
				FM_EREPORT_CLASS);
	else
		sprintf(fullclass, "%s.io.pcie.unknown",
				FM_EREPORT_CLASS);

	if ( strcmp(severity, "MSG") == 0 )	/* An error which has been corrected */
		strcat(fullclass, "_rc");
	else					/* no recovery error */
		strcat(fullclass, "_uc");

	if (raw_msg)
		free(raw_msg);
	if (err_type)
		free(err_type);

	return fullclass;
}

/* sysmon_check
 *  Check the kernel message
 *
 * Input 
 *  nvl		list of event
 *
 * Return val
 *  0 		if success
 *  non 0	if failed
 */
int sysmon_check(struct list_head *fault)
{
	int		type;
	char		*fullclass;
	struct fm_kevt	*kevt;
	nvlist_t	*nvl = NULL;
	uint64_t	ena;

	for ( kevt=kernel_event_recv(); 
			kevt != NULL; 		/* read until no message */
			kevt=kernel_event_recv() ) {
		type = kernel_event_classify(kevt);
		char pcie_id[16];
		switch (type) {
			case FM_IS_MCA_ERROR: 
				fullclass = format_mca_msg(kevt);
				ena = topo_get_cpuid(p_fmd, kevt->cpuid);
				break;
			case FM_IS_SATAEH_ERROR:
				fullclass = format_sataeh_msg(kevt);
				ena = 0/*FIXME*/;
				break;
			case FM_IS_PCIEAER_ERROR:
				memset(pcie_id, 0, sizeof(pcie_id));
				fullclass = format_pcieaer_msg(kevt, pcie_id);
				ena = topo_get_pcid(p_fmd, pcie_id);
				break;
			default:
				return -1;
				sprintf(fullclass, "%s.io.pcie.unknown",
						FM_EREPORT_CLASS);
				ena = 0;
				
		}

		if ( fullclass == NULL )
			continue;

		nvl = nvlist_alloc();
		if (nvl)
			sysmon_fm_event_post(fault, nvl, fullclass, ena);	
		else
			return -1;
	};

	return 0;
}

/*
 * sysmon_probe
 *  probe the kernel fmd event and send to fms
 *
 * Input
 *  emp		event source module pointer
 *
 * Return val
 *  return the nvlist which contain the event.
 *  NULL if failed
 */
struct list_head *
sysmon_probe(evtsrc_module_t *emp)
{
	int			ret;
	char 			fullclass[PATH_MAX];
	char 			*raw_msg = NULL;
	uint64_t		ena;
	struct list_head	*fault;

	p_fmd = emp->module.mod_fmd;

	fault = nvlist_head_alloc();
	if ( fault == NULL ) {
		fprintf(stderr, "SYSMON: alloc list_head error\n");
		return NULL;
	}
	
	ret = sysmon_check(fault);
	if ( ret )
		return NULL;
	else
		return fault;
}

static evtsrc_modops_t sysmon_mops = {
	.evt_probe = sysmon_probe,
};

fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	return (fmd_module_t *)evtsrc_init(&sysmon_mops, path, pfmd);
}

void
fmd_fini(fmd_module_t *mp)
{
}

