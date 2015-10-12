/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    faulty.c
 * Author:      Inspur OS Team 
                wang.leibj@inspur.com
 * Date:        2015-08-21
 * Description: isplay list of faulty resources
 *
 ************************************************************/

#include <string.h>
#include <errno.h>
#include <limits.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <fmd_adm.h>
#include <fmd_msg.h>
#include <stdlib.h>
#include "fmadm.h"

/*
 *
 */

#include <list.h>

typedef struct status_record {
	char *severity;
	char *msgid;
	char *class;
	char *resource;
	char *asru;
	char *fru;
	char *serial;
	uint64_t uuid;
	uint64_t rscid;
	uint8_t  state;
	uint32_t count;
	uint64_t create;
	struct list_head list;
} status_record_t;

static struct list_head status_rec_list;

static int max_display;
static int max_fault = 0;
static int opt_f = 0;
//static int opt_g = 0;

static fmd_msg_hdl_t *fmadm_msghdl = NULL; /* handle for libfmd_msg calls */

static void
usage(void)
{
	fputs("Usage: fmadm faulty [-sfvh] [-n <num>] [-t<type>]\n"
	      "\t-h    help\n"
	      "\t-s    show faulty fru's\n"
	      "\t-f    show faulty only \n"
	      "\t-n    number of fault records to display\n"
	      "\t-v    full output\n"
	      "\t-t    caselist type :repaired or happening\n"
	      , stderr);
}

void
format_date(char *buf, time_t secs)
{
	struct tm *tm = NULL;
	int year = 0, month = 0, day = 0;
	int hour = 0, min = 0, sec = 0;

	time(&secs);
	tm = gmtime(&secs);
	year = tm->tm_year + 1900;
	month = tm->tm_mon + 1;
	day = tm->tm_mday;
	hour = tm->tm_hour;
	min = tm->tm_min;
	sec = tm->tm_sec;
	sprintf(buf, "%d-%02d-%02d %02d:%02d:%02d\t", year, month, day, hour, min, sec);
}


static void
print_dict_info_line(char *msgid, fmd_msg_item_t what, const char *linehdr)
{
	char *cp = fmd_msg_getitem_id(fmadm_msghdl, msgid, what);

	if (cp != NULL)
		(void) printf("%s%s\n", linehdr, cp);
	(void) printf("\n");

//	free(cp);	/* Segmentation fault ? */
}

static void
print_dict_info(status_record_t *srp)
{
	(void) printf("\n");
	print_dict_info_line(srp->msgid, FMD_MSG_ITEM_CLASS, "Event Class : ");
	if (srp->asru) {
		(void) printf("Affects : %s\n", srp->asru);
	}
	if (srp->fru) {
		(void) printf("FRU : %s\n", srp->fru);
	}
	(void) printf("\n");
	print_dict_info_line(srp->msgid, FMD_MSG_ITEM_DESC, "Description : ");
	print_dict_info_line(srp->msgid, FMD_MSG_ITEM_RESPONSE, "Response : ");
	print_dict_info_line(srp->msgid, FMD_MSG_ITEM_IMPACT, "Impact : ");
	print_dict_info_line(srp->msgid, FMD_MSG_ITEM_ACTION, "Action : ");
	(void) printf("\n");
}

static void
print_status_record(status_record_t *srp, int opt_t)
{
	char buf[64];
	time_t sec = 0;
	char *head;
	head = "--------------------- "
	    "--------------  "
	    "---------------\n"
	    "TIME"
	    "                     MSG-ID"
	    "         SEVERITY\n--------------------- "
	    "-------------- "
	    " ---------------";
	printf("%s\n", head);

	/* Get the times */
	format_date(buf, sec);

	if (opt_t) {
		if (srp->state > FAF_CASE_CREATE)
			return;
		else {
			(void) printf("%s%s\t%s\n",
			    buf, srp->msgid, srp->severity);

			print_dict_info(srp);
		}
	} else {
		(void) printf("%s%s\t%s\n",
		    buf, srp->msgid, srp->severity);

		print_dict_info(srp);
	}
}

static status_record_t *
status_record_alloc(void)
{
	status_record_t *srp =
		(status_record_t *)malloc(sizeof (status_record_t));

	memset(srp, 0, sizeof (status_record_t));
	srp->severity = NULL;
	srp->msgid = NULL;
	srp->class = NULL;
	srp->resource = NULL;
	srp->asru = NULL;
	srp->fru = NULL;
	srp->serial = NULL;
	srp->uuid = (uint64_t)0;
	srp->rscid = (uint64_t)0;
	srp->state = (uint8_t)0;
	srp->count = (uint32_t)0;
	srp->create = (uint64_t)0;
	INIT_LIST_HEAD(&srp->list);

	return srp;
}

static void
print_catalog(fmd_adm_t *adm)
{
	struct list_head *pos = NULL;
	status_record_t *srp = NULL;
	int max = max_display;

	if (max == 0) {
		list_for_each(pos, &status_rec_list) {
			srp = list_entry(pos, status_record_t, list);

			(void) printf("\n");
			print_status_record(srp, opt_f);
		}
	} else if (max > 0) {
		list_for_each(pos, &status_rec_list) {
			srp = list_entry(pos, status_record_t, list);
			if (max > 0) {
				(void) printf("\n");
				print_status_record(srp, opt_f);
				max--;
			} else
				break;
		}
	}
}


static void
print_fru(fmd_adm_t *adm)
{
	struct list_head *pos = NULL;
	status_record_t *srp;
	char *head;
	head = "------------------  "
	       "-------------------  "
	       "-----------\n"
	       "DEV NAME                 FAULT CLASS"
	       "        COUNTS"
	       "\n------------------  "
	       "-------------------  "
	       "-----------";
	printf("%s\n", head);

	list_for_each(pos, &status_rec_list) {
		srp = list_entry(pos, status_record_t, list);

		(void) printf("\n");
		(void) printf("%-20s%-20s \t%d\n",srp->fru,srp->class, srp->count);
	}
}

static int
get_cases_from_fmd(fmd_adm_t *adm,char* type)
{
	struct list_head *pos = NULL;
	fmd_adm_caseinfo_t *aci = NULL;
	int rt = FMADM_EXIT_SUCCESS;

	/*
	 * These calls may fail with Protocol error if message payload is to big
	 */
	if (fmd_adm_case_iter(adm,type) != 0)
		die("FMD: failed to get case list from fmd");

	list_for_each(pos, &adm->cs_list) {
		aci = list_entry(pos, fmd_adm_caseinfo_t, aci_list);

		status_record_t *srp = status_record_alloc();
		char *tmp = strdup(aci->fafc.fafc_fault);	
		char  *p = strtok(tmp,":");
		int index = 0;
		if(p != NULL){
			if(index == 0 ){
				index++;
				srp->fru = p;	
			}else
				break;
		}
		
		char *temp = strstr(aci->fafc.fafc_fault,":");
		srp->class = temp+1;
		
		//srp->fru = aci->aci_fru;
		srp->asru = aci->aci_asru;
		//srp->class = aci->fafc.fafc_fault;
		srp->uuid = aci->fafc.fafc_uuid;
		srp->rscid = aci->fafc.fafc_rscid;
		srp->state = aci->fafc.fafc_state;
		srp->count = aci->fafc.fafc_count;
		srp->create = aci->fafc.fafc_create;
		srp->msgid = fmd_msg_getid_evt(fmadm_msghdl, srp->class);
		srp->severity = fmd_msg_getitem_id(fmadm_msghdl,
			srp->msgid, FMD_MSG_ITEM_SEVERITY);
		list_add(&srp->list, &status_rec_list);
	}

	return (rt);
}

/*
 * fmadm faulty command
 *
 *	-h		help
 *	-a		show hidden fault records
 *	-f		show faulty fru's
 *	-n		number of fault records to display
 *	-u		print listed uuid's only
 *	-v		full output
 *	-t		only fault records
 */

int
cmd_faulty(fmd_adm_t *adm, int argc, char *argv[])
{
	int opt_s = 0, opt_v = 0, opt_t = 0;
	int rt, c;
	char opt_type[32];
	while ((c = getopt(argc, argv, "svftn:u:h?")) != EOF) {
		switch (c) {
		case 's':
			opt_s++;
			break;
		case 'v':
			opt_v++;
			break;
		case 'f':
			opt_v++;
			opt_f++;
			break;
		case 'n':
			max_fault = atoi(optarg);
			break;
		case 't':
			opt_t++;
			break;
		case 'h':
		case '?':
			usage();
			return (FMADM_EXIT_SUCCESS);
		default:
			return (FMADM_EXIT_USAGE);
		}
	}
	if (argc == 1) {
		usage();
		return (FMADM_EXIT_SUCCESS);
	}

	if ((fmadm_msghdl = fmd_msg_init(FMD_MSG_VERSION)) == NULL)
		return (FMADM_EXIT_ERROR);

	INIT_LIST_HEAD(&status_rec_list);
	
	if(opt_t){
		sprintf(opt_type,"happening");
	}else{
		sprintf(opt_type,"repaired");
	}

	rt = get_cases_from_fmd(adm,opt_type);

	if (opt_s)
		print_fru(adm);
	max_display = max_fault;
	if (opt_v)
		print_catalog(adm);
	fmd_msg_fini(fmadm_msghdl);

	return (rt);
}
