/*
 * fmdump.c
 *
 *  Created on: Nov 1, 2010
 *      Author: Inspur OS Team
 *
 *  Description:
 *      fmdump.c
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <dirent.h>

#define __USE_XOPEN
#include <time.h>

#include "fmdump.h"

#define	FMDUMP_EXIT_SUCCESS	0
#define	FMDUMP_EXIT_FATAL	1
#define	FMDUMP_EXIT_USAGE	2
#define	FMDUMP_EXIT_ERROR	3

static char arg_class[256];
static char arg_uuid[256];
static char arg_t[256];
static char arg_T[256];

static struct tm *arg_time = NULL;
static struct tm *arg_Time = NULL;

fmd_msg_hdl_t *g_msg = NULL;

static int
usage(FILE *fp) 
{
        (void) fprintf(fp, "Usage: fmdump [-eaV] [-c <class>] [-t <time>] [-T <Time>] "
            "[-u uuid] [file]\n");

        (void) fprintf(fp,
            "\t-c  select events that match the specified class\n"
            "\t-e  display error log content instead of fault log content\n"
            "\t-t  select events that occurred after the specified time\n"
            "\t-T  select events that occurred before the specified time\n"
            "\t-u  select events that match the specified uuid\n"
            "\t-V  set very verbose mode: display complete event contents\n\n"
            "\t<time> can be:\n"
            "\t\t'mm/dd/yyyy hh:mm:ss'\n"
            "\t\t'mm/dd/yyyy hh:mm'\n"
            "\t\t mm/dd/yyyy\n"
            "\t\t'yyyy/mm/dd hh:mm:ss'\n"
            "\t\t'yyyy/mm/dd hh:mm'\n"
            "\t\t yyyy/mm/dd\n"
            "\t\t'yyyy-mm-dd hh:mm:ss'\n"
            "\t\t'yyyy-mm-dd hh:mm'\n"
            "\t\t yyyy-mm-dd\n"
            "\t\t hh:mm:ss\n"
            "\t\t hh:mm\n\n"
            "\t<Time> can be:\n"
            "\t\t'N s|sec'\n"
            "\t\t'N m|min'\n"
            "\t\t'N h|hour'\n"
            "\t\t'N d|day'\n\n");

        return (FMDUMP_EXIT_USAGE);
}

void
fmdump_vprintf(fmd_log_t *lp, fmd_dump_type_t type, int flags)
{
	fmd_log_record_t *record = NULL;
	fmd_log_t *log = NULL;
	char buffer[PATH_MAX];
	char *stop;
	memset(buffer, 0, 256 * sizeof (char));

	if (flags == 0) {
		record = lp->fst_rec;

		while (record) {
			switch (type) {
			case FMDUMP_AXXCXXX:	/* -a -c class */
				if (strcmp(arg_class, record->eclass) == 0) {
					sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n",
						record->time, record->uuid,
						fmd_msg_getid_evt(g_msg, record->eclass));
					printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
					printf("%s", buffer);
				} else
					goto retry;
				goto out;
			case FMDUMP_AXXXXXU:	/* -a -u uuid */
				if ((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid) {
					sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n",
						record->time, record->uuid,
						fmd_msg_getid_evt(g_msg, record->eclass));
					printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
					printf("%s", buffer);
				} else
					goto retry;
				goto out;
			case FMDUMP_AXXXXXX:	/* -a */
				sprintf(buffer, "%s\t0x%llx\t%s\n",
					record->time, record->uuid,
					fmd_msg_getid_evt(g_msg, record->eclass));
				printf("%s", buffer);
				memset(buffer, 0, 256 * sizeof (char));
				goto retry;
			case FMDUMP_XEXCXXX:	/* -e -c class */
				if (strcmp(arg_class, record->eclass) == 0) {
					sprintf(buffer, "%s\t\t\t%s\n",
						record->time, record->eclass);
					printf("TIME\t\t\t\t\tCLASS\n");
					printf("%s", buffer);
				} else
					goto retry;
				goto out;
			case FMDUMP_XEXXXXU:	/* -e -u uuid */
				if ((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid) {
					sprintf(buffer, "%s\t\t\t%s\n",
						record->time, record->eclass);
					printf("TIME\t\t\t\t\tCLASS\n");
					printf("%s", buffer);
				} else
					goto retry;
				goto out;
			case FMDUMP_XEXXXXX:	/* -e */
				sprintf(buffer, "%s\t%s\n",
					record->time, record->eclass);
				printf("%s", buffer);
				memset(buffer, 0, 256 * sizeof (char));
				goto retry;
			case FMDUMP_XXVCXXX:	/* -V -c class */
				if (strcmp(arg_class, record->eclass) == 0) {
					sprintf(buffer, "----------------------------"
							"----------------------------\n"
							"Event Time : %s\n"
							"Severity : %s\n"
							"EventIdx : %s\n\n"
							"Event class : %s\n"
							"Affects : %s\n"
							"FRU : %s\n\n"
							"Description : %s\n\n"
							"Response : %s\n\n"
							"Impact : %s\n\n"
							"Action : %s\n\n",
						record->time,
						fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_SEVERITY),
						fmd_msg_getid_evt(g_msg, record->eclass),
						record->eclass,	""/*NULL*/,""/*NULL*/,
						fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_DESC),
						fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_RESPONSE),
						fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_IMPACT),
						fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_ACTION));
					printf("\n");
					printf("%s", buffer);
				} else
					goto retry;
				goto out;
			case FMDUMP_XXVXXXU:	/* -V -u uuid */
				if ((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid) {
					sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n"
							"?%\t<serdtype>\n"
							"Affects : %s\n"
							"FRU : %s\n",
						record->time, record->uuid,
						fmd_msg_getid_evt(g_msg, record->eclass),
						""/*NULL*/,""/*NULL*/);
					printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
					printf("%s", buffer);
				} else
					goto retry;
				goto out;
			case FMDUMP_XXVXXXX:	/* -V */
				sprintf(buffer, "----------------------------"
						"----------------------------\n"
						"Event Time : %s\n"
						"Severity : %s\n"
						"EventIdx : %s\n\n"
						"Event class : %s\n"
						"Affects : %s\n"
						"FRU : %s\n\n"
						"Description : %s\n\n"
						"Response : %s\n\n"
						"Impact : %s\n\n"
						"Action : %s\n\n",
					record->time,
					fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_SEVERITY),
					fmd_msg_getid_evt(g_msg, record->eclass),
					record->eclass, ""/*NULL*/,""/*NULL*/,
					fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_DESC),
					fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_RESPONSE),
					fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_IMPACT),
					fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_ACTION));
				printf("\n");
				printf("%s", buffer);
				memset(buffer, 0, 256 * sizeof (char));
				goto retry;
			default:
				break;
			}
retry:
			record = record->next;
		}
	} else if (flags == 1) {
		log = lp->next;

		while (log) {
			record = log->fst_rec;

			while (record) {
				switch (type) {
				case FMDUMP_AXXCXXX:    /* -a -c class */
					if (strcmp(arg_class, record->eclass) == 0) {
						sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n",
							record->time, record->uuid,
						fmd_msg_getid_evt(g_msg, record->eclass));
						printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
						printf("%s", buffer);
					} else
						goto _retry;
					goto out;
				case FMDUMP_AXXXXXU:    /* -a -u uuid */
					if ((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid) {
						sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n",
						record->time, record->uuid,
						fmd_msg_getid_evt(g_msg, record->eclass));
						printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
						printf("%s", buffer);
					} else
						goto _retry;
					goto out;
				case FMDUMP_AXXXXXX:    /* -a */
					sprintf(buffer, "%s\t0x%llx\t%s\n",
						record->time, record->uuid,
						fmd_msg_getid_evt(g_msg, record->eclass));
					printf("%s", buffer);
					memset(buffer, 0, 256 * sizeof (char));
					goto _retry;
				case FMDUMP_XEXCXXX:    /* -e -c class */
					if (strcmp(arg_class, record->eclass) == 0) {
						sprintf(buffer, "%s\t\t\t%s\n",
							record->time, record->eclass);
						printf("TIME\t\t\t\t\tCLASS\n");
						printf("%s", buffer);
					} else
						goto _retry;
					goto out;
				case FMDUMP_XEXXXXU:    /* -e -u uuid */
					if ((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid) {
						sprintf(buffer, "%s\t\t\t%s\n",
							record->time, record->eclass);
						printf("TIME\t\t\t\t\tCLASS\n");
						printf("%s", buffer);
					} else
						goto _retry;
					goto out;
				case FMDUMP_XEXXXXX:    /* -e */
					sprintf(buffer, "%s\t%s\n",
						record->time, record->eclass);
					printf("%s", buffer);
					memset(buffer, 0, 256 * sizeof (char));
					goto _retry;
				case FMDUMP_XXVCXXX:    /* -V -c class */
					if (strcmp(arg_class, record->eclass) == 0) {
						sprintf(buffer, "----------------------------"
								"----------------------------\n"
								"Event Time : %s\n"
								"Severity : %s\n"
								"EventIdx : %s\n\n"
								"Event class : %s\n"
								"Affects : %s\n"
								"FRU : %s\n\n"
								"Description : %s\n\n"
								"Response : %s\n\n"
								"Impact : %s\n\n"
								"Action : %s\n\n",
						record->time,
						fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_SEVERITY),
						fmd_msg_getid_evt(g_msg, record->eclass),
						record->eclass, ""/*NULL*/, ""/*NULL*/,
						fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_DESC),
						fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_RESPONSE),
						fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_IMPACT),
						fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_ACTION));
						printf("\n");
						printf("%s", buffer);
					} else
						goto _retry;
					goto out;
				case FMDUMP_XXVXXXU:    /* -V -u uuid */
					if ((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid) {
						sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n"
								"?%\t<serdtype>\n"
								"Affects : %s\n"
								"FRU : %s\n",
							record->time, record->uuid,
							fmd_msg_getid_evt(g_msg, record->eclass),
							""/*NULL*/, ""/*NULL*/);
						printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
						printf("%s", buffer);
					} else
						goto _retry;
					goto out;
				case FMDUMP_XXVXXXX:    /* -V */
					sprintf(buffer, "----------------------------"
							"----------------------------\n"
							"Event Time : %s\n"
							"Severity : %s\n"
							"EventIdx : %s\n\n"
							"Event class : %s\n"
							"Affects : %s\n"
							"FRU : %s\n\n"
							"Description : %s\n\n"
							"Response : %s\n\n"
							"Impact : %s\n\n"
							"Action : %s\n\n",
					record->time,
					fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_SEVERITY),
					fmd_msg_getid_evt(g_msg, record->eclass),
					record->eclass, ""/*NULL*/, ""/*NULL*/,
					fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_DESC),
					fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_RESPONSE),
					fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_IMPACT),
					fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_ACTION));
					printf("\n");
					printf("%s", buffer);
					memset(buffer, 0, 256 * sizeof (char));
					goto _retry;
				default:
					break;
				}
_retry:
				record = record->next;
			}
			log = log->next;
		}
	}
out:
	return;
}

static int
fmdump_tcmp(char *ptime, int flags)
{
	struct tm tm;
	memset(&tm, 0, sizeof (struct tm));
	char *p = NULL;

	if ((p = strptime(ptime, "%Y-%m-%d_%H:%M:%S", &tm)) == NULL) {
		return 0;
	} else if (flags == 1) {	/* -t */
		if (arg_time->tm_year == 0) {
			if (arg_time->tm_sec == 0) {	/* -t "%H:%M" */
				if ((arg_time->tm_hour == tm.tm_hour)
				 && (arg_time->tm_min == tm.tm_min))
					return 1;
				else
					return 0;
			} else {			/* -t "%H:%M:%S" */
				if ((arg_time->tm_hour == tm.tm_hour)
				 && (arg_time->tm_min == tm.tm_min)
				 && (arg_time->tm_sec == tm.tm_sec))
					return 1;
				else
					return 0;
			}
		} else {
			if ((arg_time->tm_year == tm.tm_year)
			 && (arg_time->tm_mon == tm.tm_mon)
			 && (arg_time->tm_mday == tm.tm_mday)) {
				if (arg_time->tm_sec == 0) {
					if (arg_time->tm_min == 0)	/* -t "%Y-%m-%d" */
						return 1;
					else if ((arg_time->tm_hour == tm.tm_hour)
					      && (arg_time->tm_min == tm.tm_min))
						return 1;		/* -t "%Y-%m-%d" "%H:%M" */
					else
						return 0;
				} else if ((arg_time->tm_hour == tm.tm_hour)
					&& (arg_time->tm_min == tm.tm_min)
					&& (arg_time->tm_sec == tm.tm_sec))
					return 1;			/* -t "%Y-%m-%d" "%H:%M:%S" */
				else
					return 0;
			} else
				return 0;
		}
	} else if (flags == 0) {	/* -T */
		/* Get current time */
		struct tm *ctp;
		time_t timep;
		uint64_t timev, ctimev;

		time(&timep);
		ctp = gmtime(&timep);
		ctimev = (uint64_t)(ctp->tm_mon * 30 * 24 * 60 * 60 +
			 ctp->tm_mday * 24 * 60 * 60 +
			 ctp->tm_hour * 60 * 60 +
			 ctp->tm_min * 60 +
			 ctp->tm_sec);
		timev = (uint64_t)(tm.tm_mon * 30 * 24 *60 * 60 +
			 tm.tm_mday * 24 *60 * 60 +
			 tm.tm_hour * 60 * 60 +
			 tm.tm_min * 60 +
			 tm.tm_sec);

		if (ctp->tm_year == tm.tm_year) {
			if (arg_Time->tm_sec != 0) {            /* -T N sec */
				if (ctimev - timev <= arg_Time->tm_sec)
					return 1;
				else
					return 0;
			} else if (arg_Time->tm_min != 0) {	/* -T N min */
				if (ctimev - timev <= (arg_Time->tm_min) * 60)
					return 1;
				else
					return 0;
			} else if (arg_Time->tm_hour != 0) {	/* -T N hour */
				if (ctimev - timev <= (arg_Time->tm_hour) * 60 * 60)
					return 1;
				else
					return 0;
			} else if (arg_Time->tm_mday != 0) {    /* -T N day */
				if (ctimev - timev <= (arg_Time->tm_mday) * 24 * 60 * 60)
					return 1;
				else
					return 0;
			} else
				return 0;
		} else
			return 0;
	} else
		return 0;
}

void
fmdump_tprintf(fmd_log_t *lp, fmd_dump_type_t type, int flags)
{
	fmd_log_record_t *record = NULL;
	fmd_log_t *log = NULL;
	char buffer[PATH_MAX];
	char *stop;
	memset(buffer, 0, 256 * sizeof (char));

	if (flags == 0) {
		record = lp->fst_rec;

		while (record) {
			switch (type) {
			case FMDUMP_AXXCTXX:	/* -a -c class -t time */
				if ((strcmp(arg_class, record->eclass) == 0)
				&& (fmdump_tcmp(record->time, 1) == 1)) {
					sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n",
						record->time, record->uuid,
						fmd_msg_getid_evt(g_msg, record->eclass));
					printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
					printf("%s", buffer);
				} else
					goto retry;
				goto out;
			case FMDUMP_AXXCXTX:	/* -a -c class -T time */
				if ((strcmp(arg_class, record->eclass) == 0)
				&& (fmdump_tcmp(record->time, 0) == 1)) {
					sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n",
						record->time, record->uuid,
						fmd_msg_getid_evt(g_msg, record->eclass));
					printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
					printf("%s", buffer);
				} else
					goto retry;
				goto out;
			case FMDUMP_AXXXTXU:	/* -a -t time -u uuid */
				if (((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid)
				&& (fmdump_tcmp(record->time, 1) == 1)) {
                                        sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n",
                                                record->time, record->uuid,
                                                fmd_msg_getid_evt(g_msg, record->eclass));
                                        printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
                                        printf("%s", buffer);
                                } else 
                                        goto retry;
                                goto out; 
			case FMDUMP_AXXXXTU:	/* -a -T time -u uuid */
				if (((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid)
				&& (fmdump_tcmp(record->time, 0) == 1)) {
					sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n",
						record->time, record->uuid,
						fmd_msg_getid_evt(g_msg, record->eclass));
					printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
					printf("%s", buffer);
				} else
					goto retry;
				goto out;
			case FMDUMP_AXXXTXX:	/* -a -t time */
				if (fmdump_tcmp(record->time, 1) == 1) {
					sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n",
                                        	record->time, record->uuid,
                                        	fmd_msg_getid_evt(g_msg, record->eclass));
                                	printf("%s", buffer);
                                	memset(buffer, 0, 256 * sizeof (char));
				}
                                goto retry;
			case FMDUMP_AXXXXTX:	/* -a -T time */
				if (fmdump_tcmp(record->time, 0) == 1) {
					sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n",
						record->time, record->uuid,
						fmd_msg_getid_evt(g_msg, record->eclass));
					printf("%s", buffer);
					memset(buffer, 0, 256 * sizeof (char));
				}
				goto retry;
			case FMDUMP_XEXCTXX:	/* -e -c class -t time */
				if ((strcmp(arg_class, record->eclass) == 0)
				&& (fmdump_tcmp(record->time, 1) == 1)) {
                                        sprintf(buffer, "%s\t\t\t%s\n",
                                                record->time, record->eclass);
                                        printf("TIME\t\t\t\t\tCLASS\n");
                                        printf("%s", buffer);
                                } else
                                        goto retry;
                                goto out;
			case FMDUMP_XEXCXTX:	/* -e -c class -T time */
				if ((strcmp(arg_class, record->eclass) == 0)
				&& (fmdump_tcmp(record->time, 0) == 1)) {
					sprintf(buffer, "%s\t\t\t%s\n",
						record->time, record->eclass);
					printf("TIME\t\t\t\t\tCLASS\n");
					printf("%s", buffer);
				} else
					goto retry;
				goto out;
			case FMDUMP_XEXXTXU:	/* -e -t time -u uuid */
				if (((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid)
				&& (fmdump_tcmp(record->time, 1) == 1)) {
                                        sprintf(buffer, "%s\t\t\t%s\n",
                                                record->time, record->eclass);
                                        printf("TIME\t\t\t\t\tCLASS\n");
                                        printf("%s", buffer);
                                } else
                                        goto retry;
                                goto out;
			case FMDUMP_XEXXXTU:	/* -e -T time -u uuid */
				if (((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid) 
				&& (fmdump_tcmp(record->time, 0) == 1)) {
					sprintf(buffer, "%s\t\t\t%s\n",
						record->time, record->eclass);
					printf("TIME\t\t\t\t\tCLASS\n");
					printf("%s", buffer);
				} else  
					goto retry;
				goto out;
			case FMDUMP_XEXXTXX:	/* -e -t time */
				if (fmdump_tcmp(record->time, 1) == 1) {
					sprintf(buffer, "%s\t%s\n",
                                        	record->time, record->eclass);
                                	printf("%s", buffer);
                                	memset(buffer, 0, 256 * sizeof (char));
                                }
				goto retry;
			case FMDUMP_XEXXXTX:	/* -e -T time */
				if (fmdump_tcmp(record->time, 0) == 1) {
					sprintf(buffer, "%s\t%s\n",
						record->time, record->eclass);
					printf("%s", buffer);
					memset(buffer, 0, 256 * sizeof (char));
				}
				goto retry;
			case FMDUMP_XXVCTXX:	/* -V -c class -t time */
				if ((strcmp(arg_class, record->eclass) == 0)
				&& (fmdump_tcmp(record->time, 1) == 1)) {
                                        sprintf(buffer, "----------------------------"
							"----------------------------\n"
							"Event Time : %s\n"
                                                        "Severity : %s\n"
                                                        "EventIdx : %s\n\n"
                                                        "Event class : %s\n"
                                                        "Affects : %s\n"
                                                        "FRU : %s\n\n"
                                                        "Description : %s\n\n"
                                                        "Response : %s\n\n"
                                                        "Impact : %s\n\n"
                                                        "Action : %s\n\n",
                                                record->time,
                                                fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_SEVERITY),
                                                fmd_msg_getid_evt(g_msg, record->eclass),
                                                record->eclass, ""/*NULL*/, ""/*NULL*/,
                                                fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_DESC),
                                                fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_RESPONSE),
                                                fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_IMPACT),
                                                fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_ACTION));
                                        printf("\n");
                                        printf("%s", buffer);
                                } else
                                        goto retry;
                                goto out;
			case FMDUMP_XXVCXTX:	/* -V -c class -T time */
				if ((strcmp(arg_class, record->eclass) == 0)
                                && (fmdump_tcmp(record->time, 0) == 1)) {
                                        sprintf(buffer, "----------------------------"
							"----------------------------\n"
							"Event Time : %s\n"
                                                        "Severity : %s\n"
                                                        "EventIdx : %s\n\n"
                                                        "Event class : %s\n"
                                                        "Affects : %s\n"
                                                        "FRU : %s\n\n"
                                                        "Description : %s\n\n"
                                                        "Response : %s\n\n"
                                                        "Impact : %s\n\n"
                                                        "Action : %s\n\n",
                                                record->time,
                                                fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_SEVERITY),
                                                fmd_msg_getid_evt(g_msg, record->eclass),
                                                record->eclass, ""/*NULL*/, ""/*NULL*/,
                                                fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_DESC),
                                                fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_RESPONSE),
                                                fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_IMPACT),
                                                fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_ACTION));
                                        printf("\n");
                                        printf("%s", buffer);
                                } else 
                                        goto retry;
                                goto out; 
			case FMDUMP_XXVXTXU:	/* -V -t time -u uuid */
				if (((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid)
				&& (fmdump_tcmp(record->time, 1) == 1)) {
                                        sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n"
                                                        "?%\t<serdtype>\n"
                                                        "Affects : %s\n"
                                                        "FRU : %s\n",
                                                record->time, record->uuid,
                                                fmd_msg_getid_evt(g_msg, record->eclass),
                                                ""/*NULL*/, ""/*NULL*/);
                                        printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
                                        printf("%s", buffer);
                                } else
                                        goto retry;
                                goto out;
			case FMDUMP_XXVXXTU:	/* -V -T time -u uuid */
				if (((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid)
                                && (fmdump_tcmp(record->time, 0) == 1)) {
                                        sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n"
                                                        "?%\t<serdtype>\n"
                                                        "Affects : %s\n"
                                                        "FRU : %s\n",
                                                record->time, record->uuid,
                                                fmd_msg_getid_evt(g_msg, record->eclass),
                                                ""/*NULL*/, ""/*NULL*/);
                                        printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
                                        printf("%s", buffer);
                                } else
                                        goto retry;
                                goto out;
			case FMDUMP_XXVXTXX:	/* -V -t time */
				if (fmdump_tcmp(record->time, 1) == 1) {
					sprintf(buffer, "----------------------------"
							"----------------------------\n"
							"Event Time : %s\n"
                                        	        "Severity : %s\n"
                                                	"EventIdx : %s\n\n"
                                               		"Event class : %s\n"
                                                	"Affects : %s\n"
                                                	"FRU : %s\n\n"
                                                	"Description : %s\n\n"
                                                	"Response : %s\n\n"
                                                	"Impact : %s\n\n"
                                                	"Action : %s\n\n",
                                        	record->time,
                                        	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_SEVERITY),
                                        	fmd_msg_getid_evt(g_msg, record->eclass),
                                        	record->eclass, ""/*NULL*/, ""/*NULL*/,
                                        	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_DESC),
                                        	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_RESPONSE),
                                        	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_IMPACT),
                                        	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_ACTION));
                                	printf("\n");
                                	printf("%s", buffer);
                                	memset(buffer, 0, 256 * sizeof (char));
				}
                                goto retry;
			case FMDUMP_XXVXXTX:	/* -V -T time */
				if (fmdump_tcmp(record->time, 0) == 1) {
                                        sprintf(buffer, "----------------------------"
							"----------------------------\n"
							"Event Time : %s\n"
                                                        "Severity : %s\n"
                                                        "EventIdx : %s\n\n"
                                                        "Event class : %s\n"
                                                        "Affects : %s\n"
                                                        "FRU : %s\n\n"
                                                        "Description : %s\n\n"
                                                        "Response : %s\n\n"
                                                        "Impact : %s\n\n"
                                                        "Action : %s\n\n",
                                                record->time,
                                                fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_SEVERITY),
                                                fmd_msg_getid_evt(g_msg, record->eclass),
                                                record->eclass, ""/*NULL*/, ""/*NULL*/,
                                                fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_DESC),
                                                fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_RESPONSE),
                                                fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_IMPACT),
                                                fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_ACTION));
                                        printf("\n");
                                        printf("%s", buffer);
                                        memset(buffer, 0, 256 * sizeof (char));
                                }
                                goto retry;
			default:
                                break;
                        }
retry:
                        record = record->next;
                }
        } else if (flags == 1) {
                log = lp->next;

                while (log) {
                        record = log->fst_rec;

                        while (record) {
                                switch (type) {
				case FMDUMP_AXXCTXX:    /* -a -c class -t time */
                                	if ((strcmp(arg_class, record->eclass) == 0)
                                	&& (fmdump_tcmp(record->time, 1) == 1)) {
                                        	sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n",
                                                	record->time, record->uuid,
                                                	fmd_msg_getid_evt(g_msg, record->eclass));
                                        	printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
                                        	printf("%s", buffer);
                                	} else
                                        	goto _retry;
                                	goto out;
                        	case FMDUMP_AXXCXTX:    /* -a -c class -T time */
                                	if ((strcmp(arg_class, record->eclass) == 0)
                                	&& (fmdump_tcmp(record->time, 0) == 1)) {
                                        	sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n",
                                                	record->time, record->uuid,
                                                	fmd_msg_getid_evt(g_msg, record->eclass));
                                        	printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
                                        	printf("%s", buffer);
                                	} else
                                        	goto _retry;
                                	goto out;
				case FMDUMP_AXXXTXU:    /* -a -t time -u uuid */
                                	if (((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid)
                                	&& (fmdump_tcmp(record->time, 1) == 1)) {
                                        	sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n",
                                                	record->time, record->uuid,
                                                	fmd_msg_getid_evt(g_msg, record->eclass));
                                        	printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
                                        	printf("%s", buffer);
                                	} else
                                        	goto _retry;
                                	goto out;
                        	case FMDUMP_AXXXXTU:    /* -a -T time -u uuid */
                                	if (((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid)
                                	&& (fmdump_tcmp(record->time, 0) == 1)) {
                                        	sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n",
                                                	record->time, record->uuid,
                                                	fmd_msg_getid_evt(g_msg, record->eclass));
                                        	printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
                                        	printf("%s", buffer);
                                	} else
                                        	goto _retry;
                                	goto out;
				case FMDUMP_AXXXTXX:    /* -a -t time */
                                	if (fmdump_tcmp(record->time, 1) == 1) {
                                        	sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n",
                                                	record->time, record->uuid,
                                                	fmd_msg_getid_evt(g_msg, record->eclass));
                                        	printf("%s", buffer);
                                        	memset(buffer, 0, 256 * sizeof (char));
                                	}
                                	goto _retry;
                        	case FMDUMP_AXXXXTX:    /* -a -T time */
                                	if (fmdump_tcmp(record->time, 0) == 1) {
                                        	sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n",
                                                	record->time, record->uuid,
                                                	fmd_msg_getid_evt(g_msg, record->eclass));
                                        	printf("%s", buffer);
                                       		memset(buffer, 0, 256 * sizeof (char));
                                	}
                                	goto _retry;
				case FMDUMP_XEXCTXX:    /* -e -c class -t time */
                                	if ((strcmp(arg_class, record->eclass) == 0)
                                	&& (fmdump_tcmp(record->time, 1) == 1)) {
                                        	sprintf(buffer, "%s\t\t\t%s\n",
                                                	record->time, record->eclass);
                                        	printf("TIME\t\t\t\t\tCLASS\n");
                                        	printf("%s", buffer);
                                	} else
                                        	goto _retry;
                                	goto out;
                        	case FMDUMP_XEXCXTX:    /* -e -c class -T time */
                                	if ((strcmp(arg_class, record->eclass) == 0)
                                	&& (fmdump_tcmp(record->time, 0) == 1)) {
                                        	sprintf(buffer, "%s\t\t\t%s\n",
                                                	record->time, record->eclass);
                                        	printf("TIME\t\t\t\t\tCLASS\n");
                                        	printf("%s", buffer);
                               		} else
                                        	goto _retry;
                                	goto out;
				case FMDUMP_XEXXTXU:    /* -e -t time -u uuid */
                                	if (((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid)
                                	&& (fmdump_tcmp(record->time, 1) == 1)) {
                                        	sprintf(buffer, "%s\t\t\t%s\n",
                                                	record->time, record->eclass);
                                        	printf("TIME\t\t\t\t\tCLASS\n");
                                        	printf("%s", buffer);
                                	} else
                                        	goto _retry;
                                	goto out;
                        	case FMDUMP_XEXXXTU:    /* -e -T time -u uuid */
                                	if (((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid)
                                	&& (fmdump_tcmp(record->time, 0) == 1)) {
                                        	sprintf(buffer, "%s\t\t\t%s\n",
                                                	record->time, record->eclass);
                                        	printf("TIME\t\t\t\t\tCLASS\n");
                                        	printf("%s", buffer);
                                	} else
                                        	goto _retry;
                                	goto out;
                        	case FMDUMP_XEXXTXX:    /* -e -t time */
                                	if (fmdump_tcmp(record->time, 1) == 1) {
                                        	sprintf(buffer, "%s\t%s\n",
                                                	record->time, record->eclass);
                                        	printf("%s", buffer);
                                        	memset(buffer, 0, 256 * sizeof (char));
                                	}
                                	goto _retry;
				case FMDUMP_XEXXXTX:    /* -e -T time */
                                	if (fmdump_tcmp(record->time, 0) == 1) {
                                        	sprintf(buffer, "%s\t%s\n",
                                                	record->time, record->eclass);
                                        	printf("%s", buffer);
                                       		memset(buffer, 0, 256 * sizeof (char));
                                	}
                                	goto _retry;
                        	case FMDUMP_XXVCTXX:    /* -V -c class -t time */
                                	if ((strcmp(arg_class, record->eclass) == 0)
                                	&& (fmdump_tcmp(record->time, 1) == 1)) {
                                        	sprintf(buffer, "----------------------------"
								"----------------------------\n"
								"Event Time : %s\n"
                                                	        "Severity : %s\n"
                                                        	"EventIdx : %s\n\n"
                                                        	"Event class : %s\n"
                                                        	"Affects : %s\n"
                                                        	"FRU : %s\n\n"
                                                        	"Description : %s\n\n"
                                                        	"Response : %s\n\n"
                                                        	"Impact : %s\n\n"
                                                        	"Action : %s\n\n",
                                                	record->time,
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_SEVERITY),
                                                	fmd_msg_getid_evt(g_msg, record->eclass),
                                                	record->eclass, ""/*NULL*/, ""/*NULL*/,
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_DESC),
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_RESPONSE),
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_IMPACT),
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_ACTION));
                                        	printf("\n");
                                        	printf("%s", buffer);
                                	} else
						goto _retry;
                                	goto out;
                        	case FMDUMP_XXVCXTX:    /* -V -c class -T time */
                                	if ((strcmp(arg_class, record->eclass) == 0)
                                	&& (fmdump_tcmp(record->time, 0) == 1)) {
                                        	sprintf(buffer, "----------------------------"
								"----------------------------\n"
								"Event Time : %s\n"
                                                	        "Severity : %s\n"
                                                        	"EventIdx : %s\n\n"
                                                        	"Event class : %s\n"
                                                        	"Affects : %s\n"
                                                        	"FRU : %s\n\n"
                                                        	"Description : %s\n\n"
                                                        	"Response : %s\n\n"
                                                        	"Impact : %s\n\n"
                                                        	"Action : %s\n\n",
                                                	record->time,
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_SEVERITY),
                                                	fmd_msg_getid_evt(g_msg, record->eclass),
                                                	record->eclass, ""/*NULL*/, ""/*NULL*/,
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_DESC),
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_RESPONSE),
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_IMPACT),
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_ACTION));
                                        	printf("\n");
                                        	printf("%s", buffer);
                                	} else
                                        	goto _retry;
                                	goto out;
				case FMDUMP_XXVXTXU:    /* -V -t time -u uuid */
                                	if (((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid)
                                	&& (fmdump_tcmp(record->time, 1) == 1)) {
                                        	sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n"
                                                	        "?%\t<serdtype>\n"
                                                        	"Affects : %s\n"
                                                        	"FRU : %s\n",
                                                	record->time, record->uuid,
                                                	fmd_msg_getid_evt(g_msg, record->eclass),
                                                	""/*NULL*/, ""/*NULL*/);
                                        	printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
                                        	printf("%s", buffer);
                                	} else
                                        	goto _retry;
                                	goto out;
                        	case FMDUMP_XXVXXTU:    /* -V -T time -u uuid */
                                	if (((uint64_t)strtol(arg_uuid, &stop, 0) == record->uuid)
                                	&& (fmdump_tcmp(record->time, 0) == 1)) {
                                        	sprintf(buffer, "%s\t\t0x%llx\t\t\t\t%s\n"
                                                	        "?%\t<serdtype>\n"
                                                        	"Affects : %s\n"
                                                        	"FRU : %s\n",
                                                	record->time, record->uuid,
                                                	fmd_msg_getid_evt(g_msg, record->eclass),
                                                	""/*NULL*/, ""/*NULL*/);
                                        	printf("TIME\t\t\t\tUUID\t\t\t\tMSG-ID\n");
                                        	printf("%s", buffer);
                                	} else
                                        	goto _retry;
                                	goto out;
				case FMDUMP_XXVXTXX:    /* -V -t time */
                                	if (fmdump_tcmp(record->time, 1) == 1) {
                                        	sprintf(buffer, "----------------------------"
								"----------------------------\n"
								"Event Time : %s\n"
                                                	        "Severity : %s\n"
                                                        	"EventIdx : %s\n\n"
                                                        	"Event class : %s\n"
                                                        	"Affects : %s\n"
                                                        	"FRU : %s\n\n"
                                                        	"Description : %s\n\n"
                                                        	"Response : %s\n\n"
                                                        	"Impact : %s\n\n"
                                                        	"Action : %s\n\n",
                                                	record->time,
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_SEVERITY),
                                                	fmd_msg_getid_evt(g_msg, record->eclass),
                                                	record->eclass, ""/*NULL*/, ""/*NULL*/,
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_DESC),
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_RESPONSE),
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_IMPACT),
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_ACTION));
                                        	printf("\n");
                                        	printf("%s", buffer);
                                        	memset(buffer, 0, 256 * sizeof (char));
                                	}
                                	goto _retry;
				case FMDUMP_XXVXXTX:    /* -V -T time */
                                	if (fmdump_tcmp(record->time, 0) == 1) {
                                        	sprintf(buffer, "----------------------------"
								"----------------------------\n"
								"Event Time : %s\n"
                                                	        "Severity : %s\n"
                                                        	"EventIdx : %s\n\n"
                                                        	"Event class : %s\n"
                                                        	"Affects : %s\n"
                                                        	"FRU : %s\n\n"
                                                        	"Description : %s\n\n"
                                                        	"Response : %s\n\n"
                                                        	"Impact : %s\n\n"
                                                        	"Action : %s\n\n",
                                                	record->time,
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_SEVERITY),
                                                	fmd_msg_getid_evt(g_msg, record->eclass),
                                                	record->eclass, ""/*NULL*/, ""/*NULL*/,
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_DESC),
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_RESPONSE),
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_IMPACT),
                                                	fmd_msg_getitem_evt(g_msg, record->eclass, FMD_MSG_ITEM_ACTION));
                                        	printf("\n");
                                        	printf("%s", buffer);
                                        	memset(buffer, 0, 256 * sizeof (char));
                                	}
                                	goto _retry;
                        	default:
                                	break;
                        	}//switch
_retry:
                                record = record->next;
                        }//while
                        log = log->next;
                }//while
        }//if
out:
        return;
}

static struct tm *
gettimeopt(const char *arg)
{
	char const *suffix[] = {
		"s", "sec",
		"m", "min",
		"h", "hour",
		"d", "day",
		NULL
	};

	struct tm *tm = malloc(sizeof (struct tm));
	assert(tm != NULL);
	memset(tm, 0, sizeof (struct tm));
	char *p;

	if ((p = strptime(arg, "%m/%d/%Y" "%t" "%H:%M:%S", tm)) == NULL &&
	    (p = strptime(arg, "%m/%d/%Y" "%t" "%H:%M", tm)) == NULL &&
            (p = strptime(arg, "%m/%d/%Y", tm)) == NULL &&
            (p = strptime(arg, "%Y/%m/%d" "%t" "%H:%M:%S", tm)) == NULL &&
            (p = strptime(arg, "%Y/%m/%d" "%t" "%H:%M", tm)) == NULL &&
            (p = strptime(arg, "%Y/%m/%d", tm)) == NULL &&
            (p = strptime(arg, "%Y-%m-%d" "%t" "%H:%M:%S", tm)) == NULL &&
            (p = strptime(arg, "%Y-%m-%d" "%t" "%H:%M", tm)) == NULL &&
            (p = strptime(arg, "%Y-%m-%d", tm)) == NULL &&
            (p = strptime(arg, "%H:%M:%S", tm)) == NULL &&
            (p = strptime(arg, "%H:%M", tm)) == NULL) {
		/* -T args */
		memset(tm, 0, sizeof (struct tm));
		int j, result = 0;
		for (j = 0; suffix[j] != NULL; j++) {
			if (strstr(arg, suffix[j]) != NULL) {
				result = atoi(arg);
				if (j == 0 || j == 1) {
					tm->tm_sec = result;
					break;
				} else if (j == 2 || j == 3) {
					tm->tm_min = result;
					break;
				} else if (j == 4 || j == 5) {
					tm->tm_hour = result;
					break;
				}else if (j == 6 || j == 7) {
					tm->tm_mday = result;
					break;
				}
			}
		}

		if (suffix[j] == NULL) {
			(void) fprintf(stderr, "fmdump: illegal time "
				"format -- %s\n", arg);
				usage(stderr);
				exit(FMDUMP_EXIT_USAGE);
		}
	} else {/* -t time */
		/* "%H:%M:%S" or "%H:%M" */
		if (tm->tm_year < 0) {
			tm->tm_year = 0;
			tm->tm_mon = 0;
			tm->tm_mday = 0;
		}
		/* just ignore other cases */
	}

	return (tm);
}

static fmd_log_t *
fmd_log_alloc(void)
{
	fmd_log_t *lp = (fmd_log_t *)
		malloc(sizeof (fmd_log_t));

	lp->fst_rec = NULL;
	lp->next = NULL;

	return (lp);
}

static fmd_log_record_t *
fmd_log_rec_alloc(void)
{
	fmd_log_record_t *lrp = (fmd_log_record_t *)
		malloc(sizeof (fmd_log_record_t));

	memset(lrp, 0, sizeof (fmd_log_record_t));
	lrp->next = NULL;

	return (lrp);
}

static void
add_record(fmd_log_t *lp, fmd_log_record_t *lrp)
{
	struct fmd_log_record *record = lp->fst_rec;

	if (record == NULL)
		lp->fst_rec = lrp;
	else {
		while (record) {
			if (record->next == NULL) {
				record->next = lrp;
				return;
			}
			record = record->next;
		}
	}
}

static void
add_log(fmd_log_t *head, fmd_log_t *lp)
{
	struct fmd_log *log = head->next;

	if (log == NULL)
		head->next = lp;
	else {
		while (log) {
			if (log->next == NULL) {
				log->next = lp;
				return;
			}
			log = log->next;
		}
	}
}

fmd_log_t *
fmd_log_open(const char *filename)
{
	char *p = NULL;
	char *delims = {"\t"};
	char buffer[256] = "";
	char file_name[50] = "";
	FILE *fp = NULL;

	memset(file_name, 0, 50 * sizeof (char));

	fmd_log_t *lp = fmd_log_alloc();
	if (lp == NULL)
		return (NULL);

	if (strstr(filename, "-err") != NULL)
		sprintf(file_name, "/var/log/fm/errlog/%s", filename);
	else if (strstr(filename, "-flt") != NULL)
		sprintf(file_name, "/var/log/fm/fltlog/%s", filename);
	else if (strstr(filename, "-lst") != NULL)
		sprintf(file_name, "/var/log/fm/lstlog/%s", filename);

	if ((fp = fopen(file_name, "r+")) == NULL) {
		printf("fmdump: failed to open log file %s\n", file_name);
		return (NULL);
	}

	while (fgets(buffer, sizeof(buffer), fp)) {
		if ((buffer[0] == '\n') || (buffer[0] == '\0'))
			continue;		/* skip blank line */
		p = strtok(buffer, delims);
		int i = 0;
		char *stop = NULL;
		fmd_log_record_t *lrp = fmd_log_rec_alloc();
		if (lrp == NULL)
			return (NULL);

		while (p != NULL) {
			switch (++i) {
			case 1:
				sprintf(lrp->time, "%s", p);
				break;
			case 2:
				sprintf(lrp->eclass, "%s", p);
				break;
			case 3:
				lrp->rscid = (uint64_t)strtol(p, &stop, 0);
				break;
			case 4:
				lrp->eid = (uint64_t)strtol(p, &stop, 0);
				break;
			case 5:
				lrp->uuid = (uint64_t)strtol(p, &stop, 0);
				break;
			default:
				break;
			}
			p = strtok(NULL, delims);
		}
		memset(buffer, 0, 256 * sizeof(char));
		add_record(lp, lrp);
	}
	fclose(fp);

	return (lp);
}

void
fmd_log_close(fmd_log_t *lp)
{
	fmd_log_t *xlp, *clp;
	fmd_log_record_t *lrp, *crp;

	if (lp == NULL)
		return; /* permit null lp to simply caller code */

	lrp = lp->fst_rec;
	while (lrp != NULL) {
		crp = lrp->next;
		free(lrp);
		lrp = crp;
	}

	while (lp != NULL) {
		clp = lp->next;
		free(lp);
		lp = clp;
	}
}

/*
 * Find and return all the rotated logs.
 */
static fmd_log_t *
get_rotated_logs(char *logpath)
{
	DIR *dirp;
	struct dirent *dp;
	const char *p;
	fmd_log_t *head = fmd_log_alloc();
	if (head == NULL)
		return (NULL);

	if ((dirp = opendir(logpath)) == NULL)
		return; /* failed to open directory; just skip it */

	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_name[0] == '.')
			continue; /* skip "." and ".." */

		p = dp->d_name;

		/* check name */
		assert(p != NULL);

		fmd_log_t *lp;
		lp = fmd_log_open(p);
		if (lp == NULL)
			return (NULL);

		add_log(head, lp);
	}

	(void) closedir(dirp);

	return (head);
}

int
main(int argc, char *argv[])
{
	int opt_a = 0, opt_e = 0, opt_V = 0, opt_c = 0;
	int opt_t = 0, opt_T = 0, opt_u = 0, opt_v = 0;
	int rotated_logs = 0;
	int c, i, err;
	char arg_file[256];
	fmd_log_t *lp;

	memset(arg_class, 0, 256 * sizeof (char));
	memset(arg_uuid, 0, 256 * sizeof (char));
	memset(arg_file, 0, 256 * sizeof (char));
	memset(arg_t, 0, 256 * sizeof (char));
	memset(arg_T, 0, 256 * sizeof (char));

	while (optind < argc) {
		while ((c =
		    getopt(argc, argv, "aeVc:t:T:u:h?")) != EOF) {
			switch (c) {
			case 'a':
				opt_a++;
				break;
			case 'e':
				opt_e++;
				break;
			case 'V':
				opt_V++;
				break;
			case 'c':
				opt_c++;
				strcpy(arg_class, optarg);
				break;
			case 't':
				opt_t++;
				strcpy(arg_t, optarg);
				arg_time = gettimeopt(arg_t);
				break;
			case 'T':
				opt_T++;
				strcpy(arg_T, optarg);
				arg_Time = gettimeopt(arg_T);
				break;
			case 'u':
				opt_u++;
				strcpy(arg_uuid, optarg);
//				opt_a++; /* -u implies -a */
				break;
			case 'h':
			case '?':
				return (usage(stderr));
			default:
				break;
			}
		}

		if (optind < argc) {
			if (*arg_file != '\0') {
				(void) fprintf(stderr, "fmdump: illegal "
				    "argument -- %s\n", argv[optind]);
				return (FMDUMP_EXIT_USAGE);
			} else {
				strcpy(arg_file, argv[argc - 1]);
			}
			optind++;
		}
	}

	if (argc == 1)
		return (usage(stderr));

	if (((opt_t == 1) && (opt_T == 1))
	  ||((opt_c == 1) && (opt_u == 1)))
		return (usage(stderr));

	if (*arg_file == '\0') {
		if (opt_c) {
			if (strncmp(arg_class, "ereport.", 8) == 0)
				(void) snprintf(arg_file, sizeof (arg_file), "/var/log/fm/errlog");
			else if (strncmp(arg_class, "fault.", 6) == 0)
				(void) snprintf(arg_file, sizeof (arg_file), "/var/log/fm/fltlog");
			else if (strncmp(arg_class, "list.", 5) == 0)
				(void) snprintf(arg_file, sizeof (arg_file), "/var/log/fm/lstlog");
		} else if (opt_u)
			(void) snprintf(arg_file, sizeof (arg_file), "/var/log/fm/fltlog");
		else
			(void) snprintf(arg_file, sizeof (arg_file), "/var/log/fm/errlog");
		
		/*
		 * logadm may rotate the logs.  When no input file is specified,
		 * we try to dump all the rotated logs as well in the right
		 * order.
		 */
		rotated_logs = 1;	/* all log files in the same dir */
	} else
		rotated_logs = 0;	/* the special log file */

	if ((g_msg = fmd_msg_init(FMD_MSG_VERSION)) == NULL) {
		(void) fprintf(stderr, "fmdump: failed to initialize "
		    "libfmd_msg: %s\n", strerror(errno));
		return (FMDUMP_EXIT_FATAL);
	}

	if (rotated_logs == 0) {
		if ((lp = fmd_log_open(arg_file)) == NULL) {
			(void) fprintf(stderr, "fmdump: failed to open %s.\n",
			    arg_file);
			return (FMDUMP_EXIT_FATAL);
		}
	} else if (rotated_logs == 1) {
		if ((lp = get_rotated_logs(arg_file)) == NULL) {
			(void) fprintf(stderr, "fmdump: failed to open files in %s.\n",
			    arg_file);
			return (FMDUMP_EXIT_FATAL);
		}
	}

	opt_v = !opt_a && !opt_e && !opt_V;
	if (opt_a && opt_e && opt_V)
		return (usage(stderr));
	else if (opt_a && !opt_e && !opt_V) {
		if (opt_c && !opt_u) {
			if (opt_t && !opt_T)
				fmdump_tprintf(lp, FMDUMP_AXXCTXX, rotated_logs);
			else if (opt_T && !opt_t)
				fmdump_tprintf(lp, FMDUMP_AXXCXTX, rotated_logs);
			else
				fmdump_vprintf(lp, FMDUMP_AXXCXXX, rotated_logs);
		} else if (opt_u && !opt_c) {
			if (opt_t && !opt_T)
				fmdump_tprintf(lp, FMDUMP_AXXXTXU, rotated_logs);
			else if (opt_T && !opt_t)
				fmdump_tprintf(lp, FMDUMP_AXXXXTU, rotated_logs);
			else
				fmdump_vprintf(lp, FMDUMP_AXXXXXU, rotated_logs);
		} else if (!opt_u && !opt_c) {
			if (opt_t && !opt_T)
				fmdump_tprintf(lp, FMDUMP_AXXXTXX, rotated_logs);
			else if (opt_T && !opt_t)
				fmdump_tprintf(lp, FMDUMP_AXXXXTX, rotated_logs);
			else
				fmdump_vprintf(lp, FMDUMP_AXXXXXX, rotated_logs);
		} else if (opt_u && opt_c)
			return (usage(stderr));
	} else if (opt_e && !opt_a && !opt_V) {
		if (opt_c && !opt_u) {
			if (opt_t && !opt_T)
				fmdump_tprintf(lp, FMDUMP_XEXCTXX, rotated_logs);
			else if (opt_T && !opt_t)
                                fmdump_tprintf(lp, FMDUMP_XEXCXTX, rotated_logs);
                        else
                                fmdump_vprintf(lp, FMDUMP_XEXCXXX, rotated_logs);
                } else if (opt_u && !opt_c) {
                        if (opt_t && !opt_T)
                                fmdump_tprintf(lp, FMDUMP_XEXXTXU, rotated_logs);
                        else if (opt_T && !opt_t)
                                fmdump_tprintf(lp, FMDUMP_XEXXXTU, rotated_logs);
                        else
                                fmdump_vprintf(lp, FMDUMP_XEXXXXU, rotated_logs);
                } else if (!opt_u && !opt_c) {
                        if (opt_t && !opt_T)
                                fmdump_tprintf(lp, FMDUMP_XEXXTXX, rotated_logs);
                        else if (opt_T && !opt_t)
                                fmdump_tprintf(lp, FMDUMP_XEXXXTX, rotated_logs);
                        else
                                fmdump_vprintf(lp, FMDUMP_XEXXXXX, rotated_logs);
                } else if (opt_u && opt_c)
                        return (usage(stderr));
	} else if ((opt_V && !opt_a && !opt_e) || opt_v) {
		if (opt_c && !opt_u) {
                        if (opt_t && !opt_T)
                                fmdump_tprintf(lp, FMDUMP_XXVCTXX, rotated_logs);
                        else if (opt_T && !opt_t)
                                fmdump_tprintf(lp, FMDUMP_XXVCXTX, rotated_logs);
                        else
                                fmdump_vprintf(lp, FMDUMP_XXVCXXX, rotated_logs);
                } else if (opt_u && !opt_c) {
                        if (opt_t && !opt_T)
                                fmdump_tprintf(lp, FMDUMP_XXVXTXU, rotated_logs);
                        else if (opt_T && !opt_t)
                                fmdump_tprintf(lp, FMDUMP_XXVXXTU, rotated_logs);
                        else
                                fmdump_vprintf(lp, FMDUMP_XXVXXXU, rotated_logs);
                } else if (!opt_u && !opt_c) {
                        if (opt_t && !opt_T)
                                fmdump_tprintf(lp, FMDUMP_XXVXTXX, rotated_logs);
                        else if (opt_T && !opt_t)
                                fmdump_tprintf(lp, FMDUMP_XXVXXTX, rotated_logs);
                        else
                                fmdump_vprintf(lp, FMDUMP_XXVXXXX, rotated_logs);
                } else if (opt_u && opt_c)
                        return (usage(stderr));
	} else
		return (usage(stderr));

	fmd_log_close(lp);
	fmd_msg_fini(g_msg);

	return (FMDUMP_EXIT_SUCCESS);
}

