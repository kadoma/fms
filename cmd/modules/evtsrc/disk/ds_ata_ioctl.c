/*
 * ds_ata_ioctl.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 * This file contains the linux-specific IOCTL parts of
 * smartmontools. It includes one interface routine for ATA devices,
 * one for SCSI devices, and one for ATA devices behind escalade
 * controllers.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

//#include "sense.h"	//	SCSI Use!
#include "ds_ata.h"
#include "dev_interface.h"
#include "ds_ata_ioctl.h"
//#include "dev_interface.h"

///////////////////////////////////////////////////////////////////////
//		  	  Linux ATA support			     //
///////////////////////////////////////////////////////////////////////
// PURPOSE
//   This is an interface routine meant to isolate the OS dependent
//   parts of the code, and to provide a debugging interface.  Each
//   different port and OS needs to provide it's own interface.  This
//   is the linux one.
// DETAILED DESCRIPTION OF ARGUMENTS
//   device: is the file descriptor provided by open()
//   command: defines the different operations.
//   select: additional input data if needed (which log, which type of
//           self-test).
//   data:   location to write output data, if needed (512 bytes).
//   Note: not all commands use all arguments.
// RETURN VALUES
//  -1 if the command failed
//   0 if the command succeeded,
//   STATUS_CHECK routine:
//  -1 if the command failed
//   0 if the command succeeded and disk SMART status is "OK"
//   1 if the command succeeded and disk SMART status is "FAILING"

#define BUFFER_LENGTH (4+512)

int
ata_command_interface(int fd, smart_command_set command, int select, char *data)
{
	unsigned char buff[BUFFER_LENGTH];
	// positive: bytes to write to caller.  negative: bytes to READ from
	// caller. zero: non-data command
	int copydata = 0;

	const int HDIO_DRIVE_CMD_OFFSET = 4;

	// See struct hd_drive_cmd_hdr in hdreg.h.  Before calling ioctl()
	// buff[0]: ATA COMMAND CODE REGISTER
	// buff[1]: ATA SECTOR NUMBER REGISTER == LBA LOW REGISTER
	// buff[2]: ATA FEATURES REGISTER
	// buff[3]: ATA SECTOR COUNT REGISTER

	// Note that on return:
	// buff[2] contains the ATA SECTOR COUNT REGISTER

	// clear out buff.  Large enough for HDIO_DRIVE_CMD (4+512 bytes)
	memset(buff, 0, BUFFER_LENGTH);

	buff[0] = ATA_SMART_CMD;
	switch(command) {
		case READ_VALUES:
			buff[2] = ATA_SMART_READ_VALUES;
			buff[3] = 1;
			copydata = 512;
			break;
		case READ_THRESHOLDS:
			buff[2] = ATA_SMART_READ_THRESHOLDS;
			buff[1] = buff[3] = 1;
			copydata = 512;
			break;
		case READ_LOG:
			buff[2] = ATA_SMART_READ_LOG_SECTOR;
			buff[1] = select;
			buff[3] = 1;
			copydata = 512;
			break;
		case WRITE_LOG:
			break;
		case IDENTIFY:
			buff[0] = ATA_IDENTIFY_DEVICE;
			buff[3] = 1;
			copydata = 512;
			break;
		case PIDENTIFY:
			buff[0] = ATA_IDENTIFY_PACKET_DEVICE;
			buff[3] = 1;
			copydata = 512;
			break;
		case ENABLE:
			buff[2] = ATA_SMART_ENABLE;
			buff[1] = 1;
			break;
		case STATUS:
		// this command only says if SMART is working.  It could be
		// replaced with STATUS_CHECK below.
			buff[2] = ATA_SMART_STATUS;
			break;
		default:
//			printf("Unrecognized command %d in linux_ata_command_interface()\n"
//			     "Please contact " PACKAGE_BUGREPORT "\n", command);
			printf("Unrecognized command .. in linux_ata_command_interface()\n");
			errno = ENOSYS;
			return -1;
	}

	// This command uses the HDIO_DRIVE_TASKFILE ioctl(). This is the
	// only ioctl() that can be used to WRITE data to the disk.
	if (command == WRITE_LOG) {
		unsigned char task[sizeof(ide_task_request_t) + 512];
		ide_task_request_t *reqtask = (ide_task_request_t *) task;
		task_struct_t *taskfile = (task_struct_t *)reqtask->io_ports;
		int retval;

		memset(task, 0, sizeof(task));

		taskfile->data 		 = 0;
		taskfile->feature 	 = ATA_SMART_WRITE_LOG_SECTOR;
		taskfile->sector_count	 = 1;
		taskfile->sector_number  = select;
		taskfile->low_cylinder   = 0x4f;
		taskfile->high_cylinder  = 0xc2;
		taskfile->device_head    = 0;
		taskfile->command        = ATA_SMART_CMD;

		reqtask->data_phase      = TASKFILE_OUT;
		reqtask->req_cmd         = IDE_DRIVE_TASK_OUT;
		reqtask->out_size        = 512;
		reqtask->in_size         = 0;

		// copy user data into the task request structure
		memcpy(task+sizeof(ide_task_request_t), data, 512);

		if ((retval = ioctl(fd, HDIO_DRIVE_TASKFILE, task))) {
			if (retval==-EINVAL) {
				printf("Kernel lacks HDIO_DRIVE_TASKFILE support;");
				printf("compile kernel with CONFIG_IDE_TASKFILE_IO set\n");
			}
			return -1;
		}
		return 0;
	}

	// There are two different types of ioctls().  The HDIO_DRIVE_TASK
	// one is this:
	if (command == STATUS_CHECK || command == AUTOSAVE || command == AUTO_OFFLINE) {
		int retval;

		// NOT DOCUMENTED in /usr/src/linux/include/linux/hdreg.h. You
		// have to read the IDE driver source code.  Sigh.
		// buff[0]: ATA COMMAND CODE REGISTER
		// buff[1]: ATA FEATURES REGISTER
		// buff[2]: ATA SECTOR_COUNT
		// buff[3]: ATA SECTOR NUMBER
		// buff[4]: ATA CYL LO REGISTER
		// buff[5]: ATA CYL HI REGISTER
		// buff[6]: ATA DEVICE HEAD

		unsigned const char normal_lo=0x4f, normal_hi=0xc2;
		unsigned const char failed_lo=0xf4, failed_hi=0x2c;
		buff[4]=normal_lo;
		buff[5]=normal_hi;

		if ((retval = ioctl(fd, HDIO_DRIVE_TASK, buff))) {
			if (retval==-EINVAL) {
				printf("Error SMART Status command via HDIO_DRIVE_TASK failed");
				printf("Rebuild older linux 2.2 kernels with HDIO_DRIVE_TASK support added\n");
			} else
				syserror("Error SMART Status command failed");
			return -1;
		}

		// Cyl low and Cyl high unchanged means "Good SMART status"
		if (buff[4]==normal_lo && buff[5]==normal_hi)
			return 0;

		// These values mean "Bad SMART status"
		if (buff[4]==failed_lo && buff[5]==failed_hi)
			return 1;

		// We haven't gotten output that makes sense; print out some debugging info
		syserror("Error SMART Status command failed");
//		printf("Please get assistance from " PACKAGE_HOMEPAGE "\n");
		printf("Please get assistance from PACKAGE_HOMEPAGE \n");
		printf("Register values returned from SMART Status command are:\n");
		printf("ST =0x%02x\n",(int)buff[0]);
		printf("ERR=0x%02x\n",(int)buff[1]);
		printf("NS =0x%02x\n",(int)buff[2]);
		printf("SC =0x%02x\n",(int)buff[3]);
		printf("CL =0x%02x\n",(int)buff[4]);
		printf("CH =0x%02x\n",(int)buff[5]);
		printf("SEL=0x%02x\n",(int)buff[6]);
		return -1;
	}

#if 1
	// Note to people doing ports to other OSes -- don't worry about
	// this block -- you can safely ignore it.  I have put it here
	// because under linux when you do IDENTIFY DEVICE to a packet
	// device, it generates an ugly kernel syslog error message.  This
	// is harmless but frightens users.  So this block detects packet
	// devices and make IDENTIFY DEVICE fail "nicely" without a syslog
	// error message.
	//
	// If you read only the ATA specs, it appears as if a packet device
	// *might* respond to the IDENTIFY DEVICE command.  This is
	// misleading - it's because around the time that SFF-8020 was
	// incorporated into the ATA-3/4 standard, the ATA authors were
	// sloppy. See SFF-8020 and you will see that ATAPI devices have
	// *always* had IDENTIFY PACKET DEVICE as a mandatory part of their
	// command set, and return 'Command Aborted' to IDENTIFY DEVICE.
	if (command == IDENTIFY || command == PIDENTIFY) {
		unsigned short deviceid[256];
		// check the device identity, as seen when the system was booted
		// or the device was FIRST registered.  This will not be current
		// if the user has subsequently changed some of the parameters. If
		// device is a packet device, swap the command interpretations.
		if (!ioctl(fd, HDIO_GET_IDENTITY, deviceid) && (deviceid[0] & 0x8000))
			buff[0]=(command==IDENTIFY)?ATA_IDENTIFY_PACKET_DEVICE:ATA_IDENTIFY_DEVICE;
	}
#endif

	// We are now doing the HDIO_DRIVE_CMD type ioctl.
	if ((ioctl(fd, HDIO_DRIVE_CMD, buff)))
		return -1;

	// CHECK POWER MODE command returns information in the Sector Count
	// register (buff[3]).  Copy to return data buffer.
	if (command==CHECK_POWER_MODE)
		buff[HDIO_DRIVE_CMD_OFFSET]=buff[2];

	// if the command returns data then copy it back
	if (copydata)
		memcpy(data, buff+HDIO_DRIVE_CMD_OFFSET, copydata);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * atacmds.cpp
 * 
 * Home page of code is: http://smartmontools.sourceforge.net
 *
 * Copyright (C) 2002-9 Bruce Allen <smartmontools-support@lists.sourceforge.net>
 * Copyright (C) 2008-9 Christian Franke <smartmontools-support@lists.sourceforge.net>
 * Copyright (C) 1999-2000 Michael Cornwell <cornwell@acm.org>
 * Copyright (C) 2000 Andre Hedrick <andre@linux-ide.org>
 */

#define SMART_CYL_LOW  0x4F
#define SMART_CYL_HI   0xC2

// SMART RETURN STATUS yields SMART_CYL_HI,SMART_CYL_LOW to indicate drive
// is healthy and SRET_STATUS_HI_EXCEEDED,SRET_STATUS_MID_EXCEEDED to
// indicate that a threshhold exceeded condition has been detected.
// Those values (byte pairs) are placed in ATA register "LBA 23:8".
#define SRET_STATUS_HI_EXCEEDED 0x2C
#define SRET_STATUS_MID_EXCEEDED 0xF4

// These Drive Identity tables are taken from hdparm 5.2, and are also
// given in the ATA/ATAPI specs for the IDENTIFY DEVICE command.  Note
// that SMART was first added into the ATA/ATAPI-3 Standard with
// Revision 3 of the document, July 25, 1995.  Look at the "Document
// Status" revision commands at the beginning of
// http://www.t13.org/project/d2008r6.pdf to see this.
#define NOVAL_0                 0x0000
#define NOVAL_1                 0xffff
/* word 81: minor version number */
#define MINOR_MAX 0x22
static const char *const minor_str[] = {       /* word 81 value: */
	"Device does not report version",             /* 0x0000       */
	"ATA-1 X3T9.2 781D prior to revision 4",      /* 0x0001       */
	"ATA-1 published, ANSI X3.221-1994",          /* 0x0002       */
	"ATA-1 X3T9.2 781D revision 4",               /* 0x0003       */
	"ATA-2 published, ANSI X3.279-1996",          /* 0x0004       */
	"ATA-2 X3T10 948D prior to revision 2k",      /* 0x0005       */
	"ATA-3 X3T10 2008D revision 1",               /* 0x0006       */ /* SMART NOT INCLUDED */
	"ATA-2 X3T10 948D revision 2k",               /* 0x0007       */
	"ATA-3 X3T10 2008D revision 0",               /* 0x0008       */ 
	"ATA-2 X3T10 948D revision 3",                /* 0x0009       */
	"ATA-3 published, ANSI X3.298-199x",          /* 0x000a       */
	"ATA-3 X3T10 2008D revision 6",               /* 0x000b       */ /* 1st VERSION WITH SMART */
	"ATA-3 X3T13 2008D revision 7 and 7a",        /* 0x000c       */
	"ATA/ATAPI-4 X3T13 1153D revision 6",         /* 0x000d       */
	"ATA/ATAPI-4 T13 1153D revision 13",          /* 0x000e       */
	"ATA/ATAPI-4 X3T13 1153D revision 7",         /* 0x000f       */
	"ATA/ATAPI-4 T13 1153D revision 18",          /* 0x0010       */
	"ATA/ATAPI-4 T13 1153D revision 15",          /* 0x0011       */
	"ATA/ATAPI-4 published, ANSI NCITS 317-1998", /* 0x0012       */
	"ATA/ATAPI-5 T13 1321D revision 3",           /* 0x0013       */
	"ATA/ATAPI-4 T13 1153D revision 14",          /* 0x0014       */
	"ATA/ATAPI-5 T13 1321D revision 1",           /* 0x0015       */
	"ATA/ATAPI-5 published, ANSI NCITS 340-2000", /* 0x0016       */
	"ATA/ATAPI-4 T13 1153D revision 17",          /* 0x0017       */
	"ATA/ATAPI-6 T13 1410D revision 0",           /* 0x0018       */
	"ATA/ATAPI-6 T13 1410D revision 3a",          /* 0x0019       */
	"ATA/ATAPI-7 T13 1532D revision 1",           /* 0x001a       */
	"ATA/ATAPI-6 T13 1410D revision 2",           /* 0x001b       */
	"ATA/ATAPI-6 T13 1410D revision 1",           /* 0x001c       */
	"ATA/ATAPI-7 published, ANSI INCITS 397-2005",/* 0x001d       */
	"ATA/ATAPI-7 T13 1532D revision 0",           /* 0x001e       */
	"reserved",                                   /* 0x001f       */
	"reserved",                                   /* 0x0020       */
	"ATA/ATAPI-7 T13 1532D revision 4a",          /* 0x0021       */
	"ATA/ATAPI-6 published, ANSI INCITS 361-2002" /* 0x0022       */
};

// NOTE ATA/ATAPI-4 REV 4 was the LAST revision where the device
// attribute structures were NOT completely vendor specific.  So any
// disk that is ATA/ATAPI-4 or above can not be trusted to show the
// vendor values in sensible format.

// Negative values below are because it doesn't support SMART
static const int actual_ver[] = { 
  /* word 81 value: */
  0,            /* 0x0000       WARNING:        */
  1,            /* 0x0001       WARNING:        */
  1,            /* 0x0002       WARNING:        */
  1,            /* 0x0003       WARNING:        */
  2,            /* 0x0004       WARNING:   This array           */
  2,            /* 0x0005       WARNING:   corresponds          */
  -3, /*<== */  /* 0x0006       WARNING:   *exactly*            */
  2,            /* 0x0007       WARNING:   to the ATA/          */
  -3, /*<== */  /* 0x0008       WARNING:   ATAPI version        */
  2,            /* 0x0009       WARNING:   listed in            */
  3,            /* 0x000a       WARNING:   the                  */
  3,            /* 0x000b       WARNING:   minor_str            */
  3,            /* 0x000c       WARNING:   array                */
  4,            /* 0x000d       WARNING:   above.               */
  4,            /* 0x000e       WARNING:                        */
  4,            /* 0x000f       WARNING:   If you change        */
  4,            /* 0x0010       WARNING:   that one,            */
  4,            /* 0x0011       WARNING:   change this one      */
  4,            /* 0x0012       WARNING:   too!!!               */
  5,            /* 0x0013       WARNING:        */
  4,            /* 0x0014       WARNING:        */
  5,            /* 0x0015       WARNING:        */
  5,            /* 0x0016       WARNING:        */
  4,            /* 0x0017       WARNING:        */
  6,            /* 0x0018       WARNING:        */
  6,            /* 0x0019       WARNING:        */
  7,            /* 0x001a       WARNING:        */
  6,            /* 0x001b       WARNING:        */
  6,            /* 0x001c       WARNING:        */
  7,            /* 0x001d       WARNING:        */
  7,            /* 0x001e       WARNING:        */
  0,            /* 0x001f       WARNING:        */
  0,            /* 0x0020       WARNING:        */
  7,            /* 0x0021       WARNING:        */
  6             /* 0x0022       WARNING:        */
};

// swap two bytes.  Point to low address
void swap2(char *location)
{
	char tmp=*location;
  	*location=*(location+1);
  	*(location+1)=tmp;
  	return;
}

// swap four bytes.  Point to low address
void swap4(char *location)
{
	char tmp=*location;
  	*location=*(location+3);
  	*(location+3)=tmp;
  	swap2(location+1);
  	return;
}

// swap eight bytes.  Points to low address
void swap8(char *location)
{
	char tmp=*location;
  	*location=*(location+7);
  	*(location+7)=tmp;
  	tmp=*(location+1);
  	*(location+1)=*(location+6);
  	*(location+6)=tmp;
  	swap4(location+2);
  	return;
}

// Typesafe variants using overloading
void swapx2(unsigned short *p)
{
	swap2((char*)p);
}

void swapx4(unsigned int *p)
{
	swap4((char*)p);
}

void swapx8(uint64_t *p)
{
	swap8((char*)p);
}

// Invalidate serial number and adjust checksum in IDENTIFY data
static void
invalidate_serno(ata_identify_device_t *id)
{
	unsigned char sum = 0;
	unsigned i;
  	for (i = 0; i < sizeof(id->serial_no); i++) {
    		sum += id->serial_no[i];
		sum -= id->serial_no[i] = 'X';
  	}
  	if ((id->words088_255[255-88] & 0x00ff) == 0x00a5)
    		id->words088_255[255-88] += sum << 8;
}

static const char *const commandstrings[] = {
	"SMART ENABLE",
	"SMART DISABLE",
	"SMART AUTOMATIC ATTRIBUTE SAVE",
	"SMART IMMEDIATE OFFLINE",
	"SMART AUTO OFFLINE",
	"SMART STATUS",
	"SMART STATUS CHECK",
	"SMART READ ATTRIBUTE VALUES",
	"SMART READ ATTRIBUTE THRESHOLDS",
	"SMART READ LOG",
	"IDENTIFY DEVICE",
	"IDENTIFY PACKET DEVICE",
	"CHECK POWER MODE",
	"SMART WRITE LOG",
//	"WARNING (UNDEFINED COMMAND -- CONTACT DEVELOPERS AT " PACKAGE_BUGREPORT ")\n"
	"WARNING (UNDEFINED COMMAND -- CONTACT DEVELOPERS AT   PACKAGE_BUGREPORT  )\n"
};

static const char *
preg(const ata_register_t r, char *buf)
{
	if (!r.is_set(&r))
		//return "n/a ";
		return "....";
	sprintf(buf, "0x%02x", r.get_m_val(&r));
	return buf;
}

void 
//print_regs(const char *prefix, const ata_in_regs_t *r, const char *suffix = "\n")
print_in_regs(const char *prefix, const ata_in_regs_t *r, const char *suffix)
{
	char bufs[7][4+1+13];
	dsprintf("%s FR=%s, SC=%s, LL=%s, LM=%s, LH=%s, DEV=%s, CMD=%s%s", prefix,
		preg(r->features, bufs[0]), preg(r->sector_count, bufs[1]), preg(r->lba_low, bufs[2]),
		preg(r->lba_mid, bufs[3]), preg(r->lba_high, bufs[4]), preg(r->device, bufs[5]),
		preg(r->command, bufs[6]), suffix);
}

void
//print_regs(const char *prefix, const ata_out_regs_t *r, const char *suffix = "\n")
print_out_regs(const char *prefix, const ata_out_regs_t *r, const char *suffix)
{
	char bufs[7][4+1+13];
	dsprintf("%sERR=%s, SC=%s, LL=%s, LM=%s, LH=%s, DEV=%s, STS=%s%s", prefix,
		preg(r->error, bufs[0]), preg(r->sector_count, bufs[1]), preg(r->lba_low, bufs[2]),
		preg(r->lba_mid, bufs[3]), preg(r->lba_high, bufs[4]), preg(r->device, bufs[5]),
		preg(r->status, bufs[6]), suffix);
}

static void
prettyprint(const unsigned char *p, const char *name)
{
	dsprintf("\n===== [%s] DATA START (BASE-16) =====\n", name);
	int i;
	for (i=0; i<512; i+=16, p+=16)
		// print complete line to avoid slow tty output and extra lines in syslog.
		dsprintf("%03d-%03d: %02x %02x %02x %02x %02x %02x %02x %02x "
                	    "%02x %02x %02x %02x %02x %02x %02x %02x\n",
			i, i+16-1,
			p[ 0], p[ 1], p[ 2], p[ 3], p[ 4], p[ 5], p[ 6], p[ 7],
			p[ 8], p[ 9], p[10], p[11], p[12], p[13], p[14], p[15]);
	dsprintf("===== [%s] DATA END (512 Bytes) =====\n\n", name);
}

// This function provides the pretty-print reporting for SMART
// commands: it implements the various -r "reporting" options for ATA
// ioctls.
int
//smartcommandhandler(ata_device *device, smart_command_set command, int select, char *data)
smartcommandhandler(int fd, smart_command_set command, int select, char *data)
{
	// TODO: Rework old stuff below
	// This conditional is true for commands that return data
	int getsdata = (command == PIDENTIFY || 
                command == IDENTIFY || 
                command == READ_LOG || 
                command == READ_THRESHOLDS || 
                command == READ_VALUES ||
                command == CHECK_POWER_MODE);

	int sendsdata = (command == WRITE_LOG);
  
        // conditional is true for commands that use parameters
        int usesparam = (command == READ_LOG || 
                         command == AUTO_OFFLINE || 
                         command == AUTOSAVE || 
                         command == IMMEDIATE_OFFLINE ||
                         command == WRITE_LOG);
                  
//	dsprintf("\nREPORT-IOCTL: Device=%s Command=%s", device->get_dev_name(), commandstrings[command]);
	dsprintf("\nREPORT-IOCTL: Device=   Command=%s", commandstrings[command]);
	if (usesparam)
		dsprintf(" InputParameter=%d\n", select);
	else
		dsprintf("\n");
  
	if ((getsdata || sendsdata) && !data) {
		printf("REPORT-IOCTL: Unable to execute command %s :", commandstrings[command]);
		printf("data destination address is NULL\n");
		return -1;
	}
  
	// The reporting is cleaner, and we will find coding bugs faster, if
	// the commands that failed clearly return empty (zeroed) data
  	// structures
  	if (getsdata) {
    		if (command == CHECK_POWER_MODE)
      			data[0] = 0;
    		else
      			memset(data, '\0', 512);
  	}

  	// if requested, pretty-print the input data structure
  	if (sendsdata)
    	//pout("REPORT-IOCTL: Device=%s Command=%s\n", device->get_dev_name(), commandstrings[command]);
    		prettyprint((unsigned char *)data, commandstrings[command]);

  	// now execute the command
  	int retval = -1;
  	{
    		ata_cmd_in_t in;
		ata_cmd_in_init(&in);
    		// Set common register values
    		switch (command) {
      		default: // SMART commands
        		in.in_regs.in_48bit.command.set_m_val(&(in.in_regs.in_48bit.command), ATA_SMART_CMD);
        		in.in_regs.in_48bit.lba_high.set_m_val(&(in.in_regs.in_48bit.lba_high), SMART_CYL_HI);
			in.in_regs.in_48bit.lba_mid.set_m_val(&(in.in_regs.in_48bit.lba_mid), SMART_CYL_LOW);
        		break;
      		case IDENTIFY:
		case PIDENTIFY:
		case CHECK_POWER_MODE: // Non SMART commands
        		break;
    		}
    		// Set specific values
    		switch (command) {
      		case IDENTIFY:
        		in.in_regs.in_48bit.command.set_m_val(&(in.in_regs.in_48bit.command), ATA_IDENTIFY_DEVICE);
        		in.set_data_in(&in, data, 1);
        		break;
      		case PIDENTIFY:
        		in.in_regs.in_48bit.command.set_m_val(&(in.in_regs.in_48bit.command), ATA_IDENTIFY_PACKET_DEVICE);
        		in.set_data_in(&in, data, 1);
        		break;
      		case READ_VALUES:
        		in.in_regs.in_48bit.features.set_m_val(&(in.in_regs.in_48bit.features), ATA_SMART_READ_VALUES);
        		in.set_data_in(&in, data, 1);
        		break;
      		case READ_THRESHOLDS:
        		in.in_regs.in_48bit.features.set_m_val(&(in.in_regs.in_48bit.features), ATA_SMART_READ_THRESHOLDS);
        		in.in_regs.in_48bit.lba_low.set_m_val(&(in.in_regs.in_48bit.lba_low), 1); // TODO: CORRECT ???
        		in.set_data_in(&in, data, 1);
        		break;
      		case READ_LOG:
        		in.in_regs.in_48bit.features.set_m_val(&(in.in_regs.in_48bit.features), ATA_SMART_READ_LOG_SECTOR);
        		in.in_regs.in_48bit.lba_low.set_m_val(&(in.in_regs.in_48bit.lba_low), select);
        		in.set_data_in(&in, data, 1);
        		break;
      		case WRITE_LOG:
        		in.in_regs.in_48bit.features.set_m_val(&(in.in_regs.in_48bit.features), ATA_SMART_WRITE_LOG_SECTOR);
        		in.in_regs.in_48bit.lba_low.set_m_val(&(in.in_regs.in_48bit.lba_low), select);
        		in.set_data_out(&in, data, 1);
        		break;
      		case ENABLE:
        		in.in_regs.in_48bit.features.set_m_val(&(in.in_regs.in_48bit.features), ATA_SMART_ENABLE);
        		in.in_regs.in_48bit.lba_low.set_m_val(&(in.in_regs.in_48bit.lba_low), 1); // TODO: CORRECT ???
        		break;
     		case STATUS:
        		in.in_regs.in_48bit.features.set_m_val(&(in.in_regs.in_48bit.features), ATA_SMART_STATUS);
        		break;
      		default:
        		printf("Unrecognized command %d in smartcommandhandler()\n", command);
        		printf("       Please contact PACKAGE_BUGREPORT\n");
        		//set_err(ENOSYS);
        		errno = ENOSYS;
        		return -1;
    		}

      		print_in_regs(" Input:  ", &(in.in_regs.in_48bit),
//        		(in.direction == ata_cmd_in::data_in ? " IN\n":
//         		 in.direction == ata_cmd_in::data_out ? " OUT\n":"\n"));
			(in.direction == data_in ? " IN\n":
			 in.direction == data_out ? " OUT\n":"\n"));

    		ata_cmd_out_t out;
		ata_cmd_out_init(&out);
    		bool ok = ata_pass_through(fd, &in, &out);

    		if (out.out_regs.out_48bit.out_is_set(&(out.out_regs.out_48bit)))
      			print_out_regs(" Output: ", &(out.out_regs.out_48bit), "\n");

    		if (ok)
			switch (command) {
      			default:
        			retval = 0;
        			break;
  			}
	}

  	// If requested, invalidate serial number before any printing is done
  	if ((command == IDENTIFY || command == PIDENTIFY) && !retval)
    		invalidate_serno((ata_identify_device_t *)data);

      	dsprintf("REPORT-IOCTL: Device=   Command=%s returned %d\n",
           	commandstrings[command], retval);
    
	if (getsdata) {
    		// if requested, pretty-print the output data structure
      		if (command == CHECK_POWER_MODE)
			printf("Sector Count Register (BASE-16): %02x\n", (unsigned char)(*data));
      		else
			prettyprint((unsigned char *)data, commandstrings[command]);
	}

//  	errno = device->get_errno(); // TODO: Callers should not call syserror()

	return retval;
}

// Get number of sectors from IDENTIFY sector. If the drive doesn't
// support LBA addressing or has no user writable sectors
// (eg, CDROM or DVD) then routine returns zero.
uint64_t
get_num_sectors(const ata_identify_device_t *drive)
{
	unsigned short command_set_2  = drive->command_set_2;
  	unsigned short capabilities_0 = drive->words047_079[49-47];
  	unsigned short sects_16       = drive->words047_079[60-47];
  	unsigned short sects_32       = drive->words047_079[61-47];
  	unsigned short lba_16         = drive->words088_255[100-88];
  	unsigned short lba_32         = drive->words088_255[101-88];
  	unsigned short lba_48         = drive->words088_255[102-88];
  	unsigned short lba_64         = drive->words088_255[103-88];

  	// LBA support?
  	if (!(capabilities_0 & 0x0200))
    		return 0; // No

  	// if drive supports LBA addressing, determine 32-bit LBA capacity
  	uint64_t lba32 = (unsigned int)sects_32 << 16 |
                   	 (unsigned int)sects_16 << 0  ;

  	uint64_t lba64 = 0;
  	// if drive supports 48-bit addressing, determine THAT capacity
  	if ((command_set_2 & 0xc000) == 0x4000 && (command_set_2 & 0x0400))
      		 lba64 = (uint64_t)lba_64 << 48 |
              	         (uint64_t)lba_48 << 32 |
              		 (uint64_t)lba_32 << 16 |
              		 (uint64_t)lba_16 << 0;

  	// return the larger of the two possible capacities
  	return (lba32 > lba64 ? lba32 : lba64);
}

// This function computes the checksum of a single disk sector (512
// bytes).  Returns zero if checksum is OK, nonzero if the checksum is
// incorrect.  The size (512) is correct for all SMART structures.
unsigned char
checksum(const void *data)
{
	unsigned char sum = 0;
	int i;
  	for (i = 0; i < 512; i++)
    		sum += ((const unsigned char *)data)[i];
  	return sum;
}

// Copies n bytes (or n-1 if n is odd) from in to out, but swaps adjacents
// bytes.
void
swapbytes(char *out, const char *in, size_t n)
{
	size_t i;
	for (i = 0; i < n; i += 2) {
    		out[i]   = in[i+1];
    		out[i+1] = in[i];
  	}
}

// Copies in to out, but removes leading and trailing whitespace.
void
trim(char *out, const char *in)
{
	// Find the first non-space character (maybe none).
  	int first = -1;
  	int i;
  	for (i = 0; in[i]; i++)
    		if (!isspace((int)in[i])) {
      			first = i;
      			break;
    		}

  	if (first == -1) {
    	// There are no non-space characters.
    		out[0] = '\0';
    		return;
  	}

  	// Find the last non-space character.
  	for (i = strlen(in)-1; i >= first && isspace((int)in[i]); i--)
    		;
  	int last = i;

  	strncpy(out, in+first, last-first+1);
  	out[last-first+1] = '\0';
}

// Convenience function for formatting strings from ata_identify_device
void
format_ata_string(char *out, const char *in, int n, bool fix_swap)
{
	bool must_swap = !fix_swap;
  	char tmp[65];
  	n = n > 64 ? 64 : n;
  	if (!must_swap)
    		strncpy(tmp, in, n);
  	else
    		swapbytes(tmp, in, n);
  	tmp[n] = '\0';
  	trim(out, tmp);
}
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////



// Reads current Device Identity info (512 bytes) into buf.  Returns 0
// if all OK.  Returns -1 if no ATA Device identity can be
// established.  Returns >0 if Device is ATA Packet Device (not SMART
// capable).  The value of the integer helps identify the type of
// Packet device, which is useful so that the user can connect the
// formal device number with whatever object is inside their computer.
int
ataReadHDIdentity(ds_ata_info_t *aip, ata_identify_device_t *buf)
{
	unsigned short *rawshort=(unsigned short *)buf;
	unsigned char  *rawbyte =(unsigned char  *)buf;

	// See if device responds either to IDENTIFY DEVICE or IDENTIFY
  	// PACKET DEVICE
  	if ((smartcommandhandler(aip->ai_dsp->ds_fd, IDENTIFY, 0, (char *)buf))) {
    		if (smartcommandhandler(aip->ai_dsp->ds_fd, PIDENTIFY, 0, (char *)buf)) {
      			return -1; 
    		}
  	}

  	// If there is a checksum there, validate it
  	if ((rawshort[255] & 0x00ff) == 0x00a5 && checksum(rawbyte))
    		checksumwarning("Drive Identity Structure");
  
  	// If this is a PACKET DEVICE, return device type
  	if (rawbyte[1] & 0x80)
    		return 1+(rawbyte[1] & 0x1f);
  
  	// Not a PACKET DEVICE
  	return 0;
}

#if 0
// Returns ATA version as an integer, and a pointer to a string
// describing which revision.  Note that Revision 0 of ATA-3 does NOT
// support SMART.  For this one case we return -3 rather than +3 as
// the version number.  See notes above.
int
ataVersionInfo(const char **description, const ata_identify_device *drive, unsigned short *minor)
{
	// check that arrays at the top of this file are defined
	// consistently
  	if (sizeof(minor_str) != sizeof(char *)*(1+MINOR_MAX)) {
    		printf("Internal error in ataVersionInfo().  minor_str[] size %d\n"
         		"is not consistent with value of MINOR_MAX+1 = %d\n", 
         		(int)(sizeof(minor_str)/sizeof(char *)), MINOR_MAX+1);
    		fflush(NULL);
    		abort();
  	}
  	if (sizeof(actual_ver) != sizeof(int)*(1+MINOR_MAX)) {
    		printf("Internal error in ataVersionInfo().  actual_ver[] size %d\n"
         		"is not consistent with value of MINOR_MAX = %d\n",
         		(int)(sizeof(actual_ver)/sizeof(int)), MINOR_MAX+1);
    		fflush(NULL);
    		abort();
  	}

  	// get major and minor ATA revision numbers
  	unsigned short major = drive->major_rev_num;
  	*minor=drive->minor_rev_num;
  
  	// First check if device has ANY ATA version information in it
  	if (major==NOVAL_0 || major==NOVAL_1) {
    		*description=NULL;
    		return -1;
  	}
  
  	// The minor revision number has more information - try there first
  	if (*minor && (*minor<=MINOR_MAX)) {
    		int std = actual_ver[*minor];
    		if (std) {
      			*description=minor_str[*minor];
      			return std;
    		}
  	}

  	// Try new ATA-8 minor revision numbers (Table 31 of T13/1699-D Revision 6)
  	// (not in actual_ver/minor_str to avoid large sparse tables)
  	const char *desc;
  	switch (*minor) {
    		case 0x0027: desc = "ATA-8-ACS revision 3c"; break;
    		case 0x0028: desc = "ATA-8-ACS revision 6"; break;
    		case 0x0029: desc = "ATA-8-ACS revision 4"; break;
    		case 0x0033: desc = "ATA-8-ACS revision 3e"; break;
    		case 0x0039: desc = "ATA-8-ACS revision 4c"; break;
    		case 0x0042: desc = "ATA-8-ACS revision 3f"; break;
    		case 0x0052: desc = "ATA-8-ACS revision 3b"; break;
    		case 0x0107: desc = "ATA-8-ACS revision 2d"; break;
    		default:     desc = 0; break;
  	}
  	if (desc) {
    		*description = desc;
    		return 8;
  	}

  	// HDPARM has a very complicated algorithm from here on. Since SMART only
  	// exists on ATA-3 and later standards, let's punt on this.  If you don't
  	// like it, please fix it.  The code's in CVS.
  	int i;
  	for (i=15; i>0; i--)
    		if (major & (0x1<<i))
      			break;
  
  		*description=NULL; 
  		if (i==0)
    			return 1;
  		else
    			return i;
}
#endif

// returns 1 if SMART supported, 0 if SMART unsupported, -1 if can't tell
int
ataSmartSupport(const ata_identify_device_t *drive)
{
	unsigned short word82 = drive->command_set_1;
	unsigned short word83 = drive->command_set_2;
  
  	// check if words 82/83 contain valid info
  	if ((word83 >> 14) == 0x01)
    		// return value of SMART support bit 
    		return word82 & 0x0001;
  
  	// since we can're rely on word 82, we don't know if SMART supported
  	return -1;
}

// returns 1 if SMART enabled, 0 if SMART disabled, -1 if can't tell
int
ataIsSmartEnabled(const ata_identify_device_t *drive)
{
	unsigned short word85 = drive->cfs_enable_1;
	unsigned short word87 = drive->csf_default;
  
  	// check if words 85/86/87 contain valid info
  	if ((word87 >> 14) == 0x01)
    		// return value of SMART enabled bit
    		return word85 & 0x0001;
  
  	// Since we can't rely word85, we don't know if SMART is enabled.
  	return -1;
}

// Reads SMART attributes into *data
int
ataReadSmartValues(ds_ata_info_t *aip, ata_smart_values_t *data)
{
	if (smartcommandhandler(aip->ai_dsp->ds_fd, READ_VALUES, 0, (char *)data)) {
		syserror("Error SMART Values Read failed");
    		return -1;
  	}

  	// compute checksum
  	if (checksum(data))
    		checksumwarning("SMART Attribute Data Structure");
  
  	// swap endian order if needed
  	if (isbigendian()) {
    		int i;
    		swap2((char *)&(data->revnumber));
    		swap2((char *)&(data->total_time_to_complete_off_line));
    		swap2((char *)&(data->smart_capability));
    		for (i=0; i<NUMBER_ATA_SMART_ATTRIBUTES; i++) {
      			ata_smart_attribute_t *x=data->vendor_attributes+i;
      			swap2((char *)&(x->flags));
    		}
  	}

	return 0;
}

// This corrects some quantities that are byte reversed in the SMART
// SELF TEST LOG
void
fixsamsungselftestlog(ata_smart_selftestlog_t *data)
{
	// bytes 508/509 (numbered from 0) swapped (swap of self-test index
	// with one byte of reserved.
	swap2((char *)&(data->mostrecenttest));

  	// LBA low register (here called 'selftestnumber", containing
  	// information about the TYPE of the self-test) is byte swapped with
  	// Self-test execution status byte.  These are bytes N, N+1 in the
  	// entries.
	int i;
  	for (i = 0; i < 21; i++)
    		swap2((char *)&(data->selftest_struct[i].selftestnumber));

  	return;
}

// Reads the Self Test Log (log #6)
int
ataReadSelfTestLog(ds_ata_info_t *aip, ata_smart_selftestlog_t *data,
                        unsigned char fix_firmwarebug)
{
	// get data from device
  	if (smartcommandhandler(aip->ai_dsp->ds_fd, READ_LOG, 0x06, (char *)data)) {
    		syserror("Error SMART Error Self-Test Log Read failed");
    		return -1;
  	}

  	// compute its checksum, and issue a warning if needed
  	if (checksum(data))
    		checksumwarning("SMART Self-Test Log Structure");
  
  	// fix firmware bugs in self-test log
  	if (fix_firmwarebug == FIX_SAMSUNG)
    		fixsamsungselftestlog(data);

  	// swap endian order if needed
  	if (isbigendian()) {
    		int i;
    		swap2((char*)&(data->revnumber));
    		for (i=0; i<21; i++) {
      			ata_smart_selftestlog_struct_t *x=data->selftest_struct+i;
      			swap2((char *)&(x->timestamp));
      			swap4((char *)&(x->lbafirstfailure));
    		}
  	}

  	return 0;
}

// Print checksum warning for multi sector log
void
check_multi_sector_sum(const void *data, unsigned nsectors, const char *msg)
{
	unsigned errs = 0;
	unsigned i;
	for (i = 0; i < nsectors; i++) {
    		if (checksum((const unsigned char *)data + i*512))
      			errs++;
  	}
  	if (errs > 0) {
    		if (nsectors == 1)
      			checksumwarning(msg);
    		else
//     			checksumwarning(strprintf("%s (%u/%u)", msg, errs, nsectors).c_str());
			checksumwarning(msg);
  	}
}

// Read SMART Extended Self-test Log
bool
ataReadExtSelfTestLog(ds_ata_info_t *aip, ata_smart_extselftestlog_t *log,
                           unsigned nsectors)
{
	if (!ataReadLogExt(aip, 0x07, 0x00, 0, log, nsectors))
    		return false;

  	check_multi_sector_sum(log, nsectors, "SMART Extended Self-test Log Structure");

  	if (isbigendian()) {
    		swapx2(&log->log_desc_index);
		unsigned i;
    		for (i = 0; i < nsectors; i++) {
			unsigned j;
      			for (j = 0; j < 19; j++)
        			swapx2(&log->log_descs[i].timestamp);
    		}
  	}
	return true;
}

// Read GP Log page(s)
bool
ataReadLogExt(ds_ata_info_t *aip, unsigned char logaddr,
                   unsigned char features, unsigned page,
                   void *data, unsigned nsectors)
{
	ata_cmd_in_t in;
	ata_cmd_in_init(&in);
	in.in_regs.in_48bit.command.set_m_val(&(in.in_regs.in_48bit.command), ATA_READ_LOG_EXT);
  	in.in_regs.in_48bit.features.set_m_val(&(in.in_regs.in_48bit.features), features); // log specific
  	in.set_data_in_48bit(&in, data, nsectors);
  	in.in_regs.in_48bit.lba_low.set_m_val(&(in.in_regs.in_48bit.lba_low), logaddr);
  	in.in_regs.lba_mid_16.set_alias_16_val(&(in.in_regs.lba_mid_16), page);

  	if (!ata_pass_through_in(aip->ai_dsp->ds_fd, &in)) { // TODO: Debug output
    		if (nsectors <= 1) {
      			printf("ATA_READ_LOG_EXT (addr=0x%02x:0x%02x, page=%u, n=%u) failed: \n",
           			logaddr, features, page, nsectors);
      			return false;
    		}

    		// Recurse to retry with single sectors,
    		// multi-sector reads may not be supported by ioctl.
		unsigned i;
    		for (i = 0; i < nsectors; i++) {
      			if (!ataReadLogExt(aip, logaddr, features, page + i,
                        	 (char *)data + 512*i, 1))
        			return false;
    		}
  	}

  	return true;
}

// Read SMART Log page(s)
bool
ataReadSmartLog(ds_ata_info_t *aip, unsigned char logaddr,
                     void *data, unsigned nsectors)
{
	ata_cmd_in_t in;
	ata_cmd_in_init(&in);
	in.in_regs.in_48bit.command.set_m_val(&(in.in_regs.in_48bit.command), ATA_SMART_CMD);
	in.in_regs.in_48bit.features.set_m_val(&(in.in_regs.in_48bit.features), ATA_SMART_READ_LOG_SECTOR);
	in.set_data_in(&in, data, nsectors);
	in.in_regs.in_48bit.lba_high.set_m_val(&(in.in_regs.in_48bit.lba_high), SMART_CYL_HI);
	in.in_regs.in_48bit.lba_mid.set_m_val(&(in.in_regs.in_48bit.lba_mid), SMART_CYL_LOW);
	in.in_regs.in_48bit.lba_low.set_m_val(&(in.in_regs.in_48bit.lba_low), logaddr);

	if (!ata_pass_through_in(aip->ai_dsp->ds_fd, &in)) { // TODO: Debug output
		printf("ATA_SMART_READ_LOG failed: \n");
		return false;
  	}

  	return true;
}

// Reads the SMART or GPL Log Directory (log #0)
int
ataReadLogDirectory(ds_ata_info_t *aip, ata_smart_log_directory_t *data, bool gpl)
{
	if (!gpl) { // SMART Log directory
    		if (smartcommandhandler(aip->ai_dsp->ds_fd, READ_LOG, 0x00, (char *)data))
      			return -1;
  	} else { // GP Log directory
    		if (!ataReadLogExt(aip, 0x00, 0x00, 0, data, 1))
      			return -1;
  	}

  	// swap endian order if needed
  	if (isbigendian())
    		swapx2(&data->logversion);

  	return 0;
}

// This corrects some quantities that are byte reversed in the SMART
// ATA ERROR LOG.
void
fixsamsungerrorlog(ata_smart_errorlog_t *data)
{
	// FIXED IN SAMSUNG -25 FIRMWARE???
  	// Device error count in bytes 452-3
  	swap2((char *)&(data->ata_error_count));
  
  	// FIXED IN SAMSUNG -22a FIRMWARE
  	// step through 5 error log data structures
	int i;
  	for (i = 0; i < 5; i++) {
    		// step through 5 command data structures
		int j;
    		for (j = 0; j < 5; j++)
      		// Command data structure 4-byte millisec timestamp.  These are
      		// bytes (N+8, N+9, N+10, N+11).
      			swap4((char *)&(data->errorlog_struct[i].commands[j].timestamp));
    		// Error data structure two-byte hour life timestamp.  These are
    		// bytes (N+28, N+29).
    		swap2((char *)&(data->errorlog_struct[i].error_struct.timestamp));
  	}
  	return;
}

// NEEDED ONLY FOR SAMSUNG -22 (some) -23 AND -24?? FIRMWARE
void
fixsamsungerrorlog2(ata_smart_errorlog_t *data)
{
	// Device error count in bytes 452-3
  	swap2((char *)&(data->ata_error_count));
  	return;
}

// Reads the Summary SMART Error Log (log #1). The Comprehensive SMART
// Error Log is #2, and the Extended Comprehensive SMART Error log is
// #3
int
ataReadErrorLog(ds_ata_info_t *aip, ata_smart_errorlog_t *data,
                     unsigned char fix_firmwarebug)
{ 
	// get data from device
  	if (smartcommandhandler(aip->ai_dsp->ds_fd, READ_LOG, 0x01, (char *)data)) {
    		syserror("Error SMART Error Log Read failed");
    		return -1;
  	}
  
  	// compute its checksum, and issue a warning if needed
  	if (checksum(data))
    		checksumwarning("SMART ATA Error Log Structure");
  
  	// Some disks have the byte order reversed in some SMART Summary
  	// Error log entries
  	if (fix_firmwarebug == FIX_SAMSUNG)
    		fixsamsungerrorlog(data);
 	else if (fix_firmwarebug == FIX_SAMSUNG2)
    		fixsamsungerrorlog2(data);

  	// swap endian order if needed
  	if (isbigendian()) {
    		int i,j;
    
    		// Device error count in bytes 452-3
    		swap2((char *)&(data->ata_error_count));
    
    		// step through 5 error log data structures
    		for (i=0; i<5; i++) {
      		// step through 5 command data structures
      			for (j=0; j<5; j++)
        		// Command data structure 4-byte millisec timestamp
        			swap4((char *)&(data->errorlog_struct[i].commands[j].timestamp));
      			// Error data structure life timestamp
      			swap2((char *)&(data->errorlog_struct[i].error_struct.timestamp));
    		}
  	}
  
  	return 0;
}

// Read Extended Comprehensive Error Log
bool
ataReadExtErrorLog(ds_ata_info_t *aip, ata_smart_exterrlog_t *log, unsigned nsectors)
{
	if (!ataReadLogExt(aip, 0x03, 0x00, 0, log, nsectors))
    		return false;

  	check_multi_sector_sum(log, nsectors, "SMART Extended Comprehensive Error Log Structure");

  	if (isbigendian()) {
    		swapx2(&log->device_error_count);
    		swapx2(&log->error_log_index);

		unsigned i;
    		for (i = 0; i < nsectors; i++) {
			unsigned j;
      			for (j = 0; j < 4; j++)
        			swapx4(&log->error_logs[i].commands[j].timestamp);
      			swapx2(&log->error_logs[i].error.timestamp);
    		}
  	}

  	return true;
}


int
ataReadSmartThresholds(ds_ata_info_t *aip, struct ata_smart_thresholds_pvt *data)
{  
	// get data from device
  	if (smartcommandhandler(aip->ai_dsp->ds_fd, READ_THRESHOLDS, 0, (char *)data)) {
		syserror("Error SMART Thresholds Read failed");
    		return -1;
  	}
  
  	// compute its checksum, and issue a warning if needed
  	if (checksum(data))
    		checksumwarning("SMART Attribute Thresholds Structure");
  
  	// swap endian order if needed
  	if (isbigendian())
    		swap2((char *)&(data->revnumber));

 	return 0;
}

int
ataEnableSmart(ds_ata_info_t *aip)
{
	if (smartcommandhandler(aip->ai_dsp->ds_fd, ENABLE, 0, NULL)) {
		syserror("Error SMART Enable failed");
    		return -1;
  	}
  	return 0;
}

// If SMART is enabled, supported, and working, then this call is
// guaranteed to return 1, else zero.  Note that it should return 1
// regardless of whether the disk's SMART status is 'healthy' or
// 'failing'.
int
ataDoesSmartWork(ds_ata_info_t *aip)
{
	int retval = smartcommandhandler(aip->ai_dsp->ds_fd, STATUS, 0, NULL);

  	if (-1 == retval)
    		return 0;

  	return 1;
}

// This function tells you both about the ATA error log and the
// self-test error log capability (introduced in ATA-5).  The bit is
// poorly documented in the ATA/ATAPI standard.  Starting with ATA-6,
// SMART error logging is also indicated in bit 0 of DEVICE IDENTIFY
// word 84 and 87.  Top two bits must match the pattern 01. BEFORE
// ATA-6 these top two bits still had to match the pattern 01, but the
// remaining bits were reserved (==0).
int
isSmartErrorLogCapable(const ata_smart_values_t *data, const ata_identify_device_t *identity)
{
	unsigned short word84 = identity->command_set_extension;
  	unsigned short word87 = identity->csf_default;
  	int isata6 = identity->major_rev_num & (0x01<<6);
  	int isata7 = identity->major_rev_num & (0x01<<7);

  	if ((isata6 || isata7) && (word84>>14) == 0x01 && (word84 & 0x01))
    		return 1;
  
  	if ((isata6 || isata7) && (word87>>14) == 0x01 && (word87 & 0x01))
    		return 1;
  
  	// otherwise we'll use the poorly documented capability bit
  	return data->errorlog_capability & 0x01;
}

// See previous function.  If the error log exists then the self-test
// log should (must?) also exist.
int
isSmartTestLogCapable(const ata_smart_values_t *data, const ata_identify_device_t *identity)
{
	unsigned short word84 = identity->command_set_extension;
  	unsigned short word87 = identity->csf_default;
  	int isata6 = identity->major_rev_num & (0x01<<6);
  	int isata7 = identity->major_rev_num & (0x01<<7);

  	if ((isata6 || isata7) && (word84>>14) == 0x01 && (word84 & 0x02))
  		return 1;
  
  	if ((isata6 || isata7) && (word87>>14) == 0x01 && (word87 & 0x02))
    		return 1;

  	// otherwise we'll use the poorly documented capability bit
  	return data->errorlog_capability & 0x01;
}

int
isGeneralPurposeLoggingCapable(const ata_identify_device_t *identity)
{
	unsigned short word84 = identity->command_set_extension;
 	unsigned short word87 = identity->csf_default;

	// If bit 14 of word 84 is set to one and bit 15 of word 84 is
  	// cleared to zero, the contents of word 84 contains valid support
  	// information. If not, support information is not valid in this
  	// word.
  	if ((word84 >> 14) == 0x01)
    	// If bit 5 of word 84 is set to one, the device supports the
    	// General Purpose Logging feature set.
    		return (word84 & (0x01 << 5));
  
  	// If bit 14 of word 87 is set to one and bit 15 of word 87 is
  	// cleared to zero, the contents of words (87:85) contain valid
  	// information. If not, information is not valid in these words.  
  	if ((word87 >> 14) == 0x01)
    	// If bit 5 of word 87 is set to one, the device supports
    	// the General Purpose Logging feature set.
    		return (word87 & (0x01 << 5));

  	// not capable
  	return 0;
}

int
isSupportSelfTest (const ata_smart_values_t *data)
{
	return data->offline_data_collection_capability & 0x10;
}

// Read SCT Status
int
ataReadSCTStatus(ds_ata_info_t *aip, ata_sct_status_response_t *sts)
{
	// read SCT status via SMART log 0xe0
  	memset(sts, 0, sizeof(*sts));
  	if (smartcommandhandler(aip->ai_dsp->ds_fd, READ_LOG, 0xe0, (char *)sts)) {
		syserror("Error Read SCT Status failed");
    		return -1;
  	}

  	// swap endian order if needed
  	if (isbigendian()) {
    		swapx2(&sts->format_version);
    		swapx2(&sts->sct_version);
    		swapx2(&sts->sct_spec);
    		swapx2(&sts->ext_status_code);
    		swapx2(&sts->action_code);
    		swapx2(&sts->function_code);
    		swapx4(&sts->over_limit_count);
    		swapx4(&sts->under_limit_count);
  	}

  	// Check format version
  	if (!(sts->format_version == 2 || sts->format_version == 3)) {
    		printf("Error unknown SCT Status format version %u, should be 2 or 3.\n", sts->format_version);
    		return -1;
  	}
  	return 0;
}

// Read SCT Temperature History Table and Status
int
ataReadSCTTempHist(ds_ata_info_t *aip, ata_sct_temperature_history_table_t *tmh,
                       ata_sct_status_response_t *sts)
{
	// Check initial status
  	if (ataReadSCTStatus(aip, sts))
    		return -1;

  	// Do nothing if other SCT command is executing
  	if (sts->ext_status_code == 0xffff) {
    		printf("Another SCT command is executing, abort Read Data Table\n"
         		"(SCT ext_status_code 0x%04x, action_code=%u, function_code=%u)\n",
      			sts->ext_status_code, sts->action_code, sts->function_code);
    		return -1;
  	}

  	ata_sct_data_table_command_t cmd;
	memset(&cmd, 0, sizeof(cmd));
  	// CAUTION: DO NOT CHANGE THIS VALUE (SOME ACTION CODES MAY ERASE DISK)
  	cmd.action_code   = 5; // Data table command
  	cmd.function_code = 1; // Read table
  	cmd.table_id      = 2; // Temperature History Table

  	// write command via SMART log page 0xe0
  	if (smartcommandhandler(aip->ai_dsp->ds_fd, WRITE_LOG, 0xe0, (char *)&cmd)) {
    		syserror("Error Write SCT Data Table command failed");
    		return -1;
  	}

  	// read SCT data via SMART log page 0xe1
  	memset(tmh, 0, sizeof(*tmh));
  	if (smartcommandhandler(aip->ai_dsp->ds_fd, READ_LOG, 0xe1, (char *)tmh)) {
    		syserror("Error Read SCT Data Table failed");
    		return -1;
  	}

  	// re-read and check SCT status
  	if (ataReadSCTStatus(aip, sts))
    		return -1;

/**** BUG!!! Fix me!!!****/
//      if (!(sts->ext_status_code == 0 && sts->action_code == 5 && sts->function_code == 1)) {
/*************************/

  	if (!(sts->ext_status_code == 5 && sts->action_code == 1 && sts->function_code == 0)) {
    		printf("Error unexcepted SCT status 0x%04x (action_code=%u, function_code=%u)\n",
      			sts->ext_status_code, sts->action_code, sts->function_code);
    		return -1;
  	}

  	// swap endian order if needed
  	if (isbigendian()) {
    		swapx2(&tmh->format_version);
    		swapx2(&tmh->sampling_period);
    		swapx2(&tmh->interval);
  	}

  	// Check format version
  	if (tmh->format_version != 2) {
    		printf("Error unknown SCT Temperature History Format Version (%u),\
			 should be 2.\n", tmh->format_version);
    		return -1;
  	}
  	return 0;
}

// Print one self-test log entry.
// Returns true if self-test showed an error.
bool
ataPrintSmartSelfTestEntry(unsigned testnum, unsigned char test_type,
                           unsigned char test_status,
                           unsigned short timestamp,
                           uint64_t failing_lba,
                           bool print_error_only, bool print_header)
{
	const char *msgtest;
	switch (test_type) {
    		case 0x00: msgtest = "Offline";            break;
    		case 0x01: msgtest = "Short offline";      break;
    		case 0x02: msgtest = "Extended offline";   break;
    		case 0x03: msgtest = "Conveyance offline"; break;
    		case 0x04: msgtest = "Selective offline";  break;
    		case 0x7f: msgtest = "Abort offline test"; break;
    		case 0x81: msgtest = "Short captive";      break;
    		case 0x82: msgtest = "Extended captive";   break;
    		case 0x83: msgtest = "Conveyance captive"; break;
    		case 0x84: msgtest = "Selective captive";  break;
    		default:
      			if ((0x40 <= test_type && test_type <= 0x7e) || 0x90 <= test_type)
        			msgtest = "Vendor offline";
      			else
        			msgtest = "Reserved offline";
  	}

  	bool is_error = false;
  	const char *msgstat;
  	switch (test_status >> 4) {
    		case 0x0: msgstat = "Completed without error";       break;
    		case 0x1: msgstat = "Aborted by host";               break;
    		case 0x2: msgstat = "Interrupted (host reset)";      break;
    		case 0x3: msgstat = "Fatal or unknown error";        is_error = true; break;
    		case 0x4: msgstat = "Completed: unknown failure";    is_error = true; break;
    		case 0x5: msgstat = "Completed: electrical failure"; is_error = true; break;
    		case 0x6: msgstat = "Completed: servo/seek failure"; is_error = true; break;
    		case 0x7: msgstat = "Completed: read failure";       is_error = true; break;
    		case 0x8: msgstat = "Completed: handling damage??";  is_error = true; break;
    		case 0xf: msgstat = "Self-test routine in progress"; break;
    		default:  msgstat = "Unknown/reserved test status";
  	}

  	if (!is_error && print_error_only)
    		return false;

  	// Print header once
  	if (print_header) {
    		print_header = false;
    		printf("Num  Test_Description    Status                 \
			 Remaining  LifeTime(hours)  LBA_of_first_error\n");
  	}

  	char msglba[32];
  	if (is_error && failing_lba < 0xffffffffffffULL)
    		snprintf(msglba, sizeof(msglba), "%"PRIu64, failing_lba);
  	else
    		strcpy(msglba, "-");

  	printf("#%2u  %-19s %-29s %1d0%%  %8u         %s\n", testnum, msgtest, msgstat,
       		test_status &0x0f, timestamp, msglba);

  	return is_error;
}

// Print Smart self-test log, used by smartctl and smartd.
// return value is:
// bottom 8 bits: number of entries found where self-test showed an error
// remaining bits: if nonzero, power on hours of last self-test where error was found
int
ataPrintSmartSelfTestlog(const ata_smart_selftestlog_t *data, bool allentries,
                             unsigned char fix_firmwarebug)
{
	if (allentries)
    		printf("SMART Self-test log structure revision number %d\n",(int)data->revnumber);
  	if ((data->revnumber!=0x0001) && allentries && fix_firmwarebug != FIX_SAMSUNG)
    		printf("Warning: ATA Specification requires self-test log structure revision number = 1\n");
  	if (data->mostrecenttest==0) {
    		if (allentries)
      			printf("No self-tests have been logged.  [To run self-tests, use: smartctl -t]\n\n");
    		return 0;
  	}

  	bool noheaderprinted = true;
  	int retval = 0, hours = 0, testno = 0;

  	// print log
	int i;
  	for (i = 20; i >= 0; i--) {
    	// log is a circular buffer
    		int j = (i+data->mostrecenttest)%21;
    		const ata_smart_selftestlog_struct_t *log = data->selftest_struct+j;

    		if (nonempty(log, sizeof(*log))) {
      		// count entry based on non-empty structures -- needed for
      		// Seagate only -- other vendors don't have blank entries 'in
      		// the middle'
      			testno++;

      		// T13/1321D revision 1c: (Data structure Rev #1)

      		//The failing LBA shall be the LBA of the uncorrectable sector
      		//that caused the test to fail. If the device encountered more
      		//than one uncorrectable sector during the test, this field
      		//shall indicate the LBA of the first uncorrectable sector
      		//encountered. If the test passed or the test failed for some
      		//reason other than an uncorrectable sector, the value of this
      		//field is undefined.

      			// This is true in ALL ATA-5 specs
      			uint64_t lba48 = (log->lbafirstfailure < 0xffffffff ? log->lbafirstfailure : 0xffffffffffffULL);

      			// Print entry
      			bool errorfound = ataPrintSmartSelfTestEntry(testno,
        			log->selftestnumber, log->selfteststatus, log->timestamp,
        			lba48, !allentries, noheaderprinted);

      			// keep track of time of most recent error
      			if (errorfound && !hours)
        			hours=log->timestamp;
    		}
  	}
  	if (!allentries && retval)
    		printf("\n");

  	hours = (hours << 8);
  	return (retval | hours);
}

