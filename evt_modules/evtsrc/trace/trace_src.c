/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    main.c
 * Author:      Inspur OS Team 
                wanghuanbj@inspur.com
 * Date:        20xx-xx-xx
 * Description: 
 *
 ************************************************************/

#include <stdint.h>

#include "wrap.h"
#include "fmd.h"
#include "fmd_nvpair.h"
#include "evt_src.h"
#include "fmd_module.h"
#include "logging.h"

struct list_head *
trace_probe(evtsrc_module_t *emp)
{
    wr_log("trace", WR_LOG_DEBUG, "trace probe test hardware.");
	
    nvlist_t 	*nvl = NULL;
    struct list_head *head = nvlist_head_alloc();
	/*
	//event list
	nvl = nvlist_alloc();
	sprintf(nvl->name, "cpu");
    strcpy(nvl->value, "ereport.trace.unknown_ue");
    nvl->dev_id = 0x02;
    nvl->data=strdup("a private pointer.");
    nvlist_add_nvlist(head, nvl);
	nvl->node_num = 1;	

	//fault
	nvl = nvlist_alloc();
    sprintf(nvl->name, "cpu");
    strcpy(nvl->value, "ereport.trace.mc_ce");
    nvl->dev_id = 0x01;
    nvl->data=strdup("a private pointer.");
    nvlist_add_nvlist(head, nvl);
	nvl->node_num = 2;
*/
	//serd
	nvl = nvlist_alloc();
	sprintf(nvl->name, "cpu");
    strcpy(nvl->value, "ereport.trace.qpi_ue");
    nvl->dev_id = 0x01;
    nvl->data=strdup("a private pointer.");
    nvlist_add_nvlist(head, nvl);
	nvl->node_num = 3;

    return head;
}

static evtsrc_modops_t trace_mops = {
    .evt_probe = trace_probe,
};


fmd_module_t *
fmd_module_init(char *full_path, fmd_t *p_fmd)
{
    wr_log("trace", WR_LOG_DEBUG, "trace probe test hardware.");
    return (fmd_module_t *)evtsrc_init(&trace_mops, full_path, p_fmd);
}

void
fmd_module_fini(fmd_module_t *mp)
{
	return;
}

