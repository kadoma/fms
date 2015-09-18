/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    modinfo.c
 * Author:      Inspur OS Team 
                wang.leibj@inspur.com
 * Date:        2015-08-21
 * Description: display fault manager module status
 *
 ************************************************************/


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

int
cmd_modinfo(fmd_adm_t *adm, int argc, char *argv[])
{
	return 0;
}

#if 0

/*ARGSUSED*/
static int
config_modinfo(const fmd_adm_modinfo_t *ami, void *unused)
{
	const char *state;

	if (ami->ami_flags & FMD_ADM_MOD_FAILED)
		state = "failed";
	else
		state = "active";

	(void) printf("%-24s %-7s %-6s  %s\n",
	    ami->ami_name, ami->ami_vers, state, ami->ami_desc);

	return (0);
}

/*ARGSUSED*/
int
cmd_config(fmd_adm_t *adm, int argc, char *argv[])
{
	if (argc != 1)
		return (FMADM_EXIT_USAGE);

	(void) printf("%-24s %-7s %-6s  %s\n",
	    "MODULE", "VERSION", "STATUS", "DESCRIPTION");

	if (fmd_adm_module_iter(adm, config_modinfo, NULL) != 0)
		die("failed to retrieve configuration");

	return (FMADM_EXIT_SUCCESS);
}

int
cmd_rotate(fmd_adm_t *adm, int argc, char *argv[])
{
	if (argc != 2)
		return (FMADM_EXIT_USAGE);

	if (fmd_adm_log_rotate(adm, argv[1]) != 0)
		die("failed to rotate %s", argv[1]);

	note("%s has been rotated out and can now be archived\n", argv[1]);
	return (FMADM_EXIT_SUCCESS);
}

#endif
