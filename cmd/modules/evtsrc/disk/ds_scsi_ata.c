/*
 * ds_scsi_ata.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * You should have received a copy of the GNU General Public License
 * (for example COPYING); if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * The code in this file is based on the SCSI to ATA Translation (SAT)
 * draft found at http://www.t10.org . The original draft used for this
 * code is sat-r08.pdf which is not too far away from becoming a
 * standard. The SAT commands of interest to smartmontools are the
 * ATA PASS THROUGH SCSI (16) and ATA PASS THROUGH SCSI (12) defined in
 * section 12 of that document.
 *
 * sat-r09.pdf is the most recent, easily accessible draft prior to the
 * original SAT standard (ANSI INCITS 431-2007). By mid-2009 the second
 * version of the SAT standard (SAT-2) is nearing standardization. In
 * their wisdom an incompatible change has been introduced in draft
 * sat2r08a.pdf in the area of the ATA RETURN DESCRIPTOR. A new "fixed
 * format" ATA RETURN buffer has been defined (sat2r08b.pdf section
 * 12.2.7) for the case when DSENSE=0 in the Control mode page.
 * Unfortunately this is the normal case. If the change stands our
 * code will need to be extended for this case.
 *
 * With more transports "hiding" SATA disks (and other S-ATAPI devices)
 * behind a SCSI command set, accessing special features like SMART
 * information becomes a challenge. The SAT standard offers ATA PASS
 * THROUGH commands for special usages. Note that the SAT layer may
 * be inside a generic OS layer (e.g. libata in linux), in a host
 * adapter (HA or HBA) firmware, or somewhere on the interconnect
 * between the host computer and the SATA devices (e.g. a RAID made
 * of SATA disks and the RAID talks "SCSI" to the host computer).
 * Note that in the latter case, this code does not solve the
 * addressing issue (i.e. which SATA disk to address behind the logical
 * SCSI (RAID) interface).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "ds_scsi_ioctl.h"
#include "ds_ata_ioctl.h" // ataReadHDIdentity()
#include "dev_interface.h"

/* This is a slightly stretched SCSI sense "descriptor" format header.
   The addition is to allow the 0x70 and 0x71 response codes. The idea
   is to place the salient data of both "fixed" and "descriptor" sense
   format into one structure to ease application processing.
   The original sense buffer should be kept around for those cases
   in which more information is required (e.g. the LBA of a MEDIUM ERROR). */
/// Abridged SCSI sense data
struct sg_scsi_sense_hdr {
	unsigned char response_code; /* permit: 0x0, 0x70, 0x71, 0x72, 0x73 */
    	unsigned char sense_key;
    	unsigned char asc;
    	unsigned char ascq;
    	unsigned char byte4;
   	unsigned char byte5;
   	unsigned char byte6;
    	unsigned char additional_length;
};

/* Maps the salient data from a sense buffer which is in either fixed or
   descriptor format into a structure mimicking a descriptor format
   header (i.e. the first 8 bytes of sense descriptor format).
   If zero response code returns 0. Otherwise returns 1 and if 'sshp' is
   non-NULL then zero all fields and then set the appropriate fields in
   that structure. sshp::additional_length is always 0 for response
   codes 0x70 and 0x71 (fixed format). */
static int sg_scsi_normalize_sense(const unsigned char *sensep, int sb_len,
                                   struct sg_scsi_sense_hdr * sshp);

/* Attempt to find the first SCSI sense data descriptor that matches the
   given 'desc_type'. If found return pointer to start of sense data
   descriptor; otherwise (including fixed format sense data) returns NULL. */
static const unsigned char *sg_scsi_sense_desc_find(const unsigned char *sensep,
                                                     int sense_len, int desc_type);

/// SAT support.
/// Implements ATA by tunnelling through SCSI.

#define SAT_ATA_PASSTHROUGH_12LEN 12
#define SAT_ATA_PASSTHROUGH_16LEN 16

#define DEF_SAT_ATA_PASSTHRU_SIZE 16
#define ATA_RETURN_DESCRIPTOR 9

static int m_passthrulen = 16;
// cdb[0]: ATA PASS THROUGH (16) SCSI command opcode byte (0x85)
// cdb[1]: multiple_count, protocol + extend
// cdb[2]: offline, ck_cond, t_dir, byte_block + t_length
// cdb[3]: features (15:8)
// cdb[4]: features (7:0)
// cdb[5]: sector_count (15:8)
// cdb[6]: sector_count (7:0)
// cdb[7]: lba_low (15:8)
// cdb[8]: lba_low (7:0)
// cdb[9]: lba_mid (15:8)
// cdb[10]: lba_mid (7:0)
// cdb[11]: lba_high (15:8)
// cdb[12]: lba_high (7:0)
// cdb[13]: device
// cdb[14]: (ata) command
// cdb[15]: control (SCSI, leave as zero)
//
// 24 bit lba (from MSB): cdb[12] cdb[10] cdb[8]
// 48 bit lba (from MSB): cdb[11] cdb[9] cdb[7] cdb[12] cdb[10] cdb[8]
//
//
// cdb[0]: ATA PASS THROUGH (12) SCSI command opcode byte (0xa1)
// cdb[1]: multiple_count, protocol + extend
// cdb[2]: offline, ck_cond, t_dir, byte_block + t_length
// cdb[3]: features (7:0)
// cdb[4]: sector_count (7:0)
// cdb[5]: lba_low (7:0)
// cdb[6]: lba_mid (7:0)
// cdb[7]: lba_high (7:0)
// cdb[8]: device
// cdb[9]: (ata) command
// cdb[10]: reserved
// cdb[11]: control (SCSI, leave as zero)
//
//
// ATA Return Descriptor (component of descriptor sense data)
// des[0]: descriptor code (0x9)
// des[1]: additional descriptor length (0xc)
// des[2]: extend (bit 0)
// des[3]: error
// des[4]: sector_count (15:8)
// des[5]: sector_count (7:0)
// des[6]: lba_low (15:8)
// des[7]: lba_low (7:0)
// des[8]: lba_mid (15:8)
// des[9]: lba_mid (7:0)
// des[10]: lba_high (15:8)
// des[11]: lba_high (7:0)
// des[12]: device
// des[13]: status

// PURPOSE
//   This interface routine takes ATA SMART commands and packages
//   them in the SAT-defined ATA PASS THROUGH SCSI commands. There are
//   two available SCSI commands: a 12 byte and 16 byte variant; the
//   one used is chosen via this->m_passthrulen .
// DETAILED DESCRIPTION OF ARGUMENTS
//   device: is the file descriptor provided by (a SCSI dvice type) open()
//   command: defines the different ATA operations.
//   select: additional input data if needed (which log, which type of
//           self-test).
//   data:   location to write output data, if needed (512 bytes).
//     Note: not all commands use all arguments.
// RETURN VALUES
//  -1 if the command failed
//   0 if the command succeeded,
//   STATUS_CHECK routine: 
//  -1 if the command failed
//   0 if the command succeeded and disk SMART status is "OK"
//   1 if the command succeeded and disk SMART status is "FAILING"

bool
ata_pass_through(int fd, const ata_cmd_in_t *in, ata_cmd_out_t *out)
{
	if (!ata_cmd_is_ok(in, true, // data_out_support
			       true, // multi_sector_support
			       true)) // ata_48bit_support
    		return false;

    	struct scsi_cmnd_io io_hdr;
//    	struct scsi_sense_disect sinfo;
    	struct sg_scsi_sense_hdr ssh;
    	unsigned char cdb[SAT_ATA_PASSTHROUGH_16LEN];
    	unsigned char sense[32];
    	const unsigned char *ardp;
    	int status, ard_len, have_sense;
    	int extend = 0;
    	int ck_cond = 0;    /* set to 1 to read register(s) back */
    	int protocol = 3;   /* non-data */
    	int t_dir = 1;      /* 0 -> to device, 1 -> from device */
    	int byte_block = 1; /* 0 -> bytes, 1 -> 512 byte blocks */
    	int t_length = 0;   /* 0 -> no data transferred */
    	int passthru_size = DEF_SAT_ATA_PASSTHRU_SIZE;

    	memset(cdb, 0, sizeof(cdb));
    	memset(sense, 0, sizeof(sense));

    	// Set data direction
    	// TODO: This works only for commands where sector_count holds count!
    	switch (in->direction) {
      		case no_data:
        		break;
      		case data_in:
        		protocol = 4;  // PIO data-in
        		t_length = 2;  // sector_count holds count
        		break;
      		case data_out:
        		protocol = 5;  // PIO data-out
        		t_length = 2;  // sector_count holds count
        		t_dir = 0;     // to device
        		break;
      		default:
        		return set_err("sat_device::ata_pass_through: invalid direction=%d",
            				(int)in->direction);
    	}

    	// Check condition if any output register needed
	if (in->out_needed.f_is_set(&(in->out_needed)))
        	ck_cond = 1;

    	if ((SAT_ATA_PASSTHROUGH_12LEN == m_passthrulen) ||
            (SAT_ATA_PASSTHROUGH_16LEN == m_passthrulen))
        	passthru_size = m_passthrulen;

    	// Set extend bit on 48-bit ATA command
    	if (in->in_regs.is_48bit_cmd(&(in->in_regs))) {
      		if (passthru_size != SAT_ATA_PASSTHROUGH_16LEN)
        		return set_err("48-bit ATA commands require SAT ATA PASS-THROUGH (16)");
      		extend = 1;
    	}

    	cdb[0] = (SAT_ATA_PASSTHROUGH_12LEN == passthru_size) ?
             	  SAT_ATA_PASSTHROUGH_12 : SAT_ATA_PASSTHROUGH_16;

    	cdb[1] = (protocol << 1) | extend;
    	cdb[2] = (ck_cond << 5)  | (t_dir << 3) |
                 (byte_block << 2) | t_length;

    	if (passthru_size == SAT_ATA_PASSTHROUGH_12LEN) {
        	// ATA PASS-THROUGH (12)
        	const ata_in_regs_t lo = in->in_regs.in_48bit;
        	cdb[3] = lo.features.get_m_val(&(lo.features));
        	cdb[4] = lo.sector_count.get_m_val(&(lo.sector_count));
        	cdb[5] = lo.lba_low.get_m_val(&(lo.lba_low));
        	cdb[6] = lo.lba_mid.get_m_val(&(lo.lba_mid));
        	cdb[7] = lo.lba_high.get_m_val(&(lo.lba_high));
        	cdb[8] = lo.device.get_m_val(&(lo.device));
        	cdb[9] = lo.command.get_m_val(&(lo.command));
    	} else {
        	// ATA PASS-THROUGH (16)
        	const ata_in_regs_t lo = in->in_regs.in_48bit;
        	const ata_in_regs_t hi = in->in_regs.prev;
        	// Note: all 'in.in_regs.prev.*' are always zero for 28-bit commands
        	cdb[ 3] = hi.features.get_m_val(&(hi.features));
        	cdb[ 4] = lo.features.get_m_val(&(lo.features));
        	cdb[ 5] = hi.sector_count.get_m_val(&(hi.sector_count));
        	cdb[ 6] = lo.sector_count.get_m_val(&(lo.sector_count));
        	cdb[ 7] = hi.lba_low.get_m_val(&(hi.lba_low));
        	cdb[ 8] = lo.lba_low.get_m_val(&(lo.lba_low));
        	cdb[ 9] = hi.lba_mid.get_m_val(&(hi.lba_mid));
        	cdb[10] = lo.lba_mid.get_m_val(&(lo.lba_mid));
        	cdb[11] = hi.lba_high.get_m_val(&(hi.lba_high));
        	cdb[12] = lo.lba_high.get_m_val(&(lo.lba_high));
        	cdb[13] = lo.device.get_m_val(&(lo.device));
        	cdb[14] = lo.command.get_m_val(&(lo.command));
    	}

    	memset(&io_hdr, 0, sizeof(io_hdr));
    	if (0 == t_length) {
        	io_hdr.scsi_dir = DXFER_NONE;
        	io_hdr.scsi_buflen = 0;
    	} else if (t_dir) {         /* from device */
        	io_hdr.scsi_dir = DXFER_FROM_DEVICE;
        	io_hdr.scsi_buflen = in->size;
        	io_hdr.scsi_bufaddr = (unsigned char *)in->buffer;
        	memset(in->buffer, 0, in->size); // prefill with zeroes
    	} else {                    /* to device */
        	io_hdr.scsi_dir = DXFER_TO_DEVICE;
        	io_hdr.scsi_buflen = in->size;
        	io_hdr.scsi_bufaddr = (unsigned char *)in->buffer;
    	}
    	io_hdr.scsi_cdb = cdb;
    	io_hdr.scsi_cdblen = passthru_size;
    	io_hdr.scsi_rqbuf = sense;
    	io_hdr.max_sense_len = sizeof(sense);
    	io_hdr.scsi_timeout = 20;		//SCSI_TIMEOUT_DEFAULT;

//    	scsi_device * scsidev = get_tunnel_dev();
    	if (!scsi_pass_through(fd, &io_hdr)) {
        	printf("sat_device::ata_pass_through: scsi_pass_through() failed, "
                       "errno=.. [..]\n");
        	return false;
    	}
    	ardp = NULL;
    	ard_len = 0;
    	have_sense = sg_scsi_normalize_sense(io_hdr.scsi_rqbuf, io_hdr.resp_sense_len,
                                         &ssh);
    	if (have_sense) {
        	/* look for SAT ATA Return Descriptor */
        	ardp = sg_scsi_sense_desc_find(io_hdr.scsi_rqbuf,
                                       	       io_hdr.resp_sense_len,
                                       	       ATA_RETURN_DESCRIPTOR);
        	if (ardp) {
            		ard_len = ardp[1] + 2;
            		if (ard_len < 12)
                		ard_len = 12;
            		else if (ard_len > 14)
                		ard_len = 14;
        	}
//        	scsi_do_sense_disect(&io_hdr, &sinfo);
//        	status = scsiSimpleSenseFilter(&sinfo);
//        	if (0 != status) {
//                	printf("ata_pass_through: scsi error: %s\n", scsiErrString(status));
//                	printf("Values from ATA Return Descriptor are:\n");
//                	dStrHex((const char *)ardp, ard_len, 1);
//            		if (t_dir && (t_length > 0) && (in->direction == data_in))
//                		memset(in->buffer, 0, in->size);
//            		return set_err("scsi error %s", scsiErrString(status));
//        	}
    	}
    	if (ck_cond) {     /* expecting SAT specific sense data */
        	if (have_sense) {
            		if (ardp) {
                    		printf("Values from ATA Return Descriptor are:\n");
                    		dStrHex((const char *)ardp, ard_len, 1);
                		// Set output registers
                		ata_out_regs_t lo = out->out_regs.out_48bit;
                		lo.error.set_m_val(&(lo.error), ardp[3]);
                		lo.sector_count.set_m_val(&(lo.sector_count), ardp[5]);
                		lo.lba_low.set_m_val(&(lo.lba_low), ardp[7]);
                		lo.lba_mid.set_m_val(&(lo.lba_mid), ardp[9]);
                		lo.lba_high.set_m_val(&(lo.lba_high), ardp[11]);
                		lo.device.set_m_val(&(lo.device), ardp[12]);
                		lo.status.set_m_val(&(lo.status), ardp[13]);
                		if (in->in_regs.is_48bit_cmd(&(in->in_regs))) {
                    			ata_out_regs_t hi = out->out_regs.prev;
                    			hi.sector_count.set_m_val(&(hi.sector_count), ardp[4]);
                    			hi.lba_low.set_m_val(&(hi.lba_low), ardp[6]);
                    			hi.lba_mid.set_m_val(&(hi.lba_mid), ardp[8]);
                    			hi.lba_high.set_m_val(&(hi.lba_high), ardp[10]);
                		}
            		}
        	}
        	if (ardp == NULL)
            		ck_cond = 0;       /* not the type of sense data expected */
    	}
    	if (0 == ck_cond) {
        	if (have_sense) {
            		if ((ssh.response_code >= 0x72) &&
                	   ((SCSI_SK_NO_SENSE == ssh.sense_key) ||
                            (SCSI_SK_RECOVERED_ERR == ssh.sense_key)) &&
                            (0 == ssh.asc) && (SCSI_ASCQ_ATA_PASS_THROUGH == ssh.ascq)) {
                		if (ardp) {
                        		printf("Values from ATA Return Descriptor are:\n");
                        		dStrHex((const char *)ardp, ard_len, 1);
                    			return set_err("SAT command failed");
                		}
            		}
        	}
    	}
    	return true;
}

/////////////////////////////////////////////////////////////////////////////

/* Next two functions are borrowed from sg_lib.c in the sg3_utils
   package. Same copyrght owner, same license as this file. */
static int
sg_scsi_normalize_sense(const unsigned char *sensep, int sb_len,
                                   struct sg_scsi_sense_hdr *sshp)
{
    if (sshp)
        memset(sshp, 0, sizeof(struct sg_scsi_sense_hdr));
    if ((NULL == sensep) || (0 == sb_len) || (0x70 != (0x70 & sensep[0])))
        return 0;
    if (sshp) {
        sshp->response_code = (0x7f & sensep[0]);
        if (sshp->response_code >= 0x72) {  /* descriptor format */
            if (sb_len > 1)
                sshp->sense_key = (0xf & sensep[1]);
            if (sb_len > 2)
                sshp->asc = sensep[2];
            if (sb_len > 3)
                sshp->ascq = sensep[3];
            if (sb_len > 7)
                sshp->additional_length = sensep[7];
        } else {                              /* fixed format */
            if (sb_len > 2)
                sshp->sense_key = (0xf & sensep[2]);
            if (sb_len > 7) {
                sb_len = (sb_len < (sensep[7] + 8)) ? sb_len :
                                                      (sensep[7] + 8);
                if (sb_len > 12)
                    sshp->asc = sensep[12];
                if (sb_len > 13)
                    sshp->ascq = sensep[13];
            }
        }
    }
    return 1;
}

static const unsigned char *
sg_scsi_sense_desc_find(const unsigned char *sensep, int sense_len, int desc_type)
{
    int add_sen_len, add_len, desc_len, k;
    const unsigned char *descp;

    if ((sense_len < 8) || (0 == (add_sen_len = sensep[7])))
        return NULL;
    if ((sensep[0] < 0x72) || (sensep[0] > 0x73))
        return NULL;
    add_sen_len = (add_sen_len < (sense_len - 8)) ?
                         add_sen_len : (sense_len - 8);
    descp = &sensep[8];
    for (desc_len = 0, k = 0; k < add_sen_len; k += desc_len) {
        descp += desc_len;
        add_len = (k < (add_sen_len - 1)) ? descp[1]: -1;
        desc_len = add_len + 2;
        if (descp[0] == desc_type)
            return descp;
        if (add_len < 0) /* short descriptor ?? */
            break;
    }
    return NULL;
}

#if 0
// Call scsi_pass_through and check sense.
// TODO: Provide as member function of class scsi_device (?)
static bool
scsi_pass_through_and_check(scsi_device * scsidev,  scsi_cmnd_io * iop,
			const char * msg = "")
{
  // Provide sense buffer
  unsigned char sense[32] = {0, };
  iop->sensep = sense;
  iop->max_sense_len = sizeof(sense);
  iop->timeout = SCSI_TIMEOUT_DEFAULT;

  // Run cmd
  if (!scsi_pass_through(iop)) {
      printf("%sscsi_pass_through() failed, errno=.. [..]\n", msg);
    return false;
  }

  // Check sense
  scsi_sense_disect sinfo;
  scsi_do_sense_disect(iop, &sinfo);
  int err = scsiSimpleSenseFilter(&sinfo);
  if (err) {
      printf("%sscsi error: ..\n", msg);
    return scsidev->set_err(EIO, "scsi error %s", scsiErrString(err));
  }

  return true;
}


/////////////////////////////////////////////////////////////////////////////

// Return ATA->SCSI filter for SAT or USB.
get_sat_device(const char * type, scsi_device * scsidev)
{
  if (!strncmp(type, "sat", 3)) {
    int ptlen = 0, n1 = -1, n2 = -1;
    if (!(((sscanf(type, "sat%n,%d%n", &n1, &ptlen, &n2) == 1 && n2 == (int)strlen(type)) || n1 == (int)strlen(type))
        && (ptlen == 0 || ptlen == 12 || ptlen == 16))) {
      set_err(EINVAL, "Option '-d sat,<n>' requires <n> to be 0, 12 or 16");
      return 0;
    }
    return new sat_device(this, scsidev, type, ptlen);
  }
}

// Try to detect a SAT device behind a SCSI interface.

ata_device * smart_interface::autodetect_sat_device(scsi_device * scsidev,
  const unsigned char * inqdata, unsigned inqsize)
{
  if (!scsidev->is_open())
    return 0;

  // SAT ?
  if (inqdata && inqsize >= 36 && !memcmp(inqdata + 8, "ATA     ", 8)) { // TODO: Linux-specific?
    ata_device_auto_ptr atadev( new sat_device(this, scsidev, "") , scsidev);
    if (has_sat_pass_through(atadev.get()))
      return atadev.release(); // Detected SAT
  }

  return 0;
}
#endif

