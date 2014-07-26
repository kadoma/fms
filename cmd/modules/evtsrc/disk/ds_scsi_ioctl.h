/*
 * ds_scsi_ioctl.h
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 *      ds_scsi_ioctl.h
 */

#ifndef	_DS_SCSI_IOCTL_H
#define	_DS_SCSI_IOCTL_H

#include "ds_scsi.h"

/*
 * "impossible" status value
 */
#define	IMPOSSIBLE_SCSI_STATUS	0xff

/*
 * defines for useful SCSI Status codes
 */
#define SCSI_STATUS_CHECK_CONDITION     0x2

/*
 * Minimum length of Request Sense data that we can accept
 */
#define	MIN_REQUEST_SENSE_LEN	18

/*
 * Rounded parameter, as returned in Extended Sense information
 */
#define	ROUNDED_PARAMETER	0x37

/* Following conditional defines just in case OS already has them defined.
 * If they are defined we hope they are defined correctly (for SCSI). */
#ifndef TEST_UNIT_READY
#define TEST_UNIT_READY 0x0
#endif
#ifndef LOG_SELECT
#define LOG_SELECT 0x4c
#endif
#ifndef LOG_SENSE
#define LOG_SENSE 0x4d
#endif
#ifndef MODE_SENSE
#define MODE_SENSE 0x1a
#endif
#ifndef MODE_SENSE_10
#define MODE_SENSE_10 0x5a
#endif
#ifndef MODE_SELECT
#define MODE_SELECT 0x15
#endif
#ifndef MODE_SELECT_10
#define MODE_SELECT_10 0x55
#endif
#ifndef INQUIRY
#define INQUIRY 0x12
#endif
#ifndef REQUEST_SENSE
#define REQUEST_SENSE  0x03
#endif
#ifndef RECEIVE_DIAGNOSTIC
#define RECEIVE_DIAGNOSTIC  0x1c
#endif
#ifndef SEND_DIAGNOSTIC
#define SEND_DIAGNOSTIC  0x1d
#endif
#ifndef READ_DEFECT_10
#define READ_DEFECT_10  0x37
#endif

#ifndef SAT_ATA_PASSTHROUGH_12
#define SAT_ATA_PASSTHROUGH_12 0xa1
#endif
#ifndef SAT_ATA_PASSTHROUGH_16
#define SAT_ATA_PASSTHROUGH_16 0x85
#endif

#define DXFER_NONE        0
#define DXFER_FROM_DEVICE 1
#define DXFER_TO_DEVICE   2

#ifndef SAT_ATA_PASSTHROUGH_12
#define SAT_ATA_PASSTHROUGH_12 0xa1
#endif
#ifndef SAT_ATA_PASSTHROUGH_16
#define SAT_ATA_PASSTHROUGH_16 0x85
#endif

/* defines for useful Sense Key codes */
#define SCSI_SK_NO_SENSE                0x0
#define SCSI_SK_RECOVERED_ERR           0x1
#define SCSI_SK_NOT_READY               0x2
#define SCSI_SK_MEDIUM_ERROR            0x3
#define SCSI_SK_HARDWARE_ERROR          0x4
#define SCSI_SK_ILLEGAL_REQUEST         0x5
#define SCSI_SK_UNIT_ATTENTION          0x6
#define SCSI_SK_ABORTED_COMMAND         0xb

/* defines for useful Additional Sense Codes (ASCs) */
#define SCSI_ASC_NOT_READY              0x4     /* more info in ASCQ code */
#define SCSI_ASC_NO_MEDIUM              0x3a    /* more info in ASCQ code */
#define SCSI_ASC_UNKNOWN_OPCODE         0x20
#define SCSI_ASC_UNKNOWN_FIELD          0x24
#define SCSI_ASC_UNKNOWN_PARAM          0x26
#define SCSI_ASC_WARNING                0xb
#define SCSI_ASC_IMPENDING_FAILURE      0x5d

#define SCSI_ASCQ_ATA_PASS_THROUGH      0x1d

/*               
 * definition for user-scsi command structure
 */
struct scsi_cmnd_io {
	int             scsi_flags;    /* read, write, etc. see below */
	int             scsi_dir;
	short           scsi_status;   /* resulting status  */
	short           scsi_timeout;  /* Command Timeout */
	uint8_t         *scsi_cdb;     /* cdb to send to target */
	uint8_t         *scsi_bufaddr; /* i/o source/destination */
	size_t          scsi_buflen;   /* size of i/o to take place */
	size_t          scsi_resid;    /* resid from i/o operation */
	size_t          scsi_cdblen;   /* # of valid cdb bytes */
	uchar_t         scsi_rqstatus; /* status of request sense cmd */
	uchar_t         scsi_rqresid;  /* resid of request sense cmd */
	size_t          resp_sense_len;
	size_t          max_sense_len;
	uint8_t         *scsi_rqbuf;   /* request sense buffer */
	ulong_t         scsi_path_instance; /* private: hardware path */
};

bool scsi_pass_through(int fd, struct scsi_cmnd_io *iop);

int do_scsi_mode_sense(int, int, int, caddr_t, int, scsi_ms_header_t *,
    void *, int, void *);
int do_scsi_mode_sense_10(int, int, int, caddr_t, int, scsi_ms_header_g1_t *,
    void *, int, void *);
int do_scsi_mode_select(int, int, int, caddr_t, int, scsi_ms_header_t *,
    void *, int, void *);
int do_scsi_mode_select_10(int, int, int, caddr_t, int, scsi_ms_header_g1_t *,
    void *, int, void *);
int do_scsi_log_sense(int, int, int, caddr_t, int, void *, int, void *);
int do_scsi_request_sense(int, caddr_t, int, void *, int, void *);

#endif	/* _DS_SCSI_IOCTL_H */
