/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    fmadm.c
 * Author:      Inspur OS Team 
                wang.leibj@inspur.com
 * Date:        2015-08-21
 * Description: fault manager main
 *
 ************************************************************/

#include <string.h>
#include <strings.h>
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "fmadm.h"

static const char *g_pname;
static fmd_adm_t *g_adm;
static int g_quiet;

/*PRINTFLIKE1*/
void
note(const char *format, ...)
{
	va_list ap;

	if (g_quiet)
		return; /* suppress notices if -q specified */

	(void) fprintf(stdout, "%s: ", g_pname);
	va_start(ap, format);
	(void) vfprintf(stdout, format, ap);
	va_end(ap);
}

static void
vwarn(const char *format, va_list ap)
{
	int err = errno;

	(void) fprintf(stderr, "%s: ", g_pname);

	if (format != NULL)
		(void) vfprintf(stderr, format, ap);

	errno = err; /* restore errno for fmd_adm_errmsg() */

	if (format == NULL)
		(void) fprintf(stderr, "%s\n", fmd_adm_errmsg(g_adm));
	else if (strchr(format, '\n') == NULL)
		(void) fprintf(stderr, ": %s\n", fmd_adm_errmsg(g_adm));
}

/*PRINTFLIKE1*/
void
warn(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vwarn(format, ap);
	va_end(ap);
}

/*PRINTFLIKE1*/
void
die(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
//	vwarn(format, ap);
	va_end(ap);

	fmd_adm_close(g_adm);
//	printf("fmd_adm_close(g_adm)\n");
	exit(FMADM_EXIT_ERROR);
}

static const struct cmd {
	int (*cmd_func)(fmd_adm_t *, int, char *[]);
	const char *cmd_name;
	const char *cmd_usage;
	const char *cmd_desc;
} cmds[] = {
{ cmd_config, "config", "[-ls] [-i <interval>] [-b] [module]",
	"display or set fault manager configuration" },
{ cmd_modinfo, "modinfo", "[-l]", "display fault manager loaded module list" },
{ cmd_faulty, "faulty", "[-afgiprsv] [-u <uuid>] [-n <max_fault>]",
	"display list of faulty resources" },
//{ cmd_flush, "flush", "<fmri> ...", "flush cached state for resource" },
//{ cmd_gc, "gc", "<module>", NULL },
{ cmd_load, "load", "<path>", "load specified fault manager module" },
//{ cmd_repair, "repair", "<fmri>|label|<uuid>", NULL },
//{ cmd_repaired, "repaired", "<fmri>|label>",
//	"notify fault manager that resource has been repaired" },
//{ cmd_acquit, "acquit", "<fmri> [<uuid>] | label [<uuid>] | <uuid>",
//	"acquit resource or acquit case" },
//{ cmd_replaced, "replaced", "<fmri>|label",
//	"notify fault manager that resource has been replaced" },
//{ cmd_reset, "reset", "[-s serd] <module>", "reset module or sub-component" },
//{ cmd_rotate, "rotate", "<logname>", "rotate log file" },
{ cmd_unload, "unload", "<module>", "unload specified fault manager module" },
{ NULL, NULL, NULL }
};

static int
usage(FILE *fp)
{
	const struct cmd *cp;
	char buf[256];

	(void) fprintf(fp,
	    "Usage: %s [cmd [args ... ]]\n\n", g_pname);

	for (cp = cmds; cp->cmd_name != NULL; cp++) {
		if (cp->cmd_desc == NULL)
			continue;

		if (cp->cmd_usage != NULL) {
			(void) snprintf(buf, sizeof (buf), "%s %s %s",
			    g_pname, cp->cmd_name, cp->cmd_usage);
		} else {
			(void) snprintf(buf, sizeof (buf), "%s %s",
			    g_pname, cp->cmd_name);
		}
		(void) fprintf(fp, "\t%-30s - %s\n", buf, cp->cmd_desc);
	}

	return (FMADM_EXIT_USAGE);
}

int
main(int argc, char *argv[])
{
	const struct cmd *cp;
	const char *p;
	int  err;
	
	wr_log_logrotate(0);
        wr_log_init("./adm.log");
        wr_log_set_loglevel(WR_LOG_ERROR);

	if ((p = strrchr(argv[0], '/')) == NULL)
		g_pname = argv[0];
	else
		g_pname = p + 1;

	if (optind >= argc)
		return (usage(stdout));

	for (cp = cmds; cp->cmd_name != NULL; cp++) {
		if (strcmp(cp->cmd_name, argv[optind]) == 0)
			break;
	}

	if (cp->cmd_name == NULL) {
		wr_log("",WR_LOG_ERROR,"%s: illegal subcommand -- %s\n",g_pname,argv[optind]);
		return (usage(stderr));
	}

	if ((g_adm = fmd_adm_open()) == NULL)
		die(NULL); /* fmd_adm_errmsg() has enough info */

	argc -= optind;
	argv += optind;

	optind = 1; /* reset optind so subcommands can getopt() */

	err = cp->cmd_func(g_adm, argc, argv);
	fmd_adm_close(g_adm);
	return (err == FMADM_EXIT_USAGE ? usage(stderr) : err);
}

