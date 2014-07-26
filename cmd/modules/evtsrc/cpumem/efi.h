/*
 * efi.h
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 * 
 * Extensible Firmware Interface
 * Based on 'Extensible Firmware Interface Specification' version 0.9, April 30, 1999
 *
 * Copyright (C) 1999 VA Linux Systems
 * Copyright (C) 1999 Walt Drummond <drummond@valinux.com>
 * Copyright (C) 1999, 2002 Hewlett-Packard Co.
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 *	Stephane Eranian <eranian@hpl.hp.com>
 * 2003-11-05 Strip down to the bare minimum required to decode SAL records.
 *	      Keith Owens <kaos@sgi.com>
 */

#ifndef _EFI_H
#define _EFI_H

#include <stdio.h>
#include <string.h>
#include "mca_fm.h"

typedef struct {
	u8 b[16];
} efi_guid_t;

#define EFI_GUID(a,b,c,d0,d1,d2,d3,d4,d5,d6,d7) \
((efi_guid_t) \
{{ (a) & 0xff, ((a) >> 8) & 0xff, ((a) >> 16) & 0xff, ((a) >> 24) & 0xff, \
  (b) & 0xff, ((b) >> 8) & 0xff, \
  (c) & 0xff, ((c) >> 8) & 0xff, \
  (d0), (d1), (d2), (d3), (d4), (d5), (d6), (d7) }})

static inline int
efi_guidcmp (efi_guid_t left, efi_guid_t right)
{
	return memcmp(&left, &right, sizeof (efi_guid_t));
}

static inline char *
efi_guid_unparse(efi_guid_t *guid, char *out)
{
	sprintf(out, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		guid->b[3], guid->b[2], guid->b[1], guid->b[0],
		guid->b[5], guid->b[4], guid->b[7], guid->b[6],
		guid->b[8], guid->b[9], guid->b[10], guid->b[11],
		guid->b[12], guid->b[13], guid->b[14], guid->b[15]);
        return out;
}

#endif /* _EFI_H */
