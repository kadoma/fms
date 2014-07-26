/*
 * faulty.c
 *
 *  Created on: Oct 13, 2010
 *      Author: Inspur OS Team
 *
 *  Description:
 *      faulty.c
 */

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
 * Fmadm faulty takes a number of options which affect the format of the
 * output displayed. By default, the display reports the FRU and ASRU along
 * with other information on per-case basis as in the example below.
 *
 * --------------- ------------------------------------  -------------- -------
 * TIME            EVENT-ID                              MSG-ID         SEVERITY
 * --------------- ------------------------------------  -------------- -------
 * Sep 21 10:01:36 d482f935-5c8f-e9ab-9f25-d0aaafec1e6c  AMD-8000-2F    Major
 *
 * Fault class	: fault.memory.dimm_sb
 * Affects	: mem:///motherboard=0/chip=0/memory-controller=0/dimm=0/rank=0
 *		    faulted but still in service
 * FRU		: "CPU 0 DIMM 0" (hc://.../memory-controller=0/dimm=0)
 *		    faulty
 *
 * Description	: The number of errors associated with this memory module has
 *		exceeded acceptable levels.  Refer to
 *		http://sun.com/msg/AMD-8000-2F for more information.
 *
 * Response	: Pages of memory associated with this memory module are being
 *		removed from service as errors are reported.
 *
 * Impact	: Total system memory capacity will be reduced as pages are
 *		retired.
 *
 * Action	: Schedule a repair procedure to replace the affected memory
 *		module.  Use fmdump -v -u <EVENT_ID> to identify the module.
 *
 * The -v flag is similar, but adds some additonal information such as the
 * resource. The -s flag is also similar but just gives the top line summary.
 * All these options (ie without the -f or -r flags) use the print_catalog()
 * function to do the display.
 *
 * The -f flag changes the output so that it appears sorted on a per-fru basis.
 * The output is somewhat cut down compared to the default output. If -f is
 * used, then print_fru() is used to print the output.
 *
 * -----------------------------------------------------------------------------
 * "SLOT 2" (hc://.../hostbridge=3/pciexrc=3/pciexbus=4/pciexdev=0) faulty
 * 5ca4aeb3-36...f6be-c2e8166dc484 2 suspects in this FRU total certainty 100%
 *
 * Description	: A problem was detected for a PCI device.
 *		Refer to http://sun.com/msg/PCI-8000-7J for more information.
 *
 * Response	: One or more device instances may be disabled
 *
 * Impact	: Possible loss of services provided by the device instances
 *		associated with this fault
 *
 * Action	: Schedule a repair procedure to replace the affected device.
 * 		Use fmdump -v -u <EVENT_ID> to identify the device or contact
 *		Sun for support.
 *
 * The -r flag changes the output so that it appears sorted on a per-asru basis.
 * The output is very much cut down compared to the default output, just giving
 * the asru fmri and state. Here print_asru() is used to print the output.
 *
 * mem:///motherboard=0/chip=0/memory-controller=0/dimm=0/rank=0	degraded
 *
 * For all fmadm faulty options, the sequence of events is
 *
 * 1) Walk through all the cases in the system using fmd_adm_case_iter() and
 * for each case call dfault_rec(). This will call add_fault_record_to_catalog()
 * This will extract the data from the nvlist and call catalog_new_record() to
 * save the data away in various linked lists in the catalogue.
 *
 * 2) Once this is done, the data can be supplemented by using
 * fmd_adm_rsrc_iter(). However this is now only necessary for the -i option.
 *
 * 3) Finally print_catalog(), print_fru() or print_asru() are called as
 * appropriate to display the information from the catalogue sorted in the
 * requested way.
 *
 */

#include <fmd_list.h>

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
static int opt_t = 0;
//static int opt_g = 0;

static fmd_msg_hdl_t *fmadm_msghdl = NULL; /* handle for libfmd_msg calls */

static void
usage(void)
{
	fputs("Usage: fmadm faulty [-afvth] [-n <num>] [-u <uuid>]\n"
	      "\t-h    help\n"
	      "\t-a    show hidden fault records\n"
	      "\t-f    show faulty fru's\n"
	      "\t-n    number of fault records to display\n"
	      "\t-u    print listed uuid's only\n"
	      "\t-v    full output\n"
	      "\t-t    print only fault records\n"
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
print_faulty(fmd_adm_t *adm)
{
	struct list_head *pos = NULL;
	fmd_adm_caseinfo_t *aci;

	list_for_each(pos, &adm->cs_list) {
		aci = list_entry(pos, fmd_adm_caseinfo_t, aci_list);

		printf("%s\t%s\n", aci->aci_fru, aci->aci_asru);
	}
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
	time_t sec;
	char *head;
	head = "--------------------- "
	    "-------------------------------  "
	    "-------------- ---------\n"
	    "TIME                  UUID"
	    "                             MSG-ID"
	    "         SEVERITY\n--------------------- "
	    "------------------------------- "
	    " -------------- ---------";
	printf("%s\n", head);

	/* Get the times */
	format_date(buf, sec);

	if (opt_t) {
		if (srp->state > FAF_CASE_CREATE)
			return;
		else {
			(void) printf("%s0x%llx\t\t\t%s   %s\n",
			    buf, srp->uuid, srp->msgid, srp->severity);

			print_dict_info(srp);
		}
	} else {
		(void) printf("%s0x%llx\t\t\t%s   %s\n",
		    buf, srp->uuid, srp->msgid, srp->severity);

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
			print_status_record(srp, opt_t);
		}
	} else if (max > 0) {
		list_for_each(pos, &status_rec_list) {
			srp = list_entry(pos, status_record_t, list);
			if (max > 0) {
				(void) printf("\n");
				print_status_record(srp, opt_t);
				max--;
			} else
				break;
		}
	}
}

static void
print_uuid_catalog(fmd_adm_t *adm, int opt_uuid)
{
	struct list_head *pos = NULL;
	status_record_t *srp;

	list_for_each(pos, &status_rec_list) {
		srp = list_entry(pos, status_record_t, list);
		if (opt_uuid == srp->uuid) {
			(void) printf("\n");
			print_status_record(srp, opt_t);
		} else
			continue;
	}
}

static void
print_fru(fmd_adm_t *adm)
{
	struct list_head *pos = NULL;
	status_record_t *srp;
	char *head;
	head = "---------- "
	       "--------------------------------------  "
	       "---------- ---------- ------\n"
	       "UUID       FAULT CLASS"
	       "               		  EVENT COUNTS"
	       "   RSCID   FRU\n---------- "
	       "--------------------------------------  "
	       "---------- ---------- ------";
	printf("%s\n", head);

	list_for_each(pos, &status_rec_list) {
		srp = list_entry(pos, status_record_t, list);

		(void) printf("\n");
		(void) printf("0x%llx  %-42s %d \t0x%llx %s\n",
			srp->uuid, srp->class, srp->count, srp->rscid, srp->fru);
	}
}

static int
get_cases_from_fmd(fmd_adm_t *adm)
{
	struct list_head *pos = NULL;
	fmd_adm_caseinfo_t *aci = NULL;
	int rt = FMADM_EXIT_SUCCESS;

	/*
	 * These calls may fail with Protocol error if message payload is to big
	 */
	if (fmd_adm_case_iter(adm) != 0)
		die("FMD: failed to get case list from fmd");

	list_for_each(pos, &adm->cs_list) {
		aci = list_entry(pos, fmd_adm_caseinfo_t, aci_list);

		status_record_t *srp = status_record_alloc();

		srp->fru = aci->aci_fru;
		srp->asru = aci->aci_asru;
		srp->class = aci->fafc.fafc_fault;
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
	int opt_a = 0, opt_v = 0, opt_f = 0;
	int opt_n = 0, opt_u = 0, opt_uuid = 0;
	int rt, c;

	while ((c = getopt(argc, argv, "afvtn:u:h?")) != EOF) {
		switch (c) {
		case 'a':
			opt_a++;
			break;
		case 'f':
			opt_f++;
			break;
		case 'v':
			opt_v++;
			break;
		case 't':
			opt_v++;
			opt_t++;
			break;
		case 'n':
			max_fault = atoi(optarg);
			break;
		case 'u':
			opt_uuid = atoi(optarg);
			opt_u++;
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
	rt = get_cases_from_fmd(adm);

	if (opt_a)
		print_faulty(adm);
	max_display = max_fault;
	if (opt_f)
		print_fru(adm);
	if (opt_v)
		print_catalog(adm);
	if (opt_u)
		print_uuid_catalog(adm, opt_uuid);
	fmd_msg_fini(fmadm_msghdl);

	return (rt);
}
