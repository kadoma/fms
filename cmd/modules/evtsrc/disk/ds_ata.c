/*
 * ds_ata.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 * 	ds_ata.c
 */

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <protocol.h>
#include "fm_ata.h"
#include "ds_ata.h"
#include "ds_ata_ioctl.h"

#define	ata_set_errno(aip, errno)	(ds_set_errno((aip)->ai_dsp, (errno)))

/*
 * Table to validate log pages
 */
typedef int (*logpage_analyze_fn_t)(ds_ata_info_t *);

typedef struct logpage_validation_entry {
	uint8_t			ve_code;
	const char		*ve_desc;
	logpage_analyze_fn_t	ve_analyze;
} logpage_validation_entry_t;

//static int logpage_smart_analyze(ds_ata_info_t *);
static int logpage_temp_analyze(ds_ata_info_t *);
static int logpage_selftest_analyze(ds_ata_info_t *);
static int logpage_error_analyze(ds_ata_info_t *);
static int logpage_sataphy_analyze(ds_ata_info_t *);

static struct logpage_validation_entry log_validation[] = {
	{ LOGPAGE_ATA_TEMP, "temperature", logpage_temp_analyze },
	{ LOGPAGE_ATA_SELFTEST, "self-test", logpage_selftest_analyze },
	{ LOGPAGE_ATA_ERROR, "error", logpage_error_analyze },
	{ LOGPAGE_ATA_SATAPHY, "sata-phy-event-counters", logpage_sataphy_analyze }
};

#define	NLOG_VALIDATION	(sizeof (log_validation) / sizeof (log_validation[0]))

///////////////////////////////////////////////////////////////////////////////

/* For the given Command Register (CR) and Features Register (FR), attempts
 * to construct a string that describes the contents of the Status
 * Register (ST) and Error Register (ER).  The caller passes the string
 * buffer and the return value is a pointer to this string.  If the
 * meanings of the flags of the error register are not known for the given
 * command then it returns NULL.
 *
 * The meanings of the flags of the error register for all commands are
 * described in the ATA spec and could all be supported here in theory.
 * Currently, only a few commands are supported (those that have been seen
 * to produce errors).  If many more are to be added then this function
 * should probably be redesigned.
 */

static const char *
construct_st_er_desc(char *s, unsigned char CR, unsigned char FR,
		     unsigned char ST, unsigned char ER, unsigned short SC,
		     const ata_smart_errorlog_error_struct_t *lba28_regs,
		     const ata_smart_exterrlog_error_t *lba48_regs)
{
	const char *error_flag[8];
  	int i, print_lba=0, print_sector=0;

  	// Set of character strings corresponding to different error codes.
  	// Please keep in alphabetic order if you add more.
  	const char *abrt  = "ABRT";  // ABORTED
 	const char *amnf  = "AMNF";  // ADDRESS MARK NOT FOUND
 	const char *ccto  = "CCTO";  // COMMAND COMPLETION TIMED OUT
 	const char *eom   = "EOM";   // END OF MEDIA
 	const char *icrc  = "ICRC";  // INTERFACE CRC ERROR
 	const char *idnf  = "IDNF";  // ID NOT FOUND
 	const char *ili   = "ILI";   // MEANING OF THIS BIT IS COMMAND-SET SPECIFIC
 	const char *mc    = "MC";    // MEDIA CHANGED 
 	const char *mcr   = "MCR";   // MEDIA CHANGE REQUEST
 	const char *nm    = "NM";    // NO MEDIA
 	const char *obs   = "obs";   // OBSOLETE
 	const char *tk0nf = "TK0NF"; // TRACK 0 NOT FOUND
 	const char *unc   = "UNC";   // UNCORRECTABLE
 	const char *wp    = "WP";    // WRITE PROTECTED

  	/* If for any command the Device Fault flag of the status register is
   	 * not used then used_device_fault should be set to 0 (in the CR switch
   	 * below)
   	 */
  	int uses_device_fault = 1;

  	/* A value of NULL means that the error flag isn't used */
  	for (i = 0; i < 8; i++)
    		error_flag[i] = NULL;

  	switch (CR) {
  		case 0x10:  // RECALIBRATE
    			error_flag[2] = abrt;
    			error_flag[1] = tk0nf;
    			break;
  		case 0x20:  /* READ SECTOR(S) */
  		case 0x21:  // READ SECTOR(S)
  		case 0x24:  // READ SECTOR(S) EXT
  		case 0xC4:  /* READ MULTIPLE */
  		case 0x29:  // READ MULTIPLE EXT
    			error_flag[6] = unc;
    			error_flag[5] = mc;
    			error_flag[4] = idnf;
    			error_flag[3] = mcr;
    			error_flag[2] = abrt;
    			error_flag[1] = nm;
    			error_flag[0] = amnf;
    			print_lba=1;
    			break;
  		case 0x22:  // READ LONG (with retries)
  		case 0x23:  // READ LONG (without retries)
    			error_flag[4] = idnf;
    			error_flag[2] = abrt;
    			error_flag[0] = amnf;
    			print_lba=1;
    			break;
  		case 0x2a:  // READ STREAM DMA
  		case 0x2b:  // READ STREAM PIO
    			if (CR==0x2a)
      				error_flag[7] = icrc;
    			error_flag[6] = unc;
    			error_flag[5] = mc;
    			error_flag[4] = idnf;
    			error_flag[3] = mcr;
    			error_flag[2] = abrt;
    			error_flag[1] = nm;
    			error_flag[0] = ccto;
    			print_lba=1;
    			print_sector=SC;
    			break;
  		case 0x3A:  // WRITE STREAM DMA
  		case 0x3B:  // WRITE STREAM PIO
    			if (CR==0x3A)
      				error_flag[7] = icrc;
    			error_flag[6] = wp;
    			error_flag[5] = mc;
    			error_flag[4] = idnf;
    			error_flag[3] = mcr;
    			error_flag[2] = abrt;
    			error_flag[1] = nm;
    			error_flag[0] = ccto;
    			print_lba=1;
    			print_sector=SC;
    			break;
  		case 0x25:  /* READ DMA EXT */
  		case 0x26:  // READ DMA QUEUED EXT
  		case 0xC7:  // READ DMA QUEUED
  		case 0xC8:  /* READ DMA */
  		case 0xC9:
    			error_flag[7] = icrc;
    			error_flag[6] = unc;
    			error_flag[5] = mc;
    			error_flag[4] = idnf;
    			error_flag[3] = mcr;
    			error_flag[2] = abrt;
    			error_flag[1] = nm;
    			error_flag[0] = amnf;
    			print_lba=1;
    			if (CR==0x25 || CR==0xC8)
      				print_sector=SC;
    			break;
  		case 0x30:  /* WRITE SECTOR(S) */
  		case 0x31:  // WRITE SECTOR(S)
  		case 0x34:  // WRITE SECTOR(S) EXT
  		case 0xC5:  /* WRITE MULTIPLE */
  		case 0x39:  // WRITE MULTIPLE EXT
  		case 0xCE:  // WRITE MULTIPLE FUA EXT
    			error_flag[6] = wp;
    			error_flag[5] = mc;
    			error_flag[4] = idnf;
    			error_flag[3] = mcr;
    			error_flag[2] = abrt;
    			error_flag[1] = nm;
    			print_lba=1;
    			break;
  		case 0x32:  // WRITE LONG (with retries)
  		case 0x33:  // WRITE LONG (without retries)
    			error_flag[4] = idnf;
    			error_flag[2] = abrt;
    			print_lba=1;
    			break;
  		case 0x3C:  // WRITE VERIFY
    			error_flag[6] = unc;
    			error_flag[4] = idnf;
    			error_flag[2] = abrt;
    			error_flag[0] = amnf;
    			print_lba=1;
    			break;
  		case 0x40: // READ VERIFY SECTOR(S) with retries
  		case 0x41: // READ VERIFY SECTOR(S) without retries
  		case 0x42: // READ VERIFY SECTOR(S) EXT
    			error_flag[6] = unc;
    			error_flag[5] = mc;
    			error_flag[4] = idnf;
    			error_flag[3] = mcr;
    			error_flag[2] = abrt;
    			error_flag[1] = nm;
    			error_flag[0] = amnf;
    			print_lba=1;
    			break;
  		case 0xA0:  /* PACKET */
    			/* Bits 4-7 are all used for sense key (a 'command packet set specific error
     			 * indication' according to the ATA/ATAPI-7 standard), so "Sense key" will
     			 * be repeated in the error description string if more than one of those
     			 * bits is set.
     			 */
    			error_flag[7] = "Sense key (bit 3)",
    			error_flag[6] = "Sense key (bit 2)",
    			error_flag[5] = "Sense key (bit 1)",
    			error_flag[4] = "Sense key (bit 0)",
    			error_flag[2] = abrt;
    			error_flag[1] = eom;
    			error_flag[0] = ili;
    			break;
  		case 0xA1:  /* IDENTIFY PACKET DEVICE */
  		case 0xEF:  /* SET FEATURES */
  		case 0x00:  /* NOP */
  		case 0xC6:  /* SET MULTIPLE MODE */
    			error_flag[2] = abrt;
    			break;
  		case 0x2F:  // READ LOG EXT
    			error_flag[6] = unc;
    			error_flag[4] = idnf;
    			error_flag[2] = abrt;
    			error_flag[0] = obs;
    			break;
  		case 0x3F:  // WRITE LOG EXT
    			error_flag[4] = idnf;
    			error_flag[2] = abrt;
    			error_flag[0] = obs;
    			break;
  		case 0xB0:  /* SMART */
    			switch(FR) {
    				case 0xD0:  // SMART READ DATA
    				case 0xD1:  // SMART READ ATTRIBUTE THRESHOLDS
    				case 0xD5:  /* SMART READ LOG */
      					error_flag[6] = unc;
      					error_flag[4] = idnf;
      					error_flag[2] = abrt;
      					error_flag[0] = obs;
      					break;
    				case 0xD6:  /* SMART WRITE LOG */
      					error_flag[4] = idnf;
      					error_flag[2] = abrt;
      					error_flag[0] = obs;
      					break;
    				case 0xD2:  // Enable/Disable Attribute Autosave
    				case 0xD3:  // SMART SAVE ATTRIBUTE VALUES (ATA-3)
    				case 0xD8:  // SMART ENABLE OPERATIONS
    				case 0xD9:  /* SMART DISABLE OPERATIONS */
    				case 0xDA:  /* SMART RETURN STATUS */
    				case 0xDB:  // Enable/Disable Auto Offline (SFF)
      					error_flag[2] = abrt;
      					break;
    				case 0xD4:  // SMART EXECUTE IMMEDIATE OFFLINE
      					error_flag[4] = idnf;
      					error_flag[2] = abrt;
      					break;
    				default:
      					return NULL;
      				break;
    			}
    			break;
  		case 0xB1:  /* DEVICE CONFIGURATION */
    			switch (FR) {
    				case 0xC0:  /* DEVICE CONFIGURATION RESTORE */
      					error_flag[2] = abrt;
      					break;
    				default:
      					return NULL;
      				break;
    			}
    			break;
  		case 0xCA:  /* WRITE DMA */
  		case 0xCB:
  		case 0x35:  // WRITE DMA EXT
  		case 0x3D:  // WRITE DMA FUA EXT
  		case 0xCC:  // WRITE DMA QUEUED
  		case 0x36:  // WRITE DMA QUEUED EXT
  		case 0x3E:  // WRITE DMA QUEUED FUA EXT
    			error_flag[7] = icrc;
    			error_flag[6] = wp;
    			error_flag[5] = mc;
    			error_flag[4] = idnf;
    			error_flag[3] = mcr;
    			error_flag[2] = abrt;
    			error_flag[1] = nm;
    			error_flag[0] = amnf;
    			print_lba=1;
    			if (CR==0x35)
      				print_sector= SC;
    			break;
  		case 0xE4: // READ BUFFER
  		case 0xE8: // WRITE BUFFER
    			error_flag[2] = abrt;
    			break;
  		default:
    			return NULL;
  	}

  	s[0] = '\0';

  	/* We ignore any status flags other than Device Fault and Error */

  	if (uses_device_fault && (ST & (1 << 5))) {
    		strcat(s, "Device Fault");
    		if (ST & 1)  // Error flag
      			strcat(s, "; ");
  	}
  	if (ST & 1) {  // Error flag
    		int count = 0;

    		strcat(s, "Error: ");
    		for (i = 7; i >= 0; i--)
      			if ((ER & (1 << i)) && (error_flag[i])) {
        			if (count++ > 0)
           				strcat(s, ", ");
        			strcat(s, error_flag[i]);
      			}
  	}

  	// If the error was a READ or WRITE error, print the Logical Block
  	// Address (LBA) at which the read or write failed.
  	if (print_lba) {
    		char tmp[128];
    		// print number of sectors, if known, and append to print string
    		if (print_sector) {
      			snprintf(tmp, 128, " %d sectors", print_sector);
      			strcat(s, tmp);
    		}

    		if (lba28_regs) {
      			unsigned lba;
      			// bits 24-27: bits 0-3 of DH
      			lba   = 0xf & lba28_regs->drive_head;
      			lba <<= 8;
      			// bits 16-23: CH
      			lba  |= lba28_regs->cylinder_high;
      			lba <<= 8;
      			// bits 8-15:  CL
      			lba  |= lba28_regs->cylinder_low;
      			lba <<= 8;
      			// bits 0-7:   SN
      			lba  |= lba28_regs->sector_number;
      			snprintf(tmp, 128, " at LBA = 0x%08x = %u", lba, lba);
      			strcat(s, tmp);
    		} else if (lba48_regs) {
      			// This assumes that upper LBA registers are 0 for 28-bit commands
      			// (TODO: detect 48-bit commands above)
      			uint64_t lba48;
      			lba48   = lba48_regs->lba_high_register_hi;
      			lba48 <<= 8;
      			lba48  |= lba48_regs->lba_mid_register_hi;
      			lba48 <<= 8;
      			lba48  |= lba48_regs->lba_low_register_hi;
      			lba48  |= lba48_regs->device_register & 0xf;
      			lba48 <<= 8;
      			lba48  |= lba48_regs->lba_high_register;
      			lba48 <<= 8;
      			lba48  |= lba48_regs->lba_mid_register;
      			lba48 <<= 8;
      			lba48  |= lba48_regs->lba_low_register;
      			snprintf(tmp, 128, " at LBA = 0x%08"PRIx64" = %"PRIu64, lba48, lba48);
      			strcat(s, tmp);
    		}
  	}

  	return s;
}

static inline const char *
construct_st_errorlog_er_desc(char *s, const ata_smart_errorlog_struct_t *data)
{
	return construct_st_er_desc(s,
    		data->commands[4].commandreg,
    		data->commands[4].featuresreg,
    		data->error_struct.status,
    		data->error_struct.error_register,
    		data->error_struct.sector_count,
    		&data->error_struct, (const ata_smart_exterrlog_error_t *)0);
}

static inline const char *
construct_st_exterrlog_er_desc(char *s, const ata_smart_exterrlog_error_log_t *data)
{
	return construct_st_er_desc(s,
    		data->commands[4].command_register,
    		data->commands[4].features_register,
    		data->error.status_register,
    		data->error.error_register,
    		data->error.count_register_hi << 8 | data->error.count_register,
    		(const ata_smart_errorlog_error_struct_t *)0, &data->error);
}

// Get # sectors of a log addr, 0 if log does not exist.
static unsigned
GetNumLogSectors(const ata_smart_log_directory_t *logdir, unsigned logaddr, bool gpl)
{
        if (!logdir)
                return 0;
        if (logaddr > 0xff)
                return 0;
        if (logaddr == 0)
                return 1;
        unsigned n = logdir->entry[logaddr-1].numsectors;
        if (gpl)
        // GP logs may have >255 sectors
        n |= logdir->entry[logaddr-1].reserved << 8;
        return n;
}

// Print log 0x11
static void
PrintSataPhyEventCounters(const unsigned char *data, bool reset)
{
	if (checksum(data))
    		checksumwarning("SATA Phy Event Counters");
  	printf("SATA Phy Event Counters (GP Log 0x11)\n");
  	if (data[0] || data[1] || data[2] || data[3])
    		printf("[Reserved: 0x%02x 0x%02x 0x%02x 0x%02x]\n",
    			data[0], data[1], data[2], data[3]);
  		printf("ID      Size     Value  Description\n");

	unsigned i;
  	for (i = 4; ; ) {
    		// Get counter id and size (bits 14:12)
    		unsigned id = data[i] | (data[i+1] << 8);
    		unsigned size = ((id >> 12) & 0x7) << 1;
    		id &= 0x8fff;

    		// End of counter table ?
    		if (!id)
      			break;
    		i += 2;

    		if (!(2 <= size && size <= 8 && i + size < 512)) {
      			printf("0x%04x  %u: Invalid entry\n", id, size);
      			break;
    		}

    		// Get value
   		uint64_t val = 0, max_val = 0;
		unsigned j;
    		for (j = 0; j < size; j+=2) {
        		val |= (uint64_t)(data[i+j] | (data[i+j+1] << 8)) << (j*8);
        		max_val |= (uint64_t)0xffffU << (j*8);
    		}
    		i += size;

    		// Get name
    		const char *name;
    		switch (id) {
      			case 0x001: name = "Command failed due to ICRC error"; break; // Mandatory
      			case 0x002: name = "R_ERR response for data FIS"; break;
      			case 0x003: name = "R_ERR response for device-to-host data FIS"; break;
      			case 0x004: name = "R_ERR response for host-to-device data FIS"; break;
      			case 0x005: name = "R_ERR response for non-data FIS"; break;
      			case 0x006: name = "R_ERR response for device-to-host non-data FIS"; break;
      			case 0x007: name = "R_ERR response for host-to-device non-data FIS"; break;
      			case 0x008: name = "Device-to-host non-data FIS retries"; break;
      			case 0x009: name = "Transition from drive PhyRdy to drive PhyNRdy"; break;
      			case 0x00A: name = "Device-to-host register FISes sent due to a COMRESET"; break; // Mandatory
      			case 0x00B: name = "CRC errors within host-to-device FIS"; break;
      			case 0x00D: name = "Non-CRC errors within host-to-device FIS"; break;
      			case 0x00F: name = "R_ERR response for host-to-device data FIS, CRC"; break;
      			case 0x010: name = "R_ERR response for host-to-device data FIS, non-CRC"; break;
      			case 0x012: name = "R_ERR response for host-to-device non-data FIS, CRC"; break;
      			case 0x013: name = "R_ERR response for host-to-device non-data FIS, non-CRC"; break;
      			default:    name = (id & 0x8000 ? "Vendor specific" : "Unknown"); break;
    		}

    		// Counters stop at max value, add '+' in this case
    		printf("0x%04x  %u %12"PRIu64"%c %s\n", id, size, val,
      			(val == max_val ? '+' : ' '), name);
  	}
  	if (reset)
    		printf("All counters reset\n");
  	printf("\n");
}

// Get description for 'state' value from SMART Error Logs
static const char *
get_error_log_state_desc(unsigned state)
{
	state &= 0x0f;
  	switch (state){
    		case 0x0: return "in an unknown state";
    		case 0x1: return "sleeping";
    		case 0x2: return "in standby mode";
    		case 0x3: return "active or idle";
    		case 0x4: return "doing SMART Offline or Self-test";
  		default:
    			return (state < 0xb ? "in a reserved state"
                        	: "in a vendor specific state");
  	}
}

// returns number of errors
static int
PrintSmartErrorlog(const ata_smart_errorlog_t *data,
                   unsigned char fix_firmwarebug)
{
	printf("SMART Error Log Version: %d\n", (int)data->revnumber);
  
  	// if no errors logged, return
  	if (!data->error_log_pointer) {
    		printf("No Errors Logged\n\n");
    		return 0;
  	}
  	// If log pointer out of range, return
  	if (data->error_log_pointer > 5) {
    		printf("Invalid Error Log index = 0x%02x (T13/1321D rev 1c "
         	       "Section 8.41.6.8.2.2 gives valid range from 1 to 5)\n\n",
         	       (int)data->error_log_pointer);
    		return 0;
  	}

  	// Some internal consistency checking of the data structures
  	if ((data->ata_error_count-data->error_log_pointer)%5 && fix_firmwarebug != FIX_SAMSUNG2) {
    		printf("Warning: ATA error count %d inconsistent with error log pointer %d\n\n",
         		data->ata_error_count, data->error_log_pointer);
  	}
  
  	// starting printing error log info
  	if (data->ata_error_count<=5)
    		printf( "ATA Error Count: %d\n", (int)data->ata_error_count);
  	else
    		printf( "ATA Error Count: %d (device log contains only the most recent five errors)\n",
           		(int)data->ata_error_count);
  	printf( "\tCR = Command Register [HEX]\n"
       		"\tFR = Features Register [HEX]\n"
       		"\tSC = Sector Count Register [HEX]\n"
       		"\tSN = Sector Number Register [HEX]\n"
       		"\tCL = Cylinder Low Register [HEX]\n"
       		"\tCH = Cylinder High Register [HEX]\n"
       		"\tDH = Device/Head Register [HEX]\n"
       		"\tDC = Device Command Register [HEX]\n"
       		"\tER = Error register [HEX]\n"
       		"\tST = Status register [HEX]\n"
       		"Powered_Up_Time is measured from power on, and printed as\n"
       		"DDd+hh:mm:SS.sss where DD=days, hh=hours, mm=minutes,\n"
       		"SS=sec, and sss=millisec. It \"wraps\" after 49.710 days.\n\n" );
  
  	// now step through the five error log data structures (table 39 of spec)
	int k;
 	for (k = 4; k >= 0; k-- ) {

    		// The error log data structure entries are a circular buffer
    		int j, i = (data->error_log_pointer + k) % 5;
    		const ata_smart_errorlog_struct_t *elog = data->errorlog_struct + i;
    		const ata_smart_errorlog_error_struct_t *summary = &(elog->error_struct);

		// Spec says: unused error log structures shall be zero filled
    		if (nonempty((unsigned char*)elog,sizeof(*elog))) {
      			// Table 57 of T13/1532D Volume 1 Revision 3
      			const char *msgstate = get_error_log_state_desc(summary->state);
      			int days = (int)summary->timestamp/24;

      			// See table 42 of ATA5 spec
      			printf("Error %d occurred at disk power-on lifetime: %d hours (%d days + %d hours)\n",
             			(int)(data->ata_error_count+k-4), (int)summary->timestamp, days, 
				(int)(summary->timestamp-24*days));
      			printf("  When the command that caused the error occurred, the device was %s.\n\n",msgstate);
      			printf("  After command completion occurred, registers were:\n"
           		       "  ER ST SC SN CL CH DH\n"
           		       "  -- -- -- -- -- -- --\n"
           		       "  %02x %02x %02x %02x %02x %02x %02x",
           			(int)summary->error_register,
           			(int)summary->status,
           			(int)summary->sector_count,
           			(int)summary->sector_number,
           			(int)summary->cylinder_low,
           			(int)summary->cylinder_high,
           			(int)summary->drive_head);
      			// Add a description of the contents of the status and error registers
      			// if possible
      			char descbuf[256];
      			const char *st_er_desc = construct_st_errorlog_er_desc(descbuf, elog);
      			if (st_er_desc)
        			printf("  %s", st_er_desc);
      			printf("\n\n");
      			printf("  Commands leading to the command that caused the error were:\n"
           		       "  CR FR SC SN CL CH DH DC   Powered_Up_Time  Command/Feature_Name\n"
           		       "  -- -- -- -- -- -- -- --  ----------------  --------------------\n");
      			for ( j = 4; j >= 0; j--) {
        			const ata_smart_errorlog_command_struct_t *thiscommand = elog->commands+j;

        			// Spec says: unused data command structures shall be zero filled
        			if (nonempty((unsigned char*)thiscommand,sizeof(*thiscommand))) {
	  				char timestring[32];
	  
	  				// Convert integer milliseconds to a text-format string
	  				MsecToText(thiscommand->timestamp, timestring);
	  
          				printf("  %02x %02x %02x %02x %02x %02x %02x %02x  %16s  %s\n",
               					(int)thiscommand->commandreg,
               					(int)thiscommand->featuresreg,
               					(int)thiscommand->sector_count,
               					(int)thiscommand->sector_number,
               					(int)thiscommand->cylinder_low,
               					(int)thiscommand->cylinder_high,
               					(int)thiscommand->drive_head,
               					(int)thiscommand->devicecontrolreg,
	       					timestring,
               					"look_up_ata_command(thiscommand->commandreg, thiscommand->featuresreg)");
				}
      			}
      			printf("\n");
    		}
  	}
    	printf("\n");

  	return data->ata_error_count;  
}

// Print SMART Extended Comprehensive Error Log (GP Log 0x03)
static int
PrintSmartExtErrorLog(const ata_smart_exterrlog_t *log,
                      unsigned nsectors, unsigned max_errors)
{
	printf("SMART Extended Comprehensive Error Log Version: %u (%u sectors)\n",
       		log->version, nsectors);

  	if (!log->device_error_count) {
    		printf("No Errors Logged\n\n");
    		return 0;
  	}

  	// Check index
  	unsigned nentries = nsectors * 4;
  	unsigned erridx = log->error_log_index;
  	if (!(1 <= erridx && erridx <= nentries)) {
    		// Some Samsung disks (at least SP1614C/SW100-25, HD300LJ/ZT100-12) use the
    		// former index from Summary Error Log (byte 1, now reserved) and set byte 2-3
    		// to 0.
    		if (!(erridx == 0 && 1 <= log->reserved1 && log->reserved1 <= nentries)) {
      			printf("Invalid Error Log index = 0x%04x (reserved = 0x%02x)\n", erridx, log->reserved1);
      			return 0;
    		}
    		printf("Invalid Error Log index = 0x%04x, trying reserved byte (0x%02x) instead\n", erridx, log->reserved1);
    		erridx = log->reserved1;
  	}

  	// Index base is not clearly specified by ATA8-ACS (T13/1699-D Revision 6a),
  	// it is 1-based in practice.
  	erridx--;

  	// Calculate #errors to print
  	unsigned errcnt = log->device_error_count;

  	if (errcnt <= nentries)
    		printf("Device Error Count: %u\n", log->device_error_count);
  	else {
    		errcnt = nentries;
    		printf("Device Error Count: %u (device log contains only the most recent %u errors)\n",
         		log->device_error_count, errcnt);
  	}

  	if (max_errors < errcnt)
    		errcnt = max_errors;

  	printf( "\tCR     = Command Register\n"
       		"\tFEATR  = Features Register\n"
       		"\tCOUNT  = Count (was: Sector Count) Register\n"
       		"\tLBA_48 = Upper bytes of LBA High/Mid/Low Registers ]  ATA-8\n"
       		"\tLH     = LBA High (was: Cylinder High) Register    ]   LBA\n"
       		"\tLM     = LBA Mid (was: Cylinder Low) Register      ] Register\n"
       		"\tLL     = LBA Low (was: Sector Number) Register     ]\n"
       		"\tDV     = Device (was: Device/Head) Register\n"
       		"\tDC     = Device Control Register\n"
       		"\tER     = Error register\n"
       		"\tST     = Status register\n"
       		"Powered_Up_Time is measured from power on, and printed as\n"
       		"DDd+hh:mm:SS.sss where DD=days, hh=hours, mm=minutes,\n"
       		"SS=sec, and sss=millisec. It \"wraps\" after 49.710 days.\n\n");

  	// Iterate through circular buffer in reverse direction
	unsigned i;
	unsigned errnum;
  	for (i = 0, errnum = log->device_error_count;
       		i < errcnt; i++, errnum--, erridx = (erridx > 0 ? erridx - 1 : nentries - 1)) {

    		const ata_smart_exterrlog_error_log_t *entry = &(log[erridx/4].error_logs[erridx%4]);

    		// Skip unused entries
    		if (!nonempty(entry, sizeof(entry))) {
      			printf("Error %u [%u] log entry is empty\n", errnum, erridx);
      			continue;
    		}

    		// Print error information
    		const ata_smart_exterrlog_error_t *err = &(entry->error);
    		printf("Error %u [%u] occurred at disk power-on lifetime: %u hours (%u days + %u hours)\n",
         		errnum, erridx, err->timestamp, err->timestamp / 24, err->timestamp % 24);

    		printf("  When the command that caused the error occurred, the device was %s.\n\n",
      			get_error_log_state_desc(err->state));

    		// Print registers
    		printf("  After command completion occurred, registers were:\n"
         	       "  ER -- ST COUNT  LBA_48  LH LM LL DV DC\n"
         	       "  -- -- -- == -- == == == -- -- -- -- --\n"
         	       "  %02x -- %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
         		err->error_register,
         		err->status_register,
         		err->count_register_hi,
         		err->count_register,
         		err->lba_high_register_hi,
         		err->lba_mid_register_hi,
         		err->lba_low_register_hi,
         		err->lba_high_register,
         		err->lba_mid_register,
         		err->lba_low_register,
         		err->device_register,
         		err->device_control_register);

    		// Add a description of the contents of the status and error registers
    		// if possible
    		char descbuf[256];
    		const char *st_er_desc = construct_st_exterrlog_er_desc(descbuf, entry);
    		if (st_er_desc)
      			printf("  %s", st_er_desc);
    		printf("\n\n");

    		// Print command history
    		printf("  Commands leading to the command that caused the error were:\n"
         	       "  CR FEATR COUNT  LBA_48  LH LM LL DV DC  Powered_Up_Time  Command/Feature_Name\n"
             	       "  -- == -- == -- == == == -- -- -- -- --  ---------------  --------------------\n");
		int ci;
    		for (ci = 4; ci >= 0; ci--) {
      			const ata_smart_exterrlog_command_t *cmd = &(entry->commands[ci]);

      			// Skip unused entries
      			if (!nonempty(cmd, sizeof(cmd)))
        			continue;

      			// Print registers, timestamp and ATA command name
      			char timestring[32];
      			MsecToText(cmd->timestamp, timestring);

      			printf("  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %16s  %s\n",
           			cmd->command_register,
           			cmd->features_register_hi,
           			cmd->features_register,
           			cmd->count_register_hi,
           			cmd->count_register,
           			cmd->lba_high_register_hi,
           			cmd->lba_mid_register_hi,
           			cmd->lba_low_register_hi,
           			cmd->lba_high_register,
           			cmd->lba_mid_register,
           			cmd->lba_low_register,
           			cmd->device_register,
           			cmd->device_control_register,
           			timestring,
           			"look_up_ata_command(cmd.command_register, cmd.features_register)");
    		}
    		printf("\n");
  	}
    	printf("\n");

  	return log->device_error_count;
}

// Print SMART Extended Self-test Log (GP Log 0x07)
static void
PrintSmartExtSelfTestLog(const ata_smart_extselftestlog_t *log,
			 unsigned nsectors, unsigned max_entries)
{
	printf("SMART Extended Self-test Log Version: %u (%u sectors)\n",
       			log->version, nsectors);

  	if (!log->log_desc_index) {
    		printf("No self-tests have been logged.  [To run self-tests, use: smartctl -t]\n\n");
    		return;
  	}

  	// Check index
  	unsigned nentries = nsectors * 19;
  	unsigned logidx = log->log_desc_index;
  	if (logidx > nentries) {
    		printf("Invalid Self-test Log index = 0x%04x (reserved = 0x%02x)\n", logidx, log->reserved1);
    		return;
  	}

  	// Index base is not clearly specified by ATA8-ACS (T13/1699-D Revision 6a),
  	// it is 1-based in practice.
  	logidx--;

  	bool print_header = true;

  	// Iterate through circular buffer in reverse direction
	unsigned i;
	unsigned testnum;
  	for (i = 0, testnum = 1;
       		i < nentries && testnum <= max_entries;
       		i++, logidx = (logidx > 0 ? logidx - 1 : nentries - 1)) {

    		const ata_smart_extselftestlog_desc_t *entry = &(log[logidx/19].log_descs[logidx%19]);

    		// Skip unused entries
    		if (!nonempty(entry, sizeof(entry)))
      			continue;

    		// Get LBA
    		const unsigned char *b = entry->failing_lba;
    		uint64_t lba48 = b[0]
        	    | (          b[1] <<  8)
        	    | (          b[2] << 16)
        	    | ((uint64_t)b[3] << 24)
        	    | ((uint64_t)b[4] << 32)
        	    | ((uint64_t)b[5] << 40);

    		// Print entry
    		ataPrintSmartSelfTestEntry(testnum++, entry->self_test_type,
      			entry->self_test_status, entry->timestamp, lba48,
      			false /*!print_error_only*/, print_header);
  	}
  	printf("\n");
}

// Format SCT Temperature value
static const char *
sct_ptemp(signed char x, char *buf)
{
	if (x == -128 /*0x80 = unknown*/)
		strcpy(buf, " ?");
	else
		sprintf(buf, "%2d", x);
	return buf;
}

static const char *
sct_pbar(int x, char *buf)
{
	if (x <= 19)
		x = 0;
	else    
		x -= 19;
	bool ov = false;
	if (x > 40) {
		x = 40; ov = true;
	}       
	if (x > 0) {
		memset(buf, '*', x);
		if (ov)
			buf[x-1] = '+';
		buf[x] = 0;
	} else {
		buf[0] = '-'; buf[1] = 0;
	}       
	return buf;
}       

static const char *
sct_device_state_msg(unsigned char state)
{
	switch (state) {
		case 0: return "Active";
		case 1: return "Stand-by";
		case 2: return "Sleep";
		case 3: return "DST executing in background";
		case 4: return "SMART Off-line Data Collection executing in background";
		case 5: return "SCT command executing in background";
		default:return "Unknown";
	}       
}     

static int
ataPrintSCTStatus(const ata_sct_status_response_t *sts)
{
        printf("SCT Status Version:                  %u\n", sts->format_version);
        printf("SCT Version (vendor specific):       %u (0x%04x)\n", sts->sct_version, sts->sct_version);
        printf("SCT Support Level:                   %u\n", sts->sct_spec);
        printf("Device State:                        %s (%u)\n",
        sct_device_state_msg(sts->device_state), sts->device_state);
        char buf1[20], buf2[20];
        if ( !sts->min_temp && !sts->life_min_temp && !sts->byte205
                && !sts->under_limit_count && !sts->over_limit_count ) {
                // "Reserved" fields not set, assume "old" format version 2
                // Table 11 of T13/1701DT Revision 5
                // Table 54 of T13/1699-D Revision 3e
                printf("Current Temperature:                 %s Celsius\n",
                        sct_ptemp(sts->hda_temp, buf1));
                printf("Power Cycle Max Temperature:         %s Celsius\n",
                        sct_ptemp(sts->max_temp, buf2));
                printf("Lifetime    Max Temperature:         %s Celsius\n",
                        sct_ptemp(sts->life_max_temp, buf2));
        } else {
        // Assume "new" format version 2 or version 3
        // T13/e06152r0-3 (Additional SCT Temperature Statistics)
        // Table 60 of T13/1699-D Revision 3f
                printf("Current Temperature:                    %s Celsius\n",
                        sct_ptemp(sts->hda_temp, buf1));
                printf("Power Cycle Min/Max Temperature:     %s/%s Celsius\n",
                        sct_ptemp(sts->min_temp, buf1), sct_ptemp(sts->max_temp, buf2));
                printf("Lifetime    Min/Max Temperature:     %s/%s Celsius\n",
                        sct_ptemp(sts->life_min_temp, buf1), sct_ptemp(sts->life_max_temp, buf2));
                if (sts->byte205) // e06152r0-2, removed in e06152r3
                        printf("Lifetime    Average Temperature:        %s Celsius\n",
                		sct_ptemp((signed char)sts->byte205, buf1));
                printf("Under/Over Temperature Limit Count:  %2u/%u\n",
                        sts->under_limit_count, sts->over_limit_count);
        }
        return 0;
}

// Print SCT Temperature History Table
static int
ataPrintSCTTempHist(const ata_sct_temperature_history_table_t *tmh)
{
        char buf1[20], buf2[80];
        printf("SCT Temperature History Version:     %u\n", tmh->format_version);
        printf("Temperature Sampling Period:         %u minute%s\n",
                tmh->sampling_period, (tmh->sampling_period==1?"":"s"));
        printf("Temperature Logging Interval:        %u minute%s\n",
                tmh->interval,        (tmh->interval==1?"":"s"));
        printf("Min/Max recommended Temperature:     %s/%s Celsius\n",
                sct_ptemp(tmh->min_op_limit, buf1), sct_ptemp(tmh->max_op_limit, buf2));
        printf("Min/Max Temperature Limit:           %s/%s Celsius\n",
                sct_ptemp(tmh->under_limit, buf1), sct_ptemp(tmh->over_limit, buf2));
        printf("Temperature History Size (Index):    %u (%u)\n", tmh->cb_size, tmh->cb_index);
        if (!(0 < tmh->cb_size && tmh->cb_size <= sizeof(tmh->cb) && tmh->cb_index < tmh->cb_size)) {
                printf("Error invalid Temperature History Size or Index\n");
                return 0;
        }

        // Print table
        printf("\nIndex    Estimated Time   Temperature Celsius\n");
        unsigned n = 0, i = (tmh->cb_index+1) % tmh->cb_size;
        unsigned interval = (tmh->interval > 0 ? tmh->interval : 1);
        time_t t = time(0) - (tmh->cb_size-1) * interval * 60;
        t -= t % (interval * 60);
        while (n < tmh->cb_size) {
                // Find range of identical temperatures
                unsigned n1 = n, n2 = n+1, i2 = (i+1) % tmh->cb_size;
                while (n2 < tmh->cb_size && tmh->cb[i2] == tmh->cb[i]) {
                        n2++; i2 = (i2+1) % tmh->cb_size;
                }
                // Print range
                while (n < n2) {
                        if (n == n1 || n == n2-1 || n2 <= n1+3) {
                                char date[30];
                                // TODO: Don't print times < boot time
                                strftime(date, sizeof(date), "%Y-%m-%d %H:%M", localtime(&t));
                                printf(" %3u    %s    %s  %s\n", i, date,
                                        sct_ptemp(tmh->cb[i], buf1), sct_pbar(tmh->cb[i], buf2));
                        } else if (n == n1+1) {
                                printf(" ...    ..(%3u skipped).    ..  %s\n",
                                        n2-n1-2, sct_pbar(tmh->cb[i], buf2));
                        }
                        t += interval * 60; i = (i+1) % tmh->cb_size; n++;
                }
        }
        //assert(n == tmh->cb_size && i == (tmh->cb_index+1) % tmh->cb_size);

        return 0;
}

// Compares failure type to policy in effect, and either exits or
// simply returns to the calling routine.
void
failuretest(int type, int returnvalue)
{
}
#if 0
void
failuretest(int type, int returnvalue)
{
	// If this is an error in an "optional" SMART command
  	if (type == OPTIONAL_CMD) {
    		if (con->conservative) {
      			printf("An optional SMART command failed: exiting.  \
				Remove '-T conservative' option to continue.\n");
      			EXIT(returnvalue);
    		}
    		return;
  	}

  	// If this is an error in a "mandatory" SMART command
  	if (type == MANDATORY_CMD) {
    		if (con->permissive--)
      			return;
    		printf("A mandatory SMART command failed: exiting. \
			To continue, add one or more '-T permissive' options.\n");
    		EXIT(returnvalue);
  	}

  	printf("Smartctl internal error in failuretest(type=%d). \
		Please contact developers at " PACKAGE_HOMEPAGE "\n",type);
  	EXIT(returnvalue|FAILCMD);
}
#endif

// Initialize to zero just in case some SMART routines don't work
//static ata_identify_device_t drive;
//static ata_smart_values_t smartval;
//static ata_smart_thresholds_pvt_t smartthres;
static ata_smart_errorlog_t smarterror;
static ata_smart_selftestlog_t smartselftest;

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

/*
 * Fetch the contents of the logpage, and then call the logpage-specific
 * analysis function.  The analysis function is responsible for detecting any
 * faults and filling in the details.
 */
static int
analyze_one_logpage(ds_ata_info_t *aip, logpage_validation_entry_t *entry)
{
	int result;

	result = entry->ve_analyze(aip);

	if (!result) {
		return (result);
	} else {
		result = ata_set_errno(aip, EDS_IO);
	}

	return (result);
}

static int
logpage_temp_analyze(ds_ata_info_t *aip)
{
	int i, plen = 0;
	int e = 0;
	int returnval = 0;
	uint8_t reftemp, curtemp;
	nvlist_t *nvl = NULL;
	char fullclass[100];

	memset(fullclass, 0, 100 * sizeof (char));
	assert(aip->ai_dsp->ds_ata_overtemp == NULL);
	nvl = aip->ai_dsp->ds_ata_overtemp;

	reftemp = curtemp = INVALID_TEMPERATURE;
	
	ata_sct_status_response_t sts;
	ata_sct_temperature_history_table_t tmh;

	// Read SCT status and temperature history
	if (ataReadSCTTempHist(aip, &tmh, &sts)) {
		failuretest(OPTIONAL_CMD, returnval|=FAILSMART);
		return 0;
	}

	/*********BUG!!! Fix me!!!*********
	// Current Temp
	curtemp = sts.hda_temp;
	// Reftemp Temp
	reftemp = tmh.max_op_limit; //60 Celsius
	**********************************/

	unsigned char *lb = (unsigned char *)&sts;
	// Current Temp
	curtemp = lb[200];
	// Reftemp Temp
	reftemp = 60;           //60 Celsius

	if (reftemp != INVALID_TEMPERATURE && curtemp != INVALID_TEMPERATURE &&
		curtemp > reftemp) {

		snprintf(fullclass, sizeof(fullclass), "%s.io.ata.disk.over-temperature",
			FM_EREPORT_CLASS);

		disk_fm_event_post(nvl, fullclass);

		printf("Disk Module: post %s\n", fullclass);
	}

	return (0);
}

#define	SELFTEST_COMPLETE(code)				\
	((code) == 0x0 /*SELFTEST_OK*/ ||			\
	((code) >= 0x3 /*SELFTEST_FAILURE_INCOMPLETE*/ &&	\
	((code) <= 0x8 /*SELFTEST_FAILURE_SEG_OTHER*/)))

static int
logpage_selftest_analyze(ds_ata_info_t *aip)
{
	int retval = 0, hours = 0, testno = 0;
	int e = 0;
	int returnval = 0;
	nvlist_t *nvl = NULL;
	char fullclass[100];

	memset(fullclass, 0, 100 * sizeof (char));
	assert(aip->ai_dsp->ds_ata_testfail == NULL);
	nvl = aip->ai_dsp->ds_ata_testfail;

        // Print SMART self-test log
        unsigned char fix_firmwarebug = FIX_NOTSPECIFIED;	/////////// Question ?
        if(ataReadSelfTestLog(aip, &smartselftest, fix_firmwarebug)) {
		printf("Smartctl: SMART Self Test Log Read Failed\n");
                failuretest(OPTIONAL_CMD, returnval|=FAILSMART);
        } else {
  		// print log
		int i;
  		for (i = 20; i >= 0; i--) {
    			// log is a circular buffer
    			int j = (i + smartselftest.mostrecenttest) % 21;
    			const ata_smart_selftestlog_struct_t *log = smartselftest.selftest_struct+j;

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

  			bool is_error = false;
  			switch (log->selfteststatus >> 4) {
    				case 0x0: break;			/* Completed without error */
    				case 0x1: break;			/* Aborted by host */
    				case 0x2: break;			/* Interrupted (host reset) */
    				case 0x3: is_error = true; break;	/* Fatal or unknown error */
    				case 0x4: is_error = true; break;	/* Completed: unknown failure */
    				case 0x5: is_error = true; break;	/* Completed: electrical failure */
    				case 0x6: is_error = true; break;	/* Completed: servo/seek failure */
    				case 0x7: is_error = true; break;	/* Completed: read failure */
    				case 0x8: is_error = true; break;	/* Completed: handling damage?? */
    				case 0xf: break;			/* Self-test routine in progress */
    				default:  ;  				/* Unknown/reserved test status */
  			}

  			if (!is_error) 
				continue;

		/*
		 * We always log the last result, or the result of the
		 * last completed test.
		 */
		if (SELFTEST_COMPLETE(log->selfteststatus)) {
			if (log->selfteststatus != 0x0 /*SELFTEST_OK*/) {
				snprintf(fullclass, sizeof(fullclass),
				"%s.io.ata.disk.self-test-failure", FM_EREPORT_CLASS);

				disk_fm_event_post(nvl, fullclass);

				printf("Disk module: post %s\n", fullclass);

				return (0);
			}
		}
		} //if
		} //for
	} //else

	return 0;
}

static int
logpage_error_analyze(ds_ata_info_t *aip)
{
	int retval=0, hours=0, testno=0;
	int e = 0;
	int returnval = 0;
	nvlist_t *nvl = NULL;
	char fullclass[100];

	memset(fullclass, 0, 100 * sizeof (char));
	assert(aip->ai_dsp->ds_ata_error == NULL);
	nvl = aip->ai_dsp->ds_ata_error;

	// Print SMART error log
	unsigned char fix_firmwarebug = FIX_NOTSPECIFIED;	//////////// Question ?
	if (ataReadErrorLog(aip, &smarterror, fix_firmwarebug)) {
		printf("Smartctl: SMART Error Log Read Failed\n");
		failuretest(OPTIONAL_CMD, returnval|=FAILSMART);
	} else {
		// if no errors logged, return
  		if (!smarterror.error_log_pointer) {
    			printf("No Errors Logged\n\n");
    			return 0;
  		}
  		// If log pointer out of range, return
  		if (smarterror.error_log_pointer>5){
    			printf("Invalid Error Log index = 0x%02x (T13/1321D rev 1c "
         		     "Section 8.41.6.8.2.2 gives valid range from 1 to 5)\n\n",
         			(int)smarterror.error_log_pointer);
    			return 0;
  		}
		// quiet mode is turned on inside ataPrintSmartErrorLog()
		// now step through the five error log data structures (table 39 of spec)
        	int k;
        	for (k = 4; k >= 0; k-- ) {

                	// The error log data structure entries are a circular buffer
                	int j, i = (smarterror.error_log_pointer + k) % 5;
                	const ata_smart_errorlog_struct_t *elog = smarterror.errorlog_struct + i;
                	const ata_smart_errorlog_error_struct_t *summary = &(elog->error_struct);

                	// Spec says: unused error log structures shall be zero filled
                	if (nonempty((unsigned char*)elog,sizeof(*elog))) {
                        	// Table 57 of T13/1532D Volume 1 Revision 3

				snprintf(fullclass, sizeof(fullclass),
				"%s.io.ata.disk.self-test-failure", FM_EREPORT_CLASS);

				disk_fm_event_post(nvl, fullclass);

				printf("Disk module: post %s\n", fullclass);

                                return 0;
			}
		} //for
	} //if

	return 0;
}

static int
logpage_sataphy_analyze(ds_ata_info_t *aip)
{
	int returnval = 0;
	int sataphy = 1;
	int sataphy_reset = 1;

///////////////////////////////////////////////////////////////////
	// Print SMART and/or GP log Directory and/or logs
	// Get #pages for extended SMART logs
	ata_smart_log_directory_t gplogdir_buf;
	const ata_smart_log_directory_t *gplogdir = 0;

	if (ataReadLogDirectory(aip, &gplogdir_buf, true)) {
//	if (ataReadLogDirectory(aip, &gplogdir_buf, false)) {
		printf("Read GP Log Directory failed.\n\n");
		failuretest(OPTIONAL_CMD, returnval|=FAILSMART);
	} else
		gplogdir = &gplogdir_buf;
///////////////////////////////////////////////////////////////////

	// Print SATA Phy Event Counters
	if (sataphy) {
		unsigned nsectors = GetNumLogSectors(gplogdir, 0x11, true);
		if (!nsectors)
			printf("SATA Phy Event Counters (GP Log 0x11) not supported\n");
		else if (nsectors != 1)
			printf("SATA Phy Event Counters with %u sectors not supported\n", nsectors);
		else {
			unsigned char log_11[512] = {0, };
			unsigned char features = (sataphy_reset ? 0x01 : 0x00);
			if (!ataReadLogExt(aip, 0x11, features, 0, log_11, 1))
				failuretest(OPTIONAL_CMD, returnval|=FAILSMART);
			else
				PrintSataPhyEventCounters(log_11, sataphy_reset);
		}
	}

	return 0;
}

/*
 * Clean up the scsi-specific information structure.
 */
static void
ds_ata_close(void *arg)
{
	ds_ata_info_t *aip = arg;

	free(aip);
}

/*
 * Initialize a single disk.  Initialization consists of:
 *
 * 1. Check to see if the IE mechanism is enabled via MODE SENSE for the IE
 *    Control page (page 0x1C).
 *
 * 2. If the IE page is available, try to set the following parameters:
 *
 *    	DEXCPT		0	Enable exceptions
 *    	MRIE		6	Only report IE information on request
 *    	EWASC		1	Enable warning reporting
 *    	REPORT COUNT	1	Only report an IE exception once
 *    	LOGERR		1	Enable logging of errors
 *
 *    The remaining fields are left as-is, preserving the current values.  If we
 *    cannot set some of these fields, then we do our best.  Some drives may
 *    have a static configuration which still allows for some monitoring.
 *
 * 3. Check to see if the IE log page (page 0x2F) is supported by issuing a
 *    LOG SENSE command.
 *
 * 4. Check to see if the self-test log page (page 0x10) is supported.
 *
 * 5. Check to see if the temperature log page (page 0x0D) is supported, and
 *    contains a reference temperature.
 *
 * 6. Clear the GLTSD bit in control mode page 0xA.  This will allow the drive
 *    to save each of the log pages described above to nonvolatile storage.
 *    This is essential if the drive is to remember its failures across
 *    loss of power.
 */
static void *
ds_ata_open_common(disk_status_t *dsp, ds_ata_info_t *aip)
{
	aip->ai_dsp = dsp;

	// From here on, every command requires that SMART be enabled...
	if (!ataDoesSmartWork(aip)) {
		printf("SMART Disabled. Use option -s with argument 'on' to enable it.\n");
		ds_ata_close(aip);
		return (NULL);
	}

	return (aip);
}

static void *
ds_ata_open(disk_status_t *dsp)
{
	ds_ata_info_t *aip;

	if ((aip = calloc(sizeof (ds_ata_info_t), 1)) == NULL) {
		(void) ds_set_errno(dsp, EDS_NOMEM);
		return (NULL);
	}

	return (ds_ata_open_common(dsp, aip));
}

/*
 * Scan for any faults.  The following steps are performed:
 *
 * 1. If the temperature log page is supported, check the current temperature
 *    and threshold.  If the current temperature exceeds the threshold, report
 *    and overtemp fault.
 *
 * 2. If the selftest log page is supported, check to the last completed self
 *    test.  If the last completed test resulted in failure, report a selftest
 *    fault.
 *
 * 3. If the IE log page is supported, check to see if failure is predicted.  If
 *    so, indicate a predictive failure fault.
 *
 * 4. If the IE log page is not supported, but the mode page supports report on
 *    request mode, then issue a REQUEST SENSE for the mode page.  Indicate a
 *    predictive failure fault if necessary.
 */
static int
ds_ata_scan(void *arg)
{
	ds_ata_info_t *aip = arg;
	int i;

	for (i = 0; i < NLOG_VALIDATION; i++) {
		if (analyze_one_logpage(aip, &log_validation[i]) != 0)
			return (-1);
	}

	return (0);
}

ds_transport_t ds_ata_transport = {
	ds_ata_open,
	ds_ata_close,
	ds_ata_scan
};

