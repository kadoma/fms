/*
 * ds_ata.h
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 *      ds_ata.h
 */

#ifndef	_DS_ATA_H
#define	_DS_ATA_H

#include <sys/types.h>
//#include <sys/byteorder.h>
//#include <sys/scsi/scsi.h>

#include "ds_impl.h"
//#include "mode.h"	//	SCSI Use!

#define PRId64 "lld"
#define PRIu64 "llu"
#define PRIx64 "llx"

typedef struct ds_ata_info {
        disk_status_t           *ai_dsp;
        void                    *ai_sim;
        int                     ai_supp_log;
        int                     ai_reftemp;
} ds_ata_info_t;

#define	INVALID_TEMPERATURE		0xff

/*
 * LOG page codes
 */
#define LOGPAGE_ATA_TEMP            0xe0
#define LOGPAGE_ATA_SELFTEST        0x07
#define LOGPAGE_ATA_ERROR           0x03
#define LOGPAGE_ATA_SATAPHY	    0x11

// Possible values for fix_firmwarebug.
enum {
	FIX_NOTSPECIFIED = 0,
  	FIX_NONE,
  	FIX_SAMSUNG,
  	FIX_SAMSUNG2,
  	FIX_SAMSUNG3
};

// Return codes (bitmask)

// command line did not parse, or internal error occured in smartctl
#define FAILCMD   (0x01<<0)

// device open failed
#define FAILDEV   (0x01<<1)

// device is in low power mode and -n option requests to exit
#define FAILPOWER (0x01<<1)

// read device identity (ATA only) failed
#define FAILID    (0x01<<1)

// smart command failed, or ATA identify device structure missing information
#define FAILSMART (0x01<<2)

// SMART STATUS returned FAILURE
#define FAILSTATUS (0x01<<3)

// Attributes found <= threshold with prefail=1
#define FAILATTR (0x01<<4)

// SMART STATUS returned GOOD but age attributes failed or prefail
// attributes have failed in the past
#define FAILAGE (0x01<<5)

// Device had Errors in the error log
#define FAILERR (0x01<<6)

// Device had Errors in the self-test log
#define FAILLOG (0x01<<7)

// Classes of SMART commands.  Here 'mandatory' means "Required by the
// ATA/ATAPI-5 Specification if the device implements the S.M.A.R.T.
// command set."  The 'mandatory' S.M.A.R.T.  commands are: (1)
// Enable/Disable Attribute Autosave, (2) Enable/Disable S.M.A.R.T.,
// and (3) S.M.A.R.T. Return Status.  All others are optional.
#define OPTIONAL_CMD 1
#define MANDATORY_CMD 2

extern ds_transport_t ds_ata_transport;

#endif   /* _DS_ATA_H */
