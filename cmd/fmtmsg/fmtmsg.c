/*
 * fmtmsg.c
 *
 *  Created on: Nov 1, 2010
 *	Author: Inspur OS Team
 *
 *  Description:
 *	fmtmsg.c
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

#include "fmtmsg.h"

#define	FMTMSG_EXIT_SUCCESS	0
#define	FMTMSG_EXIT_FATAL	1
#define	FMTMSG_EXIT_USAGE	2
#define	FMTMSG_EXIT_ERROR	3

static char arg_ecode[256];
static char arg_cclass[256];
static char arg_nclass[256];

fmd_msg_hdl_t *g_msg = NULL;

static int
usage(FILE *fp) 
{
        (void) fprintf(fp, "Usage: fmtmsg [-i <code>] [-c <class>] [-n <class>]\n");

        (void) fprintf(fp,
            "\t-i  display the entire message for the given event code\n"
            "\t-c  display the entire message for the given event\n"
            "\t-n  display the event code for the given event\n");

        return (FMTMSG_EXIT_USAGE);
}

void
fmtmsg_printf(fmtmsg_type_t type)
{
	char buffer[PATH_MAX];
	memset(buffer, 0, 256 * sizeof (char));

	switch (type) {
		case FMTMSG_ECODE:	/* -i code */
			sprintf(buffer, "----------------------------"
                                        "----------------------------\n"
                                        "EventIdx : %s\n"
					"Severity : %s\n"
                                        "Event class : %s\n\n"
                                        "Description : %s\n\n"
                                        "Response : %s\n\n"
                                        "Impact : %s\n\n"
                                        "Action : %s\n\n",
				arg_ecode,
				fmd_msg_getitem_id(g_msg, arg_ecode, FMD_MSG_ITEM_SEVERITY),
				fmd_msg_getitem_id(g_msg, arg_ecode, FMD_MSG_ITEM_CLASS),
				fmd_msg_getitem_id(g_msg, arg_ecode, FMD_MSG_ITEM_DESC),
				fmd_msg_getitem_id(g_msg, arg_ecode, FMD_MSG_ITEM_RESPONSE),
				fmd_msg_getitem_id(g_msg, arg_ecode, FMD_MSG_ITEM_IMPACT),
				fmd_msg_getitem_id(g_msg, arg_ecode, FMD_MSG_ITEM_ACTION));
                        printf("%s", buffer);
			break;
		case FMTMSG_CCLASS:	/* -c class */
			sprintf(buffer, "----------------------------"
                                        "----------------------------\n"
                                        "EventIdx : %s\n"
                                        "Severity : %s\n"
                                        "Event class : %s\n\n"
                                        "Description : %s\n\n"
                                        "Response : %s\n\n"
                                        "Impact : %s\n\n"
                                        "Action : %s\n\n",
				fmd_msg_getid_evt(g_msg, arg_cclass),
				fmd_msg_getitem_evt(g_msg, arg_cclass, FMD_MSG_ITEM_SEVERITY),
				fmd_msg_getitem_evt(g_msg, arg_cclass, FMD_MSG_ITEM_CLASS),
                                fmd_msg_getitem_evt(g_msg, arg_cclass, FMD_MSG_ITEM_DESC),
                                fmd_msg_getitem_evt(g_msg, arg_cclass, FMD_MSG_ITEM_RESPONSE),
                                fmd_msg_getitem_evt(g_msg, arg_cclass, FMD_MSG_ITEM_IMPACT),
                                fmd_msg_getitem_evt(g_msg, arg_cclass, FMD_MSG_ITEM_ACTION));
                        printf("%s", buffer);
                        break;
		case FMTMSG_NCLASS:	/* -n class */
			printf("%s\n", fmd_msg_getid_evt(g_msg, arg_nclass));
                        break;
		default:
			break;
	}
}

int
main(int argc, char *argv[])
{
	int opt_i = 0, opt_c = 0, opt_n = 0;
	int c;

	memset(arg_ecode, 0, 256 * sizeof (char));
	memset(arg_cclass, 0, 256 * sizeof (char));
	memset(arg_nclass, 0, 256 * sizeof (char));

	while ((c =
	    getopt(argc, argv, "i:c:n:h?")) != EOF) {
		switch (c) {
			case 'i':
				opt_i++;
				strcpy(arg_ecode, optarg);
				break;
			case 'c':
				opt_c++;
				strcpy(arg_cclass, optarg);
				break;
			case 'n':
				opt_n++;
				strcpy(arg_nclass, optarg);
				break;
			case 'h':
			case '?':
				return (usage(stderr));
			default:
				break;
		}
	}

	if (argc == 1)
		return (usage(stderr));

	if (((opt_i == 1) && (opt_c == 1))
	  ||((opt_i == 1) && (opt_n == 1))
	  ||((opt_c == 1) && (opt_n == 1)))
		return (usage(stderr));

	if ((g_msg = fmd_msg_init(FMD_MSG_VERSION)) == NULL) {
		(void) fprintf(stderr, "fmtmsg: failed to initialize "
		    "libfmd_msg: %s\n", strerror(errno));
		return (FMTMSG_EXIT_FATAL);
	}

	if (opt_i)
		fmtmsg_printf(FMTMSG_ECODE);
	else if (opt_c)
		fmtmsg_printf(FMTMSG_CCLASS);
	else if (opt_n)
		fmtmsg_printf(FMTMSG_NCLASS);
	else
		return (usage(stderr));

	fmd_msg_fini(g_msg);

	return (FMTMSG_EXIT_SUCCESS);
}

