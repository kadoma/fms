/*
 * disk_evtsrc.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 *	Disk error evtsrc module
 *
 * 	This evtsrc module is respon
 */

#include <sys/types.h>
#include <alloca.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>

#include <fmd.h>
#include <fmd_evtsrc.h>
#include <fmd_module.h>
#include <protocol.h>
#include <nvpair.h>
#include "libdiskstatus.h"

static fmd_t *fmdp;

/*
 * This enum definition is used to define a set of error tags associated with
 * the module api error conditions.  The shell script mkerror.sh is
 * used to parse this file and create a corresponding topo_error.c source file.
 * If you do something other than add a new error tag here, you may need to
 * update the mkerror shell script as it is based upon simple regexps.
 */
enum {
	EMOD_UNKNOWN = 2000,    /* unknown libtopo error */
	EMOD_NOMEM,             /* module memory limit exceeded */
	EMOD_PARTIAL_ENUM,      /* module completed partial enumeration */
	EMOD_METHOD_INVAL,      /* method arguments invalid */
	EMOD_METHOD_NOTSUP,     /* method not supported */
	EMOD_FMRI_NVL,          /* nvlist allocation failure for FMRI */
	EMOD_FMRI_VERSION,      /* invalid FMRI scheme version */
	EMOD_FMRI_MALFORM,      /* malformed FMRI */
	EMOD_NODE_BOUND,        /* node already bound */
	EMOD_NODE_DUP,          /* duplicate node */
	EMOD_NODE_NOENT,        /* node not found */
	EMOD_NODE_RANGE,        /* invalid node range */
	EMOD_VER_ABI,           /* registered with invalid ABI version */
	EMOD_VER_OLD,           /* attempt to load obsolete module */
	EMOD_VER_NEW,           /* attempt to load a newer module */
	EMOD_NVL_INVAL,         /* invalid nvlist */
	EMOD_NONCANON,          /* non-canonical component name requested */
	EMOD_MOD_NOENT,         /* module lookup failed */
	EMOD_UKNOWN_ENUM,       /* unknown enumeration error */
	EMOD_END                /* end of mod errno list (to ease auto-merge) */
};

void
disk_fm_event_post(nvlist_t *nvl, char *fullclass)
{
	nvl = nvlist_alloc();
	sprintf(nvl->name, FM_CLASS);
	strcpy(nvl->value, fullclass);
	nvl->rscid = topo_get_storageid(fmdp, "sda");
}

/*
 * Query the current disk status. If successful, the disk status is returned
 * as an nvlist consisting of at least the following members:
 *
 *      protocol        string          Supported protocol (currently "scsi")
 *
 *      status          nvlist          Arbitrary protocol-specific information
 *                                      about the current state of the disk.
 *
 *      faults          nvlist          A list of supported faults. Each
 *                                      element of this list is a boolean value.
 *                                      An element's existence indicates that
 *                                      the drive supports detecting this fault,
 *                                      and the value indicates the current
 *                                      state of the fault.
 *
 *      <fault-name>    nvlist          For each fault named in 'faults', a
 *                                      nvlist describing protocol-specific
 *                                      attributes of the fault.
 *
 * This method relies on the libdiskstatus library to query this information.
 */
int
disk_status(disk_status_t *dsp)
{
	char *fullpath = NULL;
	int ret = 0;
	int err = 0;

	fullpath = strdup("/dev/sda");

	if ((ret = disk_status_open(dsp, fullpath, &err)) != 0) {
		err == EDS_NOMEM ? EMOD_NOMEM : EMOD_METHOD_NOTSUP;
		return err;
	}

	if ((ret = disk_status_check(dsp)) != 0) {
		err = (disk_status_errno(dsp) == EDS_NOMEM ?
			EMOD_NOMEM : EMOD_METHOD_NOTSUP);
		return err;
	}

	return 0;
}

/*
 * Check a single topo node for failure.  This simply invokes the disk status
 * method, and generates any ereports as necessary.
 */
static int
dt_analyze_disk(disk_status_t *dsp)
{
	struct list_head *faults =
		(struct list_head *)malloc(sizeof(struct list_head));

	if (faults == NULL) {
		printf("Disk Module: failed to malloc\n");
		return (-1);
	}

	dsp->faults = faults;
	dsp->ds_scsi_overtemp = dsp->ds_scsi_predfail = NULL;
	dsp->ds_scsi_testfail = NULL;
	dsp->ds_ata_overtemp = dsp->ds_ata_error = NULL;
	dsp->ds_ata_testfail = dsp->ds_ata_sataphyevt = NULL;
	INIT_LIST_HEAD(dsp->faults);

	if (disk_status(dsp) != 0)
		return (-1);

	return (0);
}

/*
 * Periodic timeout.
 * Calling dt_analyze_disk().
  */
struct list_head *
disk_probe(evtsrc_module_t *emp)
{
	struct list_head *faults = NULL;
	fmdp = emp->module.mod_fmd;

	disk_status_t *dsp =
		(disk_status_t *)malloc(sizeof(disk_status_t));

	if (dsp == NULL) {
		printf("Disk Module: failed to malloc\n");
		return NULL;
	}
	memset(dsp, 0, sizeof(disk_status_t));

	if (dt_analyze_disk(dsp) != 0 ) {
		printf("Disk Module: failed to check disk\n");
		disk_status_close(dsp);
		return NULL;
	}

	faults = dsp->faults;
	disk_status_close(dsp);

	return faults;
}

static evtsrc_modops_t disk_mops = {
        .evt_probe = disk_probe,
};


fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
        return (fmd_module_t *)evtsrc_init(&disk_mops, path, pfmd);
}

void
fmd_fini(fmd_module_t *mp)
{
}

