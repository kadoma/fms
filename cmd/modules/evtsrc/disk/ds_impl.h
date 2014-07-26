/*
 * ds_impl.h
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 *	ds_impl.h
 */

#ifndef	_DS_IMPL_H
#define	_DS_IMPL_H

#include <dlfcn.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#include <nvpair.h>

/*
 * Error definitions
 */
#define EDS_BASE        2000

enum {
	EDS_NOMEM = EDS_BASE,		/* memory allocation failure */
	EDS_CANT_OPEN,			/* failed to open device */
	EDS_NO_TRANSPORT,		/* no supported transport */
	EDS_NOT_SUPPORTED,		/* status information not supported */
	EDS_NOT_SIMULATOR,		/* not a valid simulator file */
	EDS_IO				/* I/O error */
};

struct disk_status;

typedef struct ds_transport {
	void		*(*dt_open)(struct disk_status *);
	void		(*dt_close)(void *);
	int		(*dt_scan)(void *);
} ds_transport_t;


typedef struct disk_status {
	char			*ds_path;	/* path to device */
	int			ds_fd;		/* device file descriptor */
	ds_transport_t		*ds_transport;	/* associated transport */
	void			*ds_data;	/* transport-specific data */
	nvlist_t		*ds_scsi_overtemp;	/* overtemp */
	nvlist_t		*ds_scsi_predfail;	/* predict fail */
	nvlist_t		*ds_scsi_testfail;	/* self test fail */
	nvlist_t		*ds_ata_overtemp;      	/* overtemp */
	nvlist_t                *ds_ata_error;       	/* error */
	nvlist_t                *ds_ata_testfail;      	/* self test fail */
	nvlist_t                *ds_ata_sataphyevt;	/* sata phy event counters */
	int			ds_error;	/* last error */
	struct list_head	*faults;
} disk_status_t;

#define	DS_FAULT_OVERTEMP	0x1
#define	DS_FAULT_PREDFAIL	0x2
#define	DS_FAULT_TESTFAIL	0x4
#define DS_FAULT_ATA_OVERTEMP       	0x5
#define DS_FAULT_ATA_ERROR		0x6
//#define DS_FAULT_ATA_SMARTLOG		0x7
#define DS_FAULT_ATA_TESTFAIL		0x7
#define DS_FAULT_ATA_SATAPHY		0x8

extern void dsprintf(const char *, ...);
extern bool set_err(const char *, ...);
extern void ddump(const char *, const void *, size_t);
extern bool ds_debug;

extern int ds_set_errno(struct disk_status *, int);

extern int isbigendian();

// returns true if any of the n bytes are nonzero, else zero.
extern bool nonempty(const void *, int);

// convert time in msec to a text string
extern void MsecToText(unsigned int, char *);

// Used to warn users about invalid checksums. Called from atacmds.cpp.
extern void checksumwarning(const char *);

extern void syserror(const char *);

#endif	/* _DS_IMPL_H */
