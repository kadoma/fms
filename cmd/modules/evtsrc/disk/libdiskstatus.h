/*
 * libdiskstatus.h
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 *	libdiskstatus.h
 */

#ifndef	_LIBDISKSTATUS_H
#define	_LIBDISKSTATUS_H

#include <sys/types.h>
#include <nvpair.h>
#include <protocol.h>
#include "ds_impl.h"

/*
 * Basic library functions
 */
extern int
disk_status_open(disk_status_t *, const char *, int *);

extern void
disk_status_close(disk_status_t *);

extern const char *
disk_status_errmsg(int);

extern void
disk_status_set_debug(bool);

extern int
disk_status_errno(disk_status_t *);

/*
 * Miscellaneous functions.
 */
extern const char *
disk_status_path(disk_status_t *);

/*
 * Main entry point.
 */
extern int
disk_status_check(disk_status_t *);

/*
 * Utility function to simulate predictive failure (device-specific).
 */
extern int
disk_status_test_predfail(disk_status_t *);

#endif  /* _LIBDISKSTATUS_H */
