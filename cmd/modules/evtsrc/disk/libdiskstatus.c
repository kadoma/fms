/*
 * libdiskstatus.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 * 	Disk status library
 *
 * This library is responsible for querying health and other status information
 * from disk drives.  It is intended to be a generic interface, however only
 * SCSI (and therefore SATA) disks are currently supported.  The library is
 * capable of detecting the following status conditions:
 *
 * 	- Predictive failure
 * 	- Overtemp
 * 	- Self-test failure
 *	- SATA phy event
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "libdiskstatus.h"
#include "fm_ata.h"
#include "ds_ata.h"

static ds_transport_t *ds_transports[] = {
//	&ds_scsi_transport
	&ds_ata_transport
};

#define	NTRANSPORTS	(sizeof (ds_transports) / sizeof (ds_transports[0]))

/*
 * Open a handle to a disk.  This will fail if the device cannot be opened, or
 * if no suitable transport exists for communicating with the device.
 */
int
disk_status_open(disk_status_t *dsp, const char *path, int *error)
{
	ds_transport_t *t;
	int i;

	if ((dsp->ds_fd = open(path, O_RDWR)) < 0) {
		*error = EDS_CANT_OPEN;
		free(dsp);
		printf("Disk Module: DISK Open failed.\n");
		return (-1);
	}

	if ((dsp->ds_path = strdup(path)) == NULL) {
		*error = EDS_NOMEM;
		disk_status_close(dsp);
		return (-1);
	}

	for (i = 0; i < NTRANSPORTS; i++) {
		t = ds_transports[i];

		dsp->ds_transport = t;

		if ((dsp->ds_data = t->dt_open(dsp)) == NULL) {
			if (dsp->ds_error != EDS_NO_TRANSPORT) {
				*error = dsp->ds_error;
				disk_status_close(dsp);
				return (-1);
			}
		} else {
			dsp->ds_error = 0;
			break;
		}
	}

	if (dsp->ds_error == EDS_NO_TRANSPORT) {
		*error = dsp->ds_error;
		disk_status_close(dsp);
		return (-1);
	}

	return (0);
}

/*
 * Close a handle to a disk.
 */
void
disk_status_close(disk_status_t *dsp)
{
	if (dsp->ds_data)
		dsp->ds_transport->dt_close(dsp->ds_data);

	(void) close(dsp->ds_fd);
	free(dsp->ds_path);
	free(dsp);
}

void
disk_status_set_debug(bool value)
{
	ds_debug = value;
}

/*
 * Query basic information
 */
const char *
disk_status_path(disk_status_t *dsp)
{
	return (dsp->ds_path);
}

int
disk_status_errno(disk_status_t *dsp)
{
	return (dsp->ds_error);
}

int
disk_status_check(disk_status_t *dsp)
{
	int err;

	/*
	 * Scan (or rescan) the current device.
	 */
	nvlist_free(dsp->ds_scsi_overtemp);
	nvlist_free(dsp->ds_scsi_predfail);
	nvlist_free(dsp->ds_scsi_testfail);
	nvlist_free(dsp->ds_ata_overtemp);
	nvlist_free(dsp->ds_ata_error);
	nvlist_free(dsp->ds_ata_testfail);
	nvlist_free(dsp->ds_ata_sataphyevt);
	dsp->ds_scsi_testfail = dsp->ds_scsi_overtemp = dsp->ds_scsi_predfail = NULL;
	dsp->ds_ata_testfail  = dsp->ds_ata_overtemp  = dsp->ds_ata_error
		 	      = dsp->ds_ata_sataphyevt = NULL;

	/*
	 * Even if there is an I/O failure when trying to scan the device, we
	 * can still return the current state.
	 */
	if (dsp->ds_transport->dt_scan(dsp->ds_data) != 0 &&
	    dsp->ds_error != EDS_IO) {
		nvlist_free(dsp->ds_scsi_overtemp);
		nvlist_free(dsp->ds_scsi_predfail);
		nvlist_free(dsp->ds_scsi_testfail);
		nvlist_free(dsp->ds_ata_overtemp);
		nvlist_free(dsp->ds_ata_error);
		nvlist_free(dsp->ds_ata_testfail);
		nvlist_free(dsp->ds_ata_sataphyevt);
		dsp->ds_scsi_testfail = dsp->ds_scsi_overtemp = dsp->ds_scsi_predfail = NULL;
		dsp->ds_ata_testfail  = dsp->ds_ata_overtemp  = dsp->ds_ata_error
				      = dsp->ds_ata_sataphyevt = NULL;

		return (-1);
	}

	/*
	 * Construct the list of faults.
	 */
#if 0
	if (dsp->ds_scsi_predfail != NULL) {
		if ((err = nvlist_add_boolean_value(faults,
		    FM_EREPORT_SCSI_PREDFAIL,
		    (dsp->ds_faults & DS_FAULT_PREDFAIL) != 0)) != 0 ||
		    (err = nvlist_add_nvlist(nvl, FM_EREPORT_SCSI_PREDFAIL,
		    dsp->ds_scsi_predfail)) != 0)
			goto nverror;
	}

	if (dsp->ds_scsi_testfail != NULL) {
		if ((err = nvlist_add_boolean_value(faults,
		    FM_EREPORT_SCSI_TESTFAIL,
		    (dsp->ds_faults & DS_FAULT_TESTFAIL) != 0)) != 0 ||
		    (err = nvlist_add_nvlist(nvl, FM_EREPORT_SCSI_TESTFAIL,
		    dsp->ds_scsi_testfail)) != 0)
			goto nverror;
	}

	if (dsp->ds_scsi_overtemp != NULL) {
		if ((err = nvlist_add_boolean_value(faults,
		    FM_EREPORT_SCSI_OVERTEMP,
		    (dsp->ds_faults & DS_FAULT_OVERTEMP) != 0)) != 0 ||
		    (err = nvlist_add_nvlist(nvl, FM_EREPORT_SCSI_OVERTEMP,
		    dsp->ds_scsi_overtemp)) != 0)
			goto nverror;
	}
#endif

	if (dsp->ds_ata_error != NULL) {
		nvlist_add_nvlist(dsp->faults, dsp->ds_ata_error);
	}

	if (dsp->ds_ata_testfail != NULL) {
		nvlist_add_nvlist(dsp->faults, dsp->ds_ata_testfail);
	}

	if (dsp->ds_ata_overtemp != NULL) {
		nvlist_add_nvlist(dsp->faults, dsp->ds_ata_overtemp);
	}

	if (dsp->ds_ata_sataphyevt != NULL) {
		nvlist_add_nvlist(dsp->faults, dsp->ds_ata_sataphyevt);
	}

	return 0;
}
