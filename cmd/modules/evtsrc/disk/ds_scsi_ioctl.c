/*
 * ds_scsi_ioctl.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 *      This file contains the linux-specific IOCTL parts of
 *      smartmontools. It includes one interface routine for ATA devices,
 *      one for SCSI devices, and one for ATA devices behind escalade
 *      controllers.
 */

#include <assert.h>

#include <errno.h>
#include <fcntl.h>

#include <scsi/scsi.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/sg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/types.h>
#ifndef makedev // old versions of types.h do not include sysmacros.h
#include <sys/sysmacros.h>
#endif
#ifdef WITH_SELINUX
#include <selinux/selinux.h>
#endif

#include "sense.h"
//#include "ds_scsi.h"		//SCSI Need
#include "ds_scsi_ioctl.h"

//#include "int64.h"
//#include "atacmds.h"
//#include "extern.h"
//#include "os_linux.h"
//#include "scsicmds.h"
//#include "utility.h"
//#include "extern.h"
//#include "cciss.h"
//#include "megaraid.h"

//#include "dev_interface.h"
//#include "dev_ata_cmd_set.h"

#ifndef ENOTSUP
#define ENOTSUP ENOSYS
#endif

#define ARGUSED(x) ((void)(x))

typedef struct slist {
	char	*str;
	int	value;
} slist_t;

static char *
find_string(slist_t *slist, int match_value)
{
	for (; slist->str != NULL; slist++) {
		if (slist->value == match_value) {
			return (slist->str);
		}
	}

	return ((char *)NULL);
}

/*
 * Strings for printing mode sense page control values
 */
static slist_t page_control_strings[] = {
	{ "current",	PC_CURRENT },
	{ "changeable",	PC_CHANGEABLE },
	{ "default",	PC_DEFAULT },
	{ "saved",	PC_SAVED },
	{ NULL,		0 }
};

/*
 * Strings for printing the mode select options
 */
static slist_t mode_select_strings[] = {
	{ "",		0 },
	{ "(pf)",	MODE_SELECT_PF },
	{ "(sp)",	MODE_SELECT_SP },
	{ "(pf,sp)",	MODE_SELECT_PF|MODE_SELECT_SP },
	{ NULL,		0 }
};

#if 0
/*               
 * definition for user-scsi command structure
 */
struct scsi_cmnd_io {
	int             scsi_flags;    /* read, write, etc. see below */
	int		scsi_dir;
	short           scsi_status;   /* resulting status  */
	short           scsi_timeout;  /* Command Timeout */
	uint8_t         *scsi_cdb;     /* cdb to send to target */
	uint8_t         *scsi_bufaddr; /* i/o source/destination */
	size_t          scsi_buflen;   /* size of i/o to take place */
	size_t          scsi_resid;    /* resid from i/o operation */
	size_t          scsi_cdblen;   /* # of valid cdb bytes */
//	size_t          scsi_rqlen;    /* size of uscsi_rqbuf */
	uchar_t         scsi_rqstatus; /* status of request sense cmd */
	uchar_t         scsi_rqresid;  /* resid of request sense cmd */
	size_t		resp_sense_len;
	size_t          max_sense_len;
	uint8_t         *scsi_rqbuf;   /* request sense buffer */
	ulong_t         scsi_path_instance; /* private: hardware path */
};

#define DXFER_NONE        0
#define DXFER_FROM_DEVICE 1
#define DXFER_TO_DEVICE   2
#endif

/////////////////////////////////////////////////////////////////////////////

// >>>>>> Start of general SCSI specific linux code

/* Linux specific code.
 * Historically smartmontools (and smartsuite before it) used the
 * SCSI_IOCTL_SEND_COMMAND ioctl which is available to all linux device
 * nodes that use the SCSI subsystem. A better interface has been available
 * via the SCSI generic (sg) driver but this involves the extra step of
 * mapping disk devices (e.g. /dev/sda) to the corresponding sg device
 * (e.g. /dev/sg2). In the linux kernel 2.6 series most of the facilities of
 * the sg driver have become available via the SG_IO ioctl which is available
 * on all SCSI devices (on SCSI tape devices from lk 2.6.6).
 * So the strategy below is to find out if the SG_IO ioctl is available and
 * if so use it; failing that use the older SCSI_IOCTL_SEND_COMMAND ioctl.
 * Should work in 2.0, 2.2, 2.4 and 2.6 series linux kernels. */

#define MAX_DXFER_LEN 1024      /* can be increased if necessary */
#define SEND_IOCTL_RESP_SENSE_LEN 16    /* ioctl limitation */
#define SG_IO_RESP_SENSE_LEN 64 /* large enough see buffer */
#define LSCSI_DRIVER_MASK  0xf /* mask out "suggestions" */
#define LSCSI_DRIVER_SENSE  0x8 /* alternate CHECK CONDITION indication */
#define LSCSI_DID_ERROR 0x7 /* Need to work around aacraid driver quirk */
#define LSCSI_DRIVER_TIMEOUT  0x6
#define LSCSI_DID_TIME_OUT  0x3
#define LSCSI_DID_BUS_BUSY  0x2
#define LSCSI_DID_NO_CONNECT  0x1

#ifndef SCSI_IOCTL_SEND_COMMAND
#define SCSI_IOCTL_SEND_COMMAND 1
#endif

#define SG_IO_PRESENT_UNKNOWN 0
#define SG_IO_PRESENT_YES 1
#define SG_IO_PRESENT_NO 2

static int sg_io_cmnd_io(int dev_fd, struct scsi_cmnd_io *iop, int report,
			 int unknown);
//static int sisc_cmnd_io(int dev_fd, struct scsi_cmnd_io *iop, int report);

static int sg_io_state = SG_IO_PRESENT_UNKNOWN;


/* output binary in hex and optionally ascii */
void
dStrHex(const char* str, int len, int no_ascii)
{
	const char* p = str;
	unsigned char c;
	char buff[82];
	int a = 0;
	const int bpstart = 5;
	const int cpstart = 60;
    	int cpos = cpstart;
    	int bpos = bpstart;
    	int i, k;

    	if (len <= 0) return;
    	memset(buff,' ',80);
    	buff[80]='\0';
    	k = sprintf(buff + 1, "%.2x", a);
    	buff[k + 1] = ' ';
    	if (bpos >= ((bpstart + (9 * 3))))
        	bpos++;

    	for(i = 0; i < len; i++) {
        	c = *p++;
        	bpos += 3;
        	if (bpos == (bpstart + (9 * 3)))
            		bpos++;
        	sprintf(&buff[bpos], "%.2x", (int)(unsigned char)c);
        	buff[bpos + 2] = ' ';
        	if (no_ascii)
            		buff[cpos++] = ' ';
        	else {
            		if ((c < ' ') || (c >= 0x7f))
                		c='.';
            		buff[cpos++] = c;
        	}
        	if (cpos > (cpstart+15)) {
			dsprintf("%s\n", buff);
            		bpos = bpstart;
            		cpos = cpstart;
            		a += 16;
            		memset(buff,' ',80);
            		k = sprintf(buff + 1, "%.2x", a);
            		buff[k + 1] = ' ';
        	}
    	}
    	if (cpos > cpstart) {
        	dsprintf("%s\n", buff);
    	}
}

struct scsi_opcode_name {
	uint8_t opcode;
	const char * name;
};

static struct scsi_opcode_name opcode_name_arr[] = {
	/* in ascending opcode order */
	{TEST_UNIT_READY, "test unit ready"},       /* 0x00 */
	{REQUEST_SENSE, "request sense"},           /* 0x03 */
	{INQUIRY, "inquiry"},                       /* 0x12 */
	{MODE_SELECT, "mode select(6)"},            /* 0x15 */
	{MODE_SENSE, "mode sense(6)"},              /* 0x1a */
	{RECEIVE_DIAGNOSTIC, "receive diagnostic"}, /* 0x1c */
	{SEND_DIAGNOSTIC, "send diagnostic"},       /* 0x1d */
	{READ_DEFECT_10, "read defect list(10)"},   /* 0x37 */
	{LOG_SELECT, "log select"},                 /* 0x4c */
	{LOG_SENSE, "log sense"},                   /* 0x4d */
    	{MODE_SELECT_10, "mode select(10)"},        /* 0x55 */
    	{MODE_SENSE_10, "mode sense(10)"},          /* 0x5a */
    	{SAT_ATA_PASSTHROUGH_16, "ata pass-through(16)"}, /* 0x85 */
    	{SAT_ATA_PASSTHROUGH_12, "ata pass-through(12)"}, /* 0xa1 */
};

const char *scsi_get_opcode_name(uint8_t opcode)
{
	int k;
    	int len = sizeof(opcode_name_arr) / sizeof(opcode_name_arr[0]);
    	struct scsi_opcode_name * onp;

    	for (k = 0; k < len; ++k) {
        	onp = &opcode_name_arr[k];
        	if (opcode == onp->opcode)
            		return onp->name;
        	else if (opcode < onp->opcode)
            		return NULL;
    	}
    	return NULL;
}

#if 0
struct linux_ioctl_send_command
{
    int inbufsize;
    int outbufsize;
    UINT8 buff[MAX_DXFER_LEN + 16];
};

/* The Linux SCSI_IOCTL_SEND_COMMAND ioctl is primitive and it doesn't
 * support: CDB length (guesses it from opcode), resid and timeout.
 * Patches in Linux 2.4.21 and 2.5.70 to extend SEND DIAGNOSTIC timeout
 * to 2 hours in order to allow long foreground extended self tests. */
static int sisc_cmnd_io(int dev_fd, struct scsi_cmnd_io * iop, int report)
{
    struct linux_ioctl_send_command wrk;
    int status, buff_offset;
    size_t len;

    memcpy(wrk.buff, iop->cmnd, iop->cmnd_len);
    buff_offset = iop->cmnd_len;
    if (report > 0) {
        int k, j;
        const unsigned char * ucp = iop->cmnd;
        const char * np;
        char buff[256];
        const int sz = (int)sizeof(buff);

        np = scsi_get_opcode_name(ucp[0]);
        j = snprintf(buff, sz, " [%s: ", np ? np : "<unknown opcode>");
        for (k = 0; k < (int)iop->cmnd_len; ++k)
            j += snprintf(&buff[j], (sz > j ? (sz - j) : 0), "%02x ", ucp[k]);
        if ((report > 1) &&
            (DXFER_TO_DEVICE == iop->dxfer_dir) && (iop->dxferp)) {
            int trunc = (iop->dxfer_len > 256) ? 1 : 0;

            j += snprintf(&buff[j], (sz > j ? (sz - j) : 0), "]\n  Outgoing "
                          "data, len=%d%s:\n", (int)iop->dxfer_len,
                          (trunc ? " [only first 256 bytes shown]" : ""));
            dStrHex((const char *)iop->dxferp,
                    (trunc ? 256 : iop->dxfer_len) , 1);
        }
        else
            j += snprintf(&buff[j], (sz > j ? (sz - j) : 0), "]\n");
        pout("%s", buff);
    }
    switch (iop->dxfer_dir) {
        case DXFER_NONE:
            wrk.inbufsize = 0;
            wrk.outbufsize = 0;
            break;
        case DXFER_FROM_DEVICE:
            wrk.inbufsize = 0;
            if (iop->dxfer_len > MAX_DXFER_LEN)
                return -EINVAL;
            wrk.outbufsize = iop->dxfer_len;
            break;
        case DXFER_TO_DEVICE:
            if (iop->dxfer_len > MAX_DXFER_LEN)
                return -EINVAL;
            memcpy(wrk.buff + buff_offset, iop->dxferp, iop->dxfer_len);
            wrk.inbufsize = iop->dxfer_len;
            wrk.outbufsize = 0;
            break;
        default:
            pout("do_scsi_cmnd_io: bad dxfer_dir\n");
            return -EINVAL;
    }
    iop->resp_sense_len = 0;
    iop->scsi_status = 0;
    iop->resid = 0;
    status = ioctl(dev_fd, SCSI_IOCTL_SEND_COMMAND, &wrk);
    if (-1 == status) {
        if (report)
            pout("  SCSI_IOCTL_SEND_COMMAND ioctl failed, errno=%d [%s]\n",
                 errno, strerror(errno));
        return -errno;
    }
    if (0 == status) {
        if (report > 0)
            pout("  status=0\n");
        if (DXFER_FROM_DEVICE == iop->dxfer_dir) {
            memcpy(iop->dxferp, wrk.buff, iop->dxfer_len);
            if (report > 1) {
                int trunc = (iop->dxfer_len > 256) ? 1 : 0;

                pout("  Incoming data, len=%d%s:\n", (int)iop->dxfer_len,
                     (trunc ? " [only first 256 bytes shown]" : ""));
                dStrHex((const char*)iop->dxferp,
                        (trunc ? 256 : iop->dxfer_len) , 1);
            }
        }
        return 0;
    }
    iop->scsi_status = status & 0x7e; /* bits 0 and 7 used to be for vendors */
    if (LSCSI_DRIVER_SENSE == ((status >> 24) & 0xf))
        iop->scsi_status = SCSI_STATUS_CHECK_CONDITION;
    len = (SEND_IOCTL_RESP_SENSE_LEN < iop->max_sense_len) ?
                SEND_IOCTL_RESP_SENSE_LEN : iop->max_sense_len;
    if ((SCSI_STATUS_CHECK_CONDITION == iop->scsi_status) &&
        iop->sensep && (len > 0)) {
        memcpy(iop->sensep, wrk.buff, len);
        iop->resp_sense_len = len;
        if (report > 1) {
            pout("  >>> Sense buffer, len=%d:\n", (int)len);
            dStrHex((const char *)wrk.buff, len , 1);
        }
    }
    if (report) {
        if (SCSI_STATUS_CHECK_CONDITION == iop->scsi_status) {
            pout("  status=%x: sense_key=%x asc=%x ascq=%x\n", status & 0xff,
                 wrk.buff[2] & 0xf, wrk.buff[12], wrk.buff[13]);
        }
        else
            pout("  status=0x%x\n", status);
    }
    if (iop->scsi_status > 0)
        return 0;
    else {
        if (report > 0)
            pout("  ioctl status=0x%x but scsi status=0, fail with EIO\n",
                 status);
        return -EIO;      /* give up, assume no device there */
    }
}

#endif
/////////////////////////////////////////////////////////////////////////////

/*
 * Execute a command and determine the result.  Uses the "uscsi" ioctl
 * interface, which is fully supported.
 *
 * If the user wants request sense data to be returned in case of error then ,
 * the "uscsi_cmd" structure should have the request sense buffer allocated in
 * uscsi_rqbuf.
 */
//static int
//uscsi_cmd(int fd, struct uscsi_cmd *ucmd, void *rqbuf, int *rqlen)


/*
 * Preferred implementation for issuing SCSI commands in linux. This
 * function uses the SG_IO ioctl. Return 0 if command issued successfully
 * (various status values should still be checked). If the SCSI command
 * cannot be issued then a negative errno value is returned.
 */
static int
sg_io_cmnd_io(int dev_fd, struct scsi_cmnd_io * iop, int report,
	      int unknown)
{
	struct sg_io_hdr io_hdr;

    	if (report > 0) {
        	int k, j;
        	const unsigned char * ucp = iop->scsi_cdb;
        	const char * np;
        	char buff[256];
        	const int sz = (int)sizeof(buff);

        	np = scsi_get_opcode_name(ucp[0]);
        	j = snprintf(buff, sz, " [%s: ", np ? np : "<unknown opcode>");
        	for (k = 0; k < (int)iop->scsi_cdblen; ++k)
            		j += snprintf(&buff[j], (sz > j ? (sz - j) : 0), "%02x ", ucp[k]);
        	if ((report > 1) &&
            		(DXFER_TO_DEVICE == iop->scsi_dir) && (iop->scsi_bufaddr)) {
            		int trunc = (iop->scsi_buflen > 256) ? 1 : 0;

            		j += snprintf(&buff[j], (sz > j ? (sz - j) : 0), "]\n  Outgoing "
                        	"data, len=%d%s:\n", (int)iop->scsi_buflen,
                          	(trunc ? " [only first 256 bytes shown]" : ""));
            		dStrHex((const char *)iop->scsi_bufaddr,
                    		(trunc ? 256 : iop->scsi_buflen) , 1);
        	} else
            		j += snprintf(&buff[j], (sz > j ? (sz - j) : 0), "]\n");
        	dsprintf("%s", buff);
    	}
    	memset(&io_hdr, 0, sizeof(struct sg_io_hdr));
    	io_hdr.interface_id = 'S';
    	io_hdr.cmd_len = iop->scsi_cdblen;
    	io_hdr.mx_sb_len = iop->max_sense_len;
    	io_hdr.dxfer_len = iop->scsi_buflen;
    	io_hdr.dxferp = iop->scsi_bufaddr;
    	io_hdr.cmdp = iop->scsi_cdb;
    	io_hdr.sbp = iop->scsi_rqbuf;
    	/* sg_io_hdr interface timeout has millisecond units. Timeout of 0
       	   defaults to 60 seconds. */
    	io_hdr.timeout = ((0 == iop->scsi_timeout) ? 60 : iop->scsi_timeout) * 1000;
    	switch (iop->scsi_dir) {
        	case DXFER_NONE:
            		io_hdr.dxfer_direction = SG_DXFER_NONE;
            		break;
        	case DXFER_FROM_DEVICE:
            		io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
            		break;
        	case DXFER_TO_DEVICE:
            		io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
            		break;
        	default:
            		printf("do_scsi_cmnd_io: bad dxfer_dir\n");
            	return -EINVAL;
    	}
    	iop->resp_sense_len = 0;
    	iop->scsi_status = 0;
    	iop->scsi_rqresid = 0;
    	if (ioctl(dev_fd, SG_IO, &io_hdr) < 0) {
        	if (report && (! unknown))
            		printf("  SG_IO ioctl failed, errno=%d [%s]\n", errno,
                 		strerror(errno));
        	return -errno;
    	}
    	iop->scsi_rqresid = io_hdr.resid;
    	iop->scsi_status = io_hdr.status;
    	if (report > 0) {
        	dsprintf("  scsi_status=0x%x, host_status=0x%x, driver_status=0x%x\n"
             	     "  info=0x%x  duration=%d milliseconds  resid=%d\n", io_hdr.status,
             		io_hdr.host_status, io_hdr.driver_status, io_hdr.info,
             		io_hdr.duration, io_hdr.resid);
        	if (report > 1) {
            		if (DXFER_FROM_DEVICE == iop->scsi_dir) {
                		int trunc, len;

				len = iop->scsi_buflen - iop->scsi_rqresid;
				trunc = (len > 256) ? 1 : 0;
                		if (len > 0) {
                    			dsprintf("  Incoming data, len=%d%s:\n", len,
                         			(trunc ? " [only first 256 bytes shown]" : ""));
                    			dStrHex((const char*)iop->scsi_bufaddr, (trunc ? 256 : len), 1);
                		} else
                    			dsprintf("  Incoming data trimmed to nothing by resid\n");
            		}
        	}
    	}

    	if (io_hdr.info | SG_INFO_CHECK) { /* error or warning */
        	int masked_driver_status = (LSCSI_DRIVER_MASK & io_hdr.driver_status);

        	if (0 != io_hdr.host_status) {
        		if ((LSCSI_DID_NO_CONNECT == io_hdr.host_status) ||
                    	    (LSCSI_DID_BUS_BUSY == io_hdr.host_status) ||
                            (LSCSI_DID_TIME_OUT == io_hdr.host_status))
                		return -ETIMEDOUT;
            		else
               			/* Check for DID_ERROR - workaround for aacraid driver quirk */
               		if (LSCSI_DID_ERROR != io_hdr.host_status) {
                		return -EIO; /* catch all if not DID_ERR */
               		}
        	}
        	if (0 != masked_driver_status) {
            		if (LSCSI_DRIVER_TIMEOUT == masked_driver_status)
                		return -ETIMEDOUT;
            		else if (LSCSI_DRIVER_SENSE != masked_driver_status)
                		return -EIO;
        	}
        	if (LSCSI_DRIVER_SENSE == masked_driver_status)
            		iop->scsi_status = SCSI_STATUS_CHECK_CONDITION;
        	iop->resp_sense_len = io_hdr.sb_len_wr;
        	if ((SCSI_STATUS_CHECK_CONDITION == iop->scsi_status) &&
            		iop->scsi_rqbuf && (iop->resp_sense_len > 0)) {
            			if (report > 1) {
                			dsprintf("  >>> Sense buffer, len=%d:\n",
                     				(int)iop->resp_sense_len);
                			dStrHex((const char *)iop->scsi_rqbuf, iop->resp_sense_len , 1);
            			}
        	}
        	if (report) {
            		if (SCSI_STATUS_CHECK_CONDITION == iop->scsi_status) {
                		if ((iop->scsi_rqbuf[0] & 0x7f) > 0x71)
                    			dsprintf("  status=%x: [desc] sense_key=%x asc=%x ascq=%x\n",
                        			iop->scsi_status, iop->scsi_rqbuf[1] & 0xf,
                         			iop->scsi_rqbuf[2], iop->scsi_rqbuf[3]);
                		else
                    			dsprintf("  status=%x: sense_key=%x asc=%x ascq=%x\n",
                         			iop->scsi_status, iop->scsi_rqbuf[2] & 0xf,
                         			iop->scsi_rqbuf[12], iop->scsi_rqbuf[13]);
            		} else
                		dsprintf("  status=0x%x\n", iop->scsi_status);
        	}
    	}
    	return 0;
}



#if 0
struct scsi_extended_sense *rq;
	int status;

	/*
	 * Set function flags for driver.
	 */
//	iop->scsi_flags = SCSI_ISOLATE;
//	if (!ds_debug)
//		iop->scsi_flags |= SCSI_SILENT;

#if 0
	/*
	 * If this command will perform a read, set the SCSI_READ flag
	 */
	if (iop->scsi_buflen > 0) {
		/*
		 * scsi_cdb is declared as a caddr_t, so any CDB
		 * command byte with the MSB set will result in a
		 * compiler error unless we cast to an unsigned value.
		 */
		switch ((uint8_t)iop->scsi_cdb[0]) {
		case SCMD_MODE_SENSE:
		case SCMD_MODE_SENSE_G1:
		case SCMD_LOG_SENSE_G1:
		case SCMD_REQUEST_SENSE:
			iop->scsi_flags |= SCSI_READ;
			break;

		case SCMD_MODE_SELECT:
		case SCMD_MODE_SELECT_G1:
			/* LINTED */
			iop->scsi_flags |= SCSI_WRITE;
			break;
		default:
			assert(0);
			break;
		}
	}
#endif

	/* Set timeout */
//	iop->scsi_timeout = scsi_timeout();
	iop->scsi_timeout = 20;			//SCSI_TIMEOUT_DEFAULT = 20

	/* Set driection */
//	iop->dxfer_dir = DXFER_FROM_DEVICE;
	iop->scsi_dir = DXFER_FROM_DEVICE;

	/*
	 * Set up Request Sense buffer
	 */

	if (iop->scsi_rqbuf == NULL) {
		iop->scsi_rqbuf = rqbuf;
		iop->scsi_rqlen = *rqblen;
		iop->scsi_rqresid = *rqblen;
	}
//	if (iop->scsi_rqbuf)
//		iop->scsi_flags |= SCSI_RQENABLE;
	iop->scsi_rqstatus = IMPOSSIBLE_SCSI_STATUS;

	if (iop->scsi_rqbuf != NULL && iop->scsi_rqlen > 0)
		(void) memset(iop->scsi_rqbuf, 0, iop->scsi_rqlen);

////////////////////////////////////////////
	/*
	 * Execute the ioctl
	 */
//	status = ioctl(fd, USCSICMD, ucmd);
//	if (status == 0 && iop->scsi_status == 0)
//		return (status);
////////////////////////////////////////////
	struct sg_io_hdr io_hdr;

	if (report > 0) {
		int k, j;
		const unsigned char * ucp = iop->scsi_cdb;
		const char * np;
        	char buff[256];
        	const int sz = (int)sizeof(buff);

        	np = scsi_get_opcode_name(ucp[0]);
        	j = snprintf(buff, sz, " [%s: ", np ? np : "<unknown opcode>");
        	for (k = 0; k < (int)iop->scsi_cdblen; ++k)
        		j += snprintf(&buff[j], (sz > j ? (sz - j) : 0), "%02x ", ucp[k]);
        	if ((report > 1) &&
        		(DXFER_TO_DEVICE == iop->scsi_dir) && (iop->scsi_rqbuf)) {
        		int trunc = (iop->scsi_rqlen > 256) ? 1 : 0;

        		j += snprintf(&buff[j], (sz > j ? (sz - j) : 0), "]\n  Outgoing "
                		"data, len=%d%s:\n", (int)iop->scsi_buflen,
                        	(trunc ? " [only first 256 bytes shown]" : ""));
            		dStrHex((const char *)iop->scsi_rqbuf,
                		(trunc ? 256 : iop->scsi_rqlen) , 1);
        	} else
        		j += snprintf(&buff[j], (sz > j ? (sz - j) : 0), "]\n");
        	printf("%s", buff);
    	}
	memset(&io_hdr, 0, sizeof(struct sg_io_hdr));
	io_hdr.interface_id = 'S';
//	io_hdr.cmd_len = iop->cmnd_len;
	io_hdr.cmd_len = iop->scsi_cdblen;
//	io_hdr.mx_sb_len = iop->max_sense_len;
//////	io_hdr.mx_sb_len = iop->scsi_rqlen;
	io_hdr.mx_sb_len = iop->scsi_buflen;
//	io_hdr.dxfer_len = iop->dxfer_len;
//////	io_hdr.dxfer_len = iop->scsi_buflen;
	io_hdr.dxfer_len = iop->scsi_rqlen;
//	io_hdr.dxferp = iop->dxferp;
//////	io_hdr.dxferp = iop->scsi_bufaddr;
	io_hdr.dxferp = iop->scsi_rqbuf;
//	io_hdr.cmdp = iop->cmnd;
	io_hdr.cmdp = iop->scsi_cdb;
//	io_hdr.sbp = iop->sensep;
//////	io_hdr.sbp = iop->scsi_rqbuf;
	io_hdr.sbp = iop->scsi_bufaddr;

	/*
	 * sg_io_hdr interface timeout has millisecond units. Timeout of 0
	 * defaults to 60 seconds.
	 */
	io_hdr.timeout = ((0 == iop->scsi_timeout) ? 60 : iop->scsi_timeout) *1000;
	switch (iop->scsi_dir) {
	case DXFER_NONE:
		io_hdr.dxfer_direction = SG_DXFER_NONE;
		break;
	case DXFER_FROM_DEVICE:
		io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
		break;
	case DXFER_TO_DEVICE:
		io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
		break;
	default:
		printf("do_scsi_cmnd_io: bad dxfer_dir\n");
		return -EINVAL;
	}
	iop->resp_sense_len = 0;
	iop->scsi_status = 0;
//	iop->resid = 0;
/////	iop->scsi_resid = 0;
/////////////////////////////////////////////////////////
	/*
	 * Execute the ioctl
	 */
	if (ioctl(dev_fd, SG_IO, &io_hdr) < 0) {
		return -1;
	}
//	iop->resid = io_hdr.resid;
/////	iop->scsi_resid = io_hdr.resid;
	iop->scsi_rqresid = io_hdr.resid;
	iop->scsi_status = io_hdr.status;
	if (report > 0) {
		printf("  scsi_status=0x%x, host_status=0x%x, driver_status=0x%x\n"
			"  info=0x%x  duration=%d milliseconds  resid=%d\n", io_hdr.status,
			io_hdr.host_status, io_hdr.driver_status, io_hdr.info,
			io_hdr.duration, io_hdr.resid);
		if (report > 1) {
			if (DXFER_FROM_DEVICE == iop->scsi_dir) {
				int trunc, len;

//				len = iop->dxfer_len - iop->resid;
				len = iop->scsi_rqlen - iop->scsi_rqresid;
				trunc = (len > 256) ? 1 : 0;
				if (len > 0) {
					printf("  Incoming data, len=%d%s:\n", len,
						(trunc ? " [only first 256 bytes shown]" : ""));
					dStrHex((const char*)iop->scsi_rqbuf, (trunc ? 256 : len), 1);
				} else
					printf("  Incoming data trimmed to nothing by resid\n");
			}
		}
	}

	if (io_hdr.info | SG_INFO_CHECK) { /* error or warning */
		int masked_driver_status = (LSCSI_DRIVER_MASK & io_hdr.driver_status);

		if (0 != io_hdr.host_status) {
			if ((LSCSI_DID_NO_CONNECT == io_hdr.host_status) ||
				(LSCSI_DID_BUS_BUSY == io_hdr.host_status) ||
				(LSCSI_DID_TIME_OUT == io_hdr.host_status))
				return -ETIMEDOUT;
			else
			/* Check for DID_ERROR - workaround for aacraid driver quirk */
			if (LSCSI_DID_ERROR != io_hdr.host_status) {
				return -EIO; /* catch all if not DID_ERR */
			}
		}
		if (0 != masked_driver_status) {
			if (LSCSI_DRIVER_TIMEOUT == masked_driver_status)
				return -ETIMEDOUT;
			else if (LSCSI_DRIVER_SENSE != masked_driver_status)
				return -EIO;
		}
		if (LSCSI_DRIVER_SENSE == masked_driver_status)
			iop->scsi_status = SCSI_STATUS_CHECK_CONDITION;
		iop->resp_sense_len = io_hdr.sb_len_wr;
		if ((SCSI_STATUS_CHECK_CONDITION == iop->scsi_status) &&
//			iop->sensep && (iop->resp_sense_len > 0)) {
			iop->scsi_bufaddr && (iop->resp_sense_len > 0)) {
			if (report > 1) {
				printf("  >>> Sense buffer, len=%d:\n",
					(int)iop->resp_sense_len);
				dStrHex((const char *)iop->scsi_bufaddr, iop->resp_sense_len , 1);
			}
		}
		if (report) {
			if (SCSI_STATUS_CHECK_CONDITION == iop->scsi_status) {
//				if ((iop->sensep[0] & 0x7f) > 0x71)
				if ((iop->scsi_bufaddr[0] & 0x7f) > 0x71)
					printf("  status=%x: [desc] sense_key=%x asc=%x ascq=%x\n",
						iop->scsi_status, iop->scsi_bufaddr[1] & 0xf,
						iop->scsi_bufaddr[2], iop->scsi_bufaddr[3]);
				else
					printf("  status=%x: sense_key=%x asc=%x ascq=%x\n",
						iop->scsi_status, iop->scsi_bufaddr[2] & 0xf,
						iop->scsi_bufaddr[12], iop->scsi_bufaddr[13]);
			} else
				printf("  status=0x%x\n", iop->scsi_status);
		}
	}

//////////////////////////////////////////////////////////////
//	scsi_printerr(iop, rq, *rqblen);

//////////////////////////////////////////////////////////////
	return 0;
}

#if 0

////////////////////////////////////////////
#if 0
	/*
	 * If an automatic Request Sense gave us valid info about the error, we
	 * may be able to use that to print a reasonable error msg.
	 */
	if (iop->scsi_rqstatus == IMPOSSIBLE_SCSI_STATUS) {
		printf("No request sense for command %s\n",
			find_string(scsi_cmdname_strings,
			iop->scsi_cdb[0]));
		return (-1);
	}
	if (iop->scsi_rqstatus != STATUS_GOOD) {
		printf("Request sense status for command %s: 0x%x\n",
			find_string(scsi_cmdname_strings,
			iop->scsi_cdb[0]),
			iop->scsi_rqstatus);
		return (-1);
	}
#endif
	rq = (struct scsi_extended_sense *)iop->scsi_rqbuf;
	*rqblen = iop->scsi_rqlen - iop->scsi_rqresid;
#if 0
	if ((((int)rq->es_add_len) + 8) < MIN_REQUEST_SENSE_LEN ||
		rq->es_class != CLASS_EXTENDED_SENSE ||
		*rqblen < MIN_REQUEST_SENSE_LEN) {
//		printf("Request sense for command %s failed\n",
//			find_string(scsi_cmdname_strings,
//			iop->scsi_cdb[0]));
		printf("Request sense for command /**/ failed\n");

		printf("Sense data:\n");
		ddump(NULL, (caddr_t)rqbuf, *rqblen);

		return (-1);
	}
#endif
	/*
	 * If the failed command is a Mode Select, and the
	 * target is indicating that it has rounded one of
	 * the mode select parameters, as defined in the SCSI-2
	 * specification, then we should accept the command
	 * as successful.
	 */
	if (iop->scsi_cdb[0] == MODE_SELECT ||
		iop->scsi_cdb[0] == MODE_SELECT_10) {
		if (rq->es_key == KEY_RECOVERABLE_ERROR &&
			rq->es_add_code == ROUNDED_PARAMETER &&
			rq->es_qual_code == 0) {
			return (0);
		}
	}

//	if (ds_debug)
//		scsi_printerr(iop, rq, *rqblen);
	printf("scsi_printerr()\n");
	if (rq->es_key != KEY_RECOVERABLE_ERROR)
		return (-1);

	return (0);
}

#endif

#endif

/*
 * SCSI command transmission interface function, linux version.
 * Returns 0 if SCSI command successfully launched and response
 * received. Even when 0 is returned the caller should check
 * scsi_cmnd_io::scsi_status for SCSI defined errors and warnings
 * (e.g. CHECK CONDITION). If the SCSI command could not be issued
 * (e.g. device not present or timeout) or some other problem
 * (e.g. timeout) then returns a negative errno value
 */
static int
do_normal_scsi_cmnd_io(int dev_fd, struct scsi_cmnd_io *iop, int report)
{
	int res;

	/*
	 * implementation relies on static sg_io_state variable. If not
	 * previously set tries the SG_IO ioctl. If that succeeds assume
	 * that SG_IO ioctl functional. If it fails with an errno value
	 * other than ENODEV (no device) or permission then assume
	 * SCSI_IOCTL_SEND_COMMAND is the only option.
	 */


        switch (sg_io_state) {
	case SG_IO_PRESENT_UNKNOWN:
		/* ignore report argument */
		if (0 == (res = sg_io_cmnd_io(dev_fd, iop, report, 1))) {
			sg_io_state = SG_IO_PRESENT_YES;
			return 0;
		} else if ((-ENODEV == res) || (-EACCES == res) || (-EPERM == res))
			return res;         /* wait until we see a device */
		sg_io_state = SG_IO_PRESENT_NO;
		/* drop through by design */
	case SG_IO_PRESENT_NO:
//		return sisc_cmnd_io(dev_fd, iop, rqbuf, rqblen, report);
		return 0;
	case SG_IO_PRESENT_YES:
		return sg_io_cmnd_io(dev_fd, iop, report, 0);
	default:
		printf(">>>> do_scsi_cmnd_io: bad sg_io_state=%d\n", sg_io_state);
		sg_io_state = SG_IO_PRESENT_UNKNOWN;
		return -EIO;    /* report error and reset state */
	}
}

/* SCSI Interface */
bool
scsi_pass_through(int fd, struct scsi_cmnd_io *iop)
{
	int report = 2;
	int status = do_normal_scsi_cmnd_io(fd, iop, report);
  	if (status < 0)
      		return false;
  	return true;
}

/////////////////////////////////////////////////////////////////////////////
//	NEW ADD
/////////////////////////////////////////////////////////////////////////////
int
do_scsi_request_sense(int fd, caddr_t buf, int buflen, void *rqbuf,
		      int rqblen, void *p)
{
//	struct uscsi_cmd ucmd;
//	union scsi_cdb cdb;
	struct scsi_cmnd_io iop;
	uint8_t cdb[6];
	int status;

	int report = 2;
	p = &iop;

	(void) memset(buf, 0, buflen);
//	(void) memset(&ucmd, 0, sizeof(ucmd));
	(void) memset(&iop, 0, sizeof(iop));
//	(void) memset(&cdb, 0, sizeof(union scsi_cdb));
	(void) memset(cdb, 0, sizeof(cdb));
//	cdb.scc_cmd = SCMD_REQUEST_SENSE;
//	FORMG0COUNT(&cdb, (uchar_t)buflen);
	cdb[0] = REQUEST_SENSE;
	cdb[4] = buflen;
//	ucmd.uscsi_cdb = (caddr_t)&cdb;
//	ucmd.uscsi_cdblen = CDB_GROUP0;		// #define CDB_GROUP0      6       /*  6-byte cdb's */
//	ucmd.uscsi_bufaddr = buf;
//	ucmd.uscsi_buflen = buflen;
	iop.scsi_cdb = cdb;
	iop.scsi_cdblen = sizeof(cdb);
	iop.scsi_bufaddr = buf;
	iop.scsi_buflen = buflen;
	iop.scsi_rqbuf = rqbuf;			// sensep
	iop.max_sense_len = rqblen;
	iop.scsi_dir = DXFER_FROM_DEVICE;
//	status = uscsi_cmd(fd, &iop, rqbuf, rqblen);
	status = do_normal_scsi_cmnd_io(fd, &iop, report);
	if (status)
		printf("Request sense failed\n");
	if (status == 0)
		ddump("Request Sense data:", buf, buflen);

	return (status);	
}

/*
 * Execute a uscsi mode sense command.  This can only be used to return one page
 * at a time.  Return the mode header/block descriptor and the actual page data
 * separately - this allows us to support devices which return either 0 or 1
 * block descriptors.  Whatever a device gives us in the mode header/block
 * descriptor will be returned to it upon subsequent mode selects.
 */
int
do_scsi_mode_sense(int fd, int page_code, int page_control, caddr_t page_data,
	int page_size, struct scsi_ms_header *header,
	void *rqbuf, int rqblen, void *p)
{
	caddr_t mode_sense_buf;
	struct mode_header *hdr;
	struct mode_page *pg;
	int nbytes;
//	struct uscsi_cmd ucmd;
//	union scsi_cdb cdb;
	struct scsi_cmnd_io iop;
	uint8_t cdb[6];
	int status;
	int maximum;
	char *pc;

	int report = 2;
	p = &iop;

	assert(page_size >= 0 && page_size < 256);
	assert(page_control == PC_CURRENT || page_control == PC_CHANGEABLE ||
		page_control == PC_DEFAULT || page_control == PC_SAVED);

    	nbytes = sizeof(struct scsi_ms_header) + page_size;
	mode_sense_buf = alloca((uint_t)nbytes);

	/*
	 * Build and execute the uscsi ioctl
 	 */
	(void) memset(mode_sense_buf, 0, nbytes);
//	(void) memset(&ucmd, 0, sizeof(ucmd));
	(void) memset(&iop, 0, sizeof(iop));
//	(void) memset(&cdb, 0, sizeof(union scsi_cdb));
	(void) memset(cdb, 0, sizeof(cdb));
//	cdb.scc_cmd = SCMD_MODE_SENSE;
//	FORMG0COUNT(&cdb, (uchar_t)nbytes);
//	cdb.cdb_opaque[2] = page_control | page_code;
	cdb[0] = MODE_SENSE;
	cdb[2] = page_control | page_code;
	cdb[4] = nbytes;
//	ucmd.uscsi_cdb = (caddr_t)&cdb;
//	ucmd.uscsi_cdblen = CDB_GROUP0;
//	ucmd.uscsi_bufaddr = mode_sense_buf;
//	ucmd.uscsi_buflen = nbytes;
	iop.scsi_cdb = cdb;
	iop.scsi_cdblen = 6;
	iop.scsi_bufaddr = mode_sense_buf;
	iop.scsi_buflen = nbytes;
	iop.scsi_rqbuf = rqbuf;                 // sensep
	iop.max_sense_len = rqblen;
	iop.scsi_dir = DXFER_FROM_DEVICE;
//	status = uscsi_cmd(fd, &ucmd, rqbuf, rqblen);
	status = do_normal_scsi_cmnd_io(fd, &iop, report);
	if (status) {
		printf("Mode sense page 0x%x failed\n", page_code);
		return (-1);
	}

	ddump("RAW MODE SENSE BUFFER", mode_sense_buf, nbytes);

	/*
	 * Verify that the returned data looks reasonable, find the actual page
 	 * data, and copy it into the user's buffer.  Copy the mode_header and
 	 * block_descriptor into the header structure, which can then be used to
 	 * return the same data to the drive when issuing a mode select.
 	 */
	hdr = (struct mode_header *)mode_sense_buf;
	(void) memset((caddr_t)header, 0, sizeof(struct scsi_ms_header));
	if (hdr->bdesc_length != sizeof(struct block_descriptor) &&
		hdr->bdesc_length != 0) {
		printf("\nMode sense page 0x%x: block descriptor "
			"length %d incorrect\n", page_code, hdr->bdesc_length);
		ddump("Mode sense:", mode_sense_buf, nbytes);
		return (-1);
	}
	(void) memcpy((caddr_t)header, mode_sense_buf,
	(int)(MODE_HEADER_LENGTH + hdr->bdesc_length));
	pg = (struct mode_page *)((ulong_t)mode_sense_buf +
		MODE_HEADER_LENGTH + hdr->bdesc_length);

	if (page_code == MODEPAGE_ALLPAGES) {
	/* special case */

		(void) memcpy(page_data, (caddr_t)pg,
			(hdr->length + sizeof(header->ms_header.length)) -
			(MODE_HEADER_LENGTH + hdr->bdesc_length));

		pc = find_string(page_control_strings, page_control);
		printf("\nMode sense page 0x%x (%s):\n", page_code,
			pc != NULL ? pc : "");
		ddump("header:", (caddr_t)header,
			sizeof (struct scsi_ms_header));
		ddump("data:", page_data,
			(hdr->length +
			sizeof (header->ms_header.length)) -
			(MODE_HEADER_LENGTH + hdr->bdesc_length));

		return (0);
	}

	if (pg->code != page_code) {
		printf("\nMode sense page 0x%x: incorrect page code 0x%x\n",
			page_code, pg->code);
		ddump("Mode sense:", mode_sense_buf, nbytes);
		return (-1);
	
	}

	/*
	 * Accept up to "page_size" bytes of mode sense data.  This allows us to
 	 * accept both CCS and SCSI-2 structures, as long as we request the
 	 * greater of the two.
 	 */
	 maximum = page_size - sizeof (struct mode_page);
	 if (((int)pg->length) > maximum) {
	 	printf("Mode sense page 0x%x: incorrect page "
			"length %d - expected max %d\n",
			page_code, pg->length, maximum);
		ddump("Mode sense:", mode_sense_buf, nbytes);
		return (-1);
	}

	(void) memcpy(page_data, (caddr_t)pg, MODESENSE_PAGE_LEN(pg));

	pc = find_string(page_control_strings, page_control);
	printf("\nMode sense page 0x%x (%s):\n", page_code,
		pc != NULL ? pc : "");
	ddump("header:", (caddr_t)header, sizeof (struct scsi_ms_header));
	ddump("data:", page_data, MODESENSE_PAGE_LEN(pg));

	return (0);
}

/*
 * Execute a uscsi MODE SENSE(10) command.  This can only be used to return one
 * page at a time.  Return the mode header/block descriptor and the actual page
 * data separately - this allows us to support devices which return either 0 or
 * 1 block descriptors.  Whatever a device gives us in the mode header/block
 * descriptor will be returned to it upon subsequent mode selects.
 */
int
do_scsi_mode_sense_10(int fd, int page_code, int page_control,
	caddr_t page_data, int page_size, struct scsi_ms_header_g1 *header,
	void *rqbuf, int rqblen, void *p)
{
	caddr_t mode_sense_buf;
	struct mode_header_g1 *hdr;
	struct mode_page *pg;
	int nbytes;
//	struct uscsi_cmd ucmd;
//	union scsi_cdb cdb;
	struct scsi_cmnd_io iop;
	uint8_t cdb[10];
	int status;
	int maximum;
	ushort_t length, bdesc_length;
	char *pc;

	int report = 2;
	p = &iop;

	assert(page_size >= 0 && page_size < UINT16_MAX);
	assert(page_control == PC_CURRENT || page_control == PC_CHANGEABLE ||
	    page_control == PC_DEFAULT || page_control == PC_SAVED);

	nbytes = sizeof (struct scsi_ms_header_g1) + page_size;
	mode_sense_buf = alloca((uint_t)nbytes);

	(void) memset(mode_sense_buf, 0, nbytes);
//	(void) memset((char *)&ucmd, 0, sizeof(ucmd));
	(void) memset((char *)&iop, 0, sizeof(iop));
//	(void) memset((char *)&cdb, 0, sizeof(union scsi_cdb));
	(void) memset(cdb, 0, sizeof(cdb));
//	cdb.scc_cmd = SCMD_MODE_SENSE_G1;
//	FORMG1COUNT(&cdb, (uint16_t)nbytes);
//	cdb.cdb_opaque[2] = page_control | page_code;
	cdb[0] = MODE_SENSE_10;
	cdb[2] = page_control | page_code;
//	cdb[7] = (bufLen >> 8) & 0xff;
	cdb[7] = (nbytes >> 8) & 0xff;
//	cdb[8] = bufLen & 0xff;
	cdb[8] = nbytes & 0xff;
//	ucmd.uscsi_cdb = (caddr_t)&cdb;
//	ucmd.uscsi_cdblen = CDB_GROUP1;
//	ucmd.uscsi_bufaddr = mode_sense_buf;
//	ucmd.uscsi_buflen = nbytes;
	iop.scsi_cdb = cdb;
	iop.scsi_cdblen = 10;
	iop.scsi_bufaddr = mode_sense_buf;
	iop.scsi_buflen = nbytes;
	iop.scsi_rqbuf = rqbuf;                 // sensep
	iop.max_sense_len = rqblen;
	iop.scsi_dir = DXFER_FROM_DEVICE;

//	status = uscsi_cmd(fd, &ucmd, rqbuf, rqblen);
	status = do_normal_scsi_cmnd_io(fd, &iop, report);
	if (status) {
		printf("Mode sense(10) page 0x%x failed\n",
		    page_code);
		return (-1);
	}

	ddump("RAW MODE SENSE(10) BUFFER", mode_sense_buf, nbytes);

	/*
	 * Verify that the returned data looks reasonable, find the actual page
	 * data, and copy it into the user's buffer.  Copy the mode_header and
	 * block_descriptor into the header structure, which can then be used to
	 * return the same data to the drive when issuing a mode select.
	 */
	/* LINTED */
	hdr = (struct mode_header_g1 *)mode_sense_buf;

	length = BE_16(hdr->length);
	bdesc_length = BE_16(hdr->bdesc_length);

	(void) memset((caddr_t)header, 0, sizeof (struct scsi_ms_header_g1));
	if (bdesc_length != sizeof (struct block_descriptor) &&
	    bdesc_length != 0) {
		printf("\nMode sense(10) page 0x%x: block descriptor "
		    "length %d incorrect\n", page_code, bdesc_length);
		ddump("Mode sense(10):", mode_sense_buf, nbytes);
		return (-1);
	}
	(void) memcpy((caddr_t)header, mode_sense_buf,
		(int)(MODE_HEADER_LENGTH_G1 + bdesc_length));
	pg = (struct mode_page *)((ulong_t)mode_sense_buf +
		MODE_HEADER_LENGTH_G1 + bdesc_length);

	if (page_code == MODEPAGE_ALLPAGES) {
		/* special case */

		(void) memcpy(page_data, (caddr_t)pg,
		    (length + sizeof (header->ms_header.length)) -
		    (MODE_HEADER_LENGTH_G1 + bdesc_length));

		pc = find_string(page_control_strings, page_control);
		printf("\nMode sense(10) page 0x%x (%s):\n",
		    page_code, pc != NULL ? pc : "");
		ddump("header:", (caddr_t)header,
		    MODE_HEADER_LENGTH_G1 + bdesc_length);

		ddump("data:", page_data,
		    (length + sizeof (header->ms_header.length)) -
		    (MODE_HEADER_LENGTH_G1 + bdesc_length));

		return (0);
	}

	if (pg->code != page_code) {
		printf("\nMode sense(10) page 0x%x: incorrect page "
		    "code 0x%x\n", page_code, pg->code);
		ddump("Mode sense(10):", mode_sense_buf, nbytes);
		return (-1);
	}

	/*
	 * Accept up to "page_size" bytes of mode sense data.  This allows us to
	 * accept both CCS and SCSI-2 structures, as long as we request the
	 * greater of the two.
	 */
	maximum = page_size - sizeof (struct mode_page);
	if (((int)pg->length) > maximum) {
		printf("Mode sense(10) page 0x%x: incorrect page "
		    "length %d - expected max %d\n",
		    page_code, pg->length, maximum);
		ddump("Mode sense(10):", mode_sense_buf,
		    nbytes);
		return (-1);
	}

	(void) memcpy(page_data, (caddr_t)pg, MODESENSE_PAGE_LEN(pg));

	pc = find_string(page_control_strings, page_control);
	printf("\nMode sense(10) page 0x%x (%s):\n", page_code,
	    pc != NULL ? pc : "");
	ddump("header:", (caddr_t)header,
	    sizeof (struct scsi_ms_header_g1));
	ddump("data:", page_data, MODESENSE_PAGE_LEN(pg));

	return (0);
}

/*
 * Execute a uscsi mode select command.
 */
int
do_scsi_mode_select(int fd, int page_code, int options, caddr_t page_data,
	int page_size, struct scsi_ms_header *header,
	void *rqbuf, int rqblen, void *p)
{
	caddr_t mode_select_buf;
	int nbytes;
//	struct uscsi_cmd ucmd;
//	union scsi_cdb cdb;
	struct scsi_cmnd_io iop;
	uint8_t cdb[6];
	int status;
	char *s;

	int report = 2;
	p = &iop;

	assert(((struct mode_page *)page_data)->ps == 0);
	assert(header->ms_header.length == 0);
	assert(header->ms_header.device_specific == 0);
	assert((options & ~(MODE_SELECT_SP|MODE_SELECT_PF)) == 0);

	nbytes = sizeof (struct scsi_ms_header) + page_size;
	mode_select_buf = alloca((uint_t)nbytes);

	/*
	 * Build the mode select data out of the header and page data This
	 * allows us to support devices which return either 0 or 1 block
	 * descriptors.
	 */
	(void) memset(mode_select_buf, 0, nbytes);
	nbytes = MODE_HEADER_LENGTH;
	if (header->ms_header.bdesc_length ==
	    sizeof (struct block_descriptor)) {
		nbytes += sizeof (struct block_descriptor);
	}

	s = find_string(mode_select_strings,
		options & (MODE_SELECT_SP|MODE_SELECT_PF));
	printf("\nMode select page 0x%x%s:\n", page_code,
		s != NULL ? s : "");
	ddump("header:", (caddr_t)header, nbytes);
	ddump("data:", (caddr_t)page_data, page_size);

	/*
	 * Put the header and data together
	 */
	(void) memcpy(mode_select_buf, (caddr_t)header, nbytes);
	(void) memcpy(mode_select_buf + nbytes, page_data, page_size);
	nbytes += page_size;

	/*
	 * Build and execute the uscsi ioctl
	 */
//	(void) memset((char *)&ucmd, 0, sizeof(ucmd));
	(void) memset((char *)&iop, 0, sizeof(iop));
//	(void) memset((char *)&cdb, 0, sizeof(union scsi_cdb));
	(void) memset(cdb, 0, sizeof(cdb));
//	cdb.scc_cmd = SCMD_MODE_SELECT;
//	FORMG0COUNT(&cdb, (uchar_t)nbytes);
//	cdb.cdb_opaque[1] = (uchar_t)options;
	cdb[0] = MODE_SELECT;
//	cdb[1] = 0x10 | (sp & 1);	/* set PF (page format) bit always */
	cdb[1] = (uchar_t)options;
//	cdb[4] = hdr_plus_1_pg;		/* make sure only one page sent */
//	ucmd.uscsi_cdb = (caddr_t)&cdb;
//	ucmd.uscsi_cdblen = CDB_GROUP0;
//	ucmd.uscsi_bufaddr = mode_select_buf;
//	ucmd.uscsi_buflen = nbytes;
	iop.scsi_cdb = cdb;
	iop.scsi_cdblen = 6;
	iop.scsi_bufaddr = mode_select_buf;
	iop.scsi_buflen = nbytes;
	iop.scsi_rqbuf = rqbuf;                 // sensep
	iop.max_sense_len = rqblen;
	iop.scsi_dir = DXFER_TO_DEVICE;
//	status = uscsi_cmd(fd, &ucmd, rqbuf, rqblen);
	status = do_normal_scsi_cmnd_io(fd, &iop, report);

	if (status)
		printf("Mode select page 0x%x failed\n", page_code);

	return (status);
}

/*
 * Execute a uscsi mode select(10) command.
 */
int
do_scsi_mode_select_10(int fd, int page_code, int options,
	caddr_t page_data, int page_size, struct scsi_ms_header_g1 *header,
	void *rqbuf, int rqblen, void *p)
{
	caddr_t				mode_select_buf;
	int				nbytes;
//	struct uscsi_cmd		ucmd;
//	union scsi_cdb			cdb;
	struct scsi_cmnd_io iop;
	uint8_t cdb[10];
	int				status;
	char				*s;

	int report = 2;
	p = &iop;

	assert(((struct mode_page *)page_data)->ps == 0);
	assert(header->ms_header.length == 0);
	assert(header->ms_header.device_specific == 0);
	assert((options & ~(MODE_SELECT_SP|MODE_SELECT_PF)) == 0);

	nbytes = sizeof (struct scsi_ms_header_g1) + page_size;
	mode_select_buf = alloca((uint_t)nbytes);

	/*
	 * Build the mode select data out of the header and page data
	 * This allows us to support devices which return either
	 * 0 or 1 block descriptors.
	 */
	(void) memset(mode_select_buf, 0, nbytes);
	nbytes = sizeof (struct mode_header_g1);
	if (BE_16(header->ms_header.bdesc_length) ==
	    sizeof (struct block_descriptor)) {
		nbytes += sizeof (struct block_descriptor);
	}

	/*
	 * Dump the structures
	 */
	s = find_string(mode_select_strings,
		options & (MODE_SELECT_SP|MODE_SELECT_PF));
	printf("\nMode select(10) page 0x%x%s:\n", page_code,
	    s != NULL ? s : "");
	ddump("header:", (caddr_t)header, nbytes);
	ddump("data:", (caddr_t)page_data, page_size);

	/*
	 * Put the header and data together
	 */
	(void) memcpy(mode_select_buf, (caddr_t)header, nbytes);
	(void) memcpy(mode_select_buf + nbytes, page_data, page_size);
	nbytes += page_size;

	/*
	 * Build and execute the uscsi ioctl
	 */
//	(void) memset((char *)&ucmd, 0, sizeof(ucmd));
	(void) memset((char *)&iop, 0, sizeof(iop));
//	(void) memset((char *)&cdb, 0, sizeof(union scsi_cdb));
	(void) memset(cdb, 0, sizeof(cdb));
//	cdb.scc_cmd = SCMD_MODE_SELECT_G1;
//	FORMG1COUNT(&cdb, (uint16_t)nbytes);
//	cdb.cdb_opaque[1] = (uchar_t)options;
	cdb[0] = MODE_SELECT_10;
//	cdb[1] = 0x10 | (sp & 1);      /* set PF (page format) bit always */
	cdb[1] = (uchar_t)options;
//	cdb[8] = hdr_plus_1_pg; /* make sure only one page sent */
//	ucmd.uscsi_cdb = (caddr_t)&cdb;
//	ucmd.uscsi_cdblen = CDB_GROUP1;
//	ucmd.uscsi_bufaddr = mode_select_buf;
//	ucmd.uscsi_buflen = nbytes;
	iop.scsi_cdb = cdb;
	iop.scsi_cdblen = 10;
	iop.scsi_bufaddr = mode_select_buf;
	iop.scsi_buflen = nbytes;
	iop.scsi_rqbuf = rqbuf;                 // sensep
	iop.max_sense_len = rqblen;
	iop.scsi_dir = DXFER_TO_DEVICE;
//	status = uscsi_cmd(fd, &ucmd, rqbuf, rqblen);
	status = do_normal_scsi_cmnd_io(fd, &iop, report);

	if (status)
		printf("Mode select(10) page 0x%x failed\n", page_code);

	return (status);
}

/////////////////////////////////////////////////////////////////////////////

int
do_scsi_log_sense(int fd, int page_code, int page_control, caddr_t page_data,
	int page_size, void *rqbuf, int rqblen, void *p)
{
	caddr_t log_sense_buf;	
	scsi_log_header_t *hdr;
//	struct uscsi_cmd ucmd;

	/*Ben add*/
	struct scsi_cmnd_io iop;

//	union scsi_cdb cdb;
	uint8_t cdb[10];
	int status;
	ushort_t len;
	char *pc;

	int report = 2;
	p = &iop;

	assert(page_size >= 0 && page_size < UINT16_MAX);
	assert(page_control == PC_CURRENT || page_control == PC_CHANGEABLE ||
		page_control == PC_DEFAULT || page_control == PC_SAVED);

	if (page_size < sizeof (scsi_log_header_t))
		return (-1);

	log_sense_buf = alloca((uint_t)page_size);

	/*
	 * Build and execute the uscsi ioctl
	 */
	(void) memset(log_sense_buf, 0, page_size);
//	(void) memset((char *)&ucmd, 0, sizeof(ucmd));
	(void) memset((char *)&iop, 0, sizeof(iop));
//	(void) memset((char *)&cdb, 0, sizeof(union scsi_cdb));
	(void) memset(cdb, 0, sizeof(cdb));
//	cdb.scc_cmd = SCMD_LOG_SENSE_G1;
//	FORMG1COUNT(&cdb, (uint16_t)page_size);
//	cdb.cdb_opaque[2] = page_control | page_code;
	cdb[0] = LOG_SENSE;
	cdb[2] = page_control | page_code;	/* Page control (PC) == 1 */
	cdb[7] = (page_size >> 8) & 0xff;
	cdb[8] = page_size & 0xff;
//	ucmd.uscsi_cdb = (caddr_t)&cdb;
//	ucmd.uscsi_cdblen = CDB_GROUP1;
//	ucmd.uscsi_bufaddr = log_sense_buf;
//	ucmd.uscsi_buflen = page_size;
	iop.scsi_cdb = cdb;
	iop.scsi_cdblen = sizeof(cdb);
//	iop.scsi_cdblen = 10;		//#define CDB_GROUP1      10      /* 10-byte cdb's */
	iop.scsi_bufaddr = log_sense_buf;
	iop.scsi_buflen = page_size;
	iop.scsi_rqbuf = rqbuf;                 // sensep
	iop.max_sense_len = rqblen;
	iop.scsi_dir = DXFER_FROM_DEVICE;

//////////////
//	status = uscsi_cmd(fd, &ucmd, rqbuf, rqblen);

//	status = do_normal_scsi_cmnd_io(fd, /**/&ucmd, rqbuf, rqblen,/**/ report);
	status = do_normal_scsi_cmnd_io(fd, &iop, report);

//////////////
	if (status) {
		printf("Log sense page 0x%x failed\n", page_code);
		return (-1);
	}

	/*
	 * Verify that the returned data looks reasonable, then copy it into the
	 * user's buffer.
	 */
	hdr = (scsi_log_header_t *)log_sense_buf;

	/*
	 * Ensure we have a host-understandable length field
	 */
	len = BE_16(hdr->lh_length);

	if (hdr->lh_code != page_code) {
		printf("\nLog sense page 0x%x: incorrect page code 0x%x\n",
			page_code, hdr->lh_code);
		ddump("Log sense:", log_sense_buf, page_size);
		return (-1);
	}

	ddump("LOG SENSE RAW OUTPUT", log_sense_buf,
		sizeof (scsi_log_header_t) + len);

	/*
	 * Accept up to "page_size" bytes of mode sense data.  This allows us to
	 * accept both CCS and SCSI-2 structures, as long as we request the
	 * greater of the two.
	 */
	(void) memcpy(page_data, (caddr_t)hdr, len +
		sizeof (scsi_log_header_t));

	pc = find_string(page_control_strings, page_control);
	printf("\nLog sense page 0x%x (%s):\n", page_code,
		pc != NULL ? pc : "");
	ddump("header:", (caddr_t)hdr,
		sizeof (scsi_log_header_t));
	ddump("data:", (caddr_t)hdr +
		sizeof (scsi_log_header_t), len);

	return (0);
}

