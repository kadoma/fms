/*
 * ds_ata_ioctl.h
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 *	ds_ata_ioctl.h
 */

#ifndef	_DS_ATA_IOCTL_H
#define	_DS_ATA_IOCTL_H

#include "ds_ata.h"

#include "dev_interface.h" // ata_device/

//////////////////////////////////////////////////////////////////////////////
// os_linux.h
// The following definitions are from hdreg.h in the kernel source
// tree.  They don't carry any Copyright statements, but I think they
// are primarily from Mark Lord and Andre Hedrick.
typedef unsigned char task_ioreg_t;

typedef struct hd_drive_task_hdr {
	task_ioreg_t data;
	task_ioreg_t feature;
	task_ioreg_t sector_count;
	task_ioreg_t sector_number;
	task_ioreg_t low_cylinder;
	task_ioreg_t high_cylinder;
	task_ioreg_t device_head;
	task_ioreg_t command;
} task_struct_t;

typedef union ide_reg_valid_s {
	unsigned all			: 16;
	struct {
    		unsigned data		: 1;
    		unsigned error_feature	: 1;
    		unsigned sector		: 1;
    		unsigned nsector	: 1;
    		unsigned lcyl		: 1;
    		unsigned hcyl		: 1;
    		unsigned select		: 1;
    		unsigned status_command	: 1;
    		unsigned data_hob		: 1;
    		unsigned error_feature_hob	: 1;
    		unsigned sector_hob		: 1;
    		unsigned nsector_hob		: 1;
    		unsigned lcyl_hob		: 1;
    		unsigned hcyl_hob		: 1;
    		unsigned select_hob		: 1;
    		unsigned control_hob		: 1;
  	} b;
} ide_reg_valid_t;

typedef struct ide_task_request_s {
	task_ioreg_t	   io_ports[8];
  	task_ioreg_t	   hob_ports[8];
  	ide_reg_valid_t    out_flags;
  	ide_reg_valid_t    in_flags;
  	int		   data_phase;
  	int		   req_cmd;
  	unsigned long	   out_size;
  	unsigned long	   in_size;
} ide_task_request_t;

#define TASKFILE_NO_DATA	  0x0000
#define TASKFILE_IN		  0x0001
#define TASKFILE_OUT		  0x0004
#define HDIO_DRIVE_TASK_HDR_SIZE  8*sizeof(task_ioreg_t)
#define IDE_DRIVE_TASK_NO_DATA	       0
#define IDE_DRIVE_TASK_IN	       2
#define IDE_DRIVE_TASK_OUT	       3
#define HDIO_DRIVE_CMD            0x031f
#define HDIO_DRIVE_TASK           0x031e
#define HDIO_DRIVE_TASKFILE       0x031d
#define HDIO_GET_IDENTITY         0x030d

#define HPTIO_CTL                 0x03ff // ioctl interface for HighPoint raid device

//////////////////////////////////////////////////////////////////////////////

typedef enum {
	// returns no data, just succeeds or fails
	ENABLE,
	DISABLE,
	AUTOSAVE,
	IMMEDIATE_OFFLINE,
	AUTO_OFFLINE,
	STATUS,       // just says if SMART is working or not
	STATUS_CHECK, // says if disk's SMART status is healthy, or failing
	// return 512 bytes of data:
	READ_VALUES,
	READ_THRESHOLDS,
	READ_LOG,
	IDENTIFY,
	PIDENTIFY,
	// returns 1 byte of data
	CHECK_POWER_MODE,
	// writes 512 bytes of data:
	WRITE_LOG
} smart_command_set;

// ATA Specification Command Register Values (Commands)
#define ATA_IDENTIFY_DEVICE             0xec
#define ATA_IDENTIFY_PACKET_DEVICE      0xa1
#define ATA_SMART_CMD                   0xb0
#define ATA_CHECK_POWER_MODE            0xe5
// 48-bit commands
#define ATA_READ_LOG_EXT                0x2F

// ATA Specification Feature Register Values (SMART Subcommands).
// Note that some are obsolete as of ATA-7.
#define ATA_SMART_READ_VALUES           0xd0
#define ATA_SMART_READ_THRESHOLDS       0xd1
#define ATA_SMART_AUTOSAVE              0xd2
#define ATA_SMART_SAVE                  0xd3
#define ATA_SMART_IMMEDIATE_OFFLINE     0xd4
#define ATA_SMART_READ_LOG_SECTOR       0xd5
#define ATA_SMART_WRITE_LOG_SECTOR      0xd6
#define ATA_SMART_WRITE_THRESHOLDS      0xd7
#define ATA_SMART_ENABLE                0xd8
#define ATA_SMART_DISABLE               0xd9
#define ATA_SMART_STATUS                0xda
// SFF 8035i Revision 2 Specification Feature Register Value (SMART
// Subcommand)
#define ATA_SMART_AUTO_OFFLINE          0xdb

// Sector Number values for ATA_SMART_IMMEDIATE_OFFLINE Subcommand
#define OFFLINE_FULL_SCAN               0
#define SHORT_SELF_TEST                 1
#define EXTEND_SELF_TEST                2
#define CONVEYANCE_SELF_TEST            3
#define SELECTIVE_SELF_TEST             4
#define ABORT_SELF_TEST                 127
#define SHORT_CAPTIVE_SELF_TEST         129
#define EXTEND_CAPTIVE_SELF_TEST        130
#define CONVEYANCE_CAPTIVE_SELF_TEST    131
#define SELECTIVE_CAPTIVE_SELF_TEST     132
#define CAPTIVE_MASK                    (0x01<<7)

// Maximum allowed number of SMART Attributes
#define NUMBER_ATA_SMART_ATTRIBUTES     30

// Needed parts of the ATA DRIVE IDENTIFY Structure. Those labeled
// word* are NOT used.
typedef struct ata_identify_device {
	unsigned short words000_009[10];
	unsigned char  serial_no[20];
	unsigned short words020_022[3];
	unsigned char  fw_rev[8];
	unsigned char  model[40];
	unsigned short words047_079[33];
	unsigned short major_rev_num;
	unsigned short minor_rev_num;
	unsigned short command_set_1;
	unsigned short command_set_2;
	unsigned short command_set_extension;
	unsigned short cfs_enable_1;
	unsigned short word086;
	unsigned short csf_default;
	unsigned short words088_255[168];
} ata_identify_device_t;
//ASSERT_SIZEOF_STRUCT(ata_identify_device, 512);

/* ata_smart_attribute is the vendor specific in SFF-8035 spec */ 
typedef struct ata_smart_attribute {
	unsigned char id;
	// meaning of flag bits: see MACROS just below
	// WARNING: MISALIGNED!
	unsigned short flags; 
	unsigned char current;
	unsigned char worst;
	unsigned char raw[6];
	unsigned char reserv;
} ata_smart_attribute_t;
//ASSERT_SIZEOF_STRUCT(ata_smart_attribute, 12);

// MACROS to interpret the flags bits in the previous structure.
// These have not been implemented using bitflags and a union, to make
// it portable across bit/little endian and different platforms.

// 0: Prefailure bit

// From SFF 8035i Revision 2 page 19: Bit 0 (pre-failure/advisory bit)
// - If the value of this bit equals zero, an attribute value less
// than or equal to its corresponding attribute threshold indicates an
// advisory condition where the usage or age of the device has
// exceeded its intended design life period. If the value of this bit
// equals one, an attribute value less than or equal to its
// corresponding attribute threshold indicates a prefailure condition
// where imminent loss of data is being predicted.
#define ATTRIBUTE_FLAGS_PREFAILURE(x) (x & 0x01)

// 1: Online bit 

//  From SFF 8035i Revision 2 page 19: Bit 1 (on-line data collection
// bit) - If the value of this bit equals zero, then the attribute
// value is updated only during off-line data collection
// activities. If the value of this bit equals one, then the attribute
// value is updated during normal operation of the device or during
// both normal operation and off-line testing.
#define ATTRIBUTE_FLAGS_ONLINE(x) (x & 0x02)


// The following are (probably) IBM's, Maxtors and  Quantum's definitions for the
// vendor-specific bits:
// 2: Performance type bit
#define ATTRIBUTE_FLAGS_PERFORMANCE(x) (x & 0x04)

// 3: Errorrate type bit
#define ATTRIBUTE_FLAGS_ERRORRATE(x) (x & 0x08)

// 4: Eventcount bit
#define ATTRIBUTE_FLAGS_EVENTCOUNT(x) (x & 0x10)

// 5: Selfpereserving bit
#define ATTRIBUTE_FLAGS_SELFPRESERVING(x) (x & 0x20)


// Last ten bits are reserved for future use

/* ata_smart_values is format of the read drive Attribute command */
/* see Table 34 of T13/1321D Rev 1 spec (Device SMART data structure) for *some* info */
typedef struct ata_smart_values {
	unsigned short int revnumber;
	ata_smart_attribute_t vendor_attributes[NUMBER_ATA_SMART_ATTRIBUTES];
	unsigned char offline_data_collection_status;
	unsigned char self_test_exec_status;  //IBM # segments for offline collection
	unsigned short int total_time_to_complete_off_line; // IBM different
	unsigned char vendor_specific_366; // Maxtor & IBM curent segment pointer
	unsigned char offline_data_collection_capability;
	unsigned short int smart_capability;
	unsigned char errorlog_capability;
	unsigned char vendor_specific_371;  // Maxtor, IBM: self-test failure checkpoint see below!
	unsigned char short_test_completion_time;
	unsigned char extend_test_completion_time;
	unsigned char conveyance_test_completion_time;
	unsigned char reserved_375_385[11];
	unsigned char vendor_specific_386_510[125]; // Maxtor bytes 508-509 Attribute/Threshold Revision #
	unsigned char chksum;
} ata_smart_values_t;
//ASSERT_SIZEOF_STRUCT(ata_smart_values, 512);

/* Maxtor, IBM: self-test failure checkpoint byte meaning:
 00 - write test
 01 - servo basic
 02 - servo random
 03 - G-list scan
 04 - Handling damage
 05 - Read scan
*/

/* Vendor attribute of SMART Threshold (compare to ata_smart_attribute above) */
typedef struct ata_smart_threshold_entry {
	unsigned char id;
	unsigned char threshold;
	unsigned char reserved[10];
} ata_smart_threshold_entry_t;

/* Format of Read SMART THreshold Command */
/* Compare to ata_smart_values above */
typedef struct ata_smart_thresholds_pvt {
	unsigned short int revnumber;
	ata_smart_threshold_entry_t thres_entries[NUMBER_ATA_SMART_ATTRIBUTES];
	unsigned char reserved[149];
	unsigned char chksum;
} ata_smart_thresholds_pvt_t;


// Table 42 of T13/1321D Rev 1 spec (Error Data Structure)
typedef struct ata_smart_errorlog_error_struct {
	unsigned char reserved;
	unsigned char error_register;
	unsigned char sector_count;
	unsigned char sector_number;
	unsigned char cylinder_low;
	unsigned char cylinder_high;
	unsigned char drive_head;
	unsigned char status;
	unsigned char extended_error[19];
	unsigned char state;
	unsigned short timestamp;
} ata_smart_errorlog_error_struct_t;


// Table 41 of T13/1321D Rev 1 spec (Command Data Structure)
typedef struct ata_smart_errorlog_command_struct {
	unsigned char devicecontrolreg;
	unsigned char featuresreg;
	unsigned char sector_count;
	unsigned char sector_number;
	unsigned char cylinder_low;
	unsigned char cylinder_high;
	unsigned char drive_head;
	unsigned char commandreg;
	unsigned int timestamp;
} ata_smart_errorlog_command_struct_t;

// Table 40 of T13/1321D Rev 1 spec (Error log data structure)
typedef struct ata_smart_errorlog_struct {
	ata_smart_errorlog_command_struct_t commands[5];
	ata_smart_errorlog_error_struct_t error_struct;
} ata_smart_errorlog_struct_t;

// Table 39 of T13/1321D Rev 1 spec (SMART error log sector)
typedef struct ata_smart_errorlog {
	unsigned char revnumber;
 	unsigned char error_log_pointer;
  	ata_smart_errorlog_struct_t errorlog_struct[5];
  	unsigned short int ata_error_count;
  	unsigned char reserved[57];
  	unsigned char checksum;
} ata_smart_errorlog_t;

// Extended Comprehensive SMART Error Log data structures
// See Section A.7 of
//   AT Attachment 8 - ATA/ATAPI Command Set (ATA8-ACS)
//   T13/1699-D Revision 6a (Working Draft), September 6, 2008.

// Command data structure
// Table A.9 of T13/1699-D Revision 6a
typedef struct ata_smart_exterrlog_command {
	unsigned char device_control_register;
	unsigned char features_register;
	unsigned char features_register_hi;
	unsigned char count_register;
	unsigned char count_register_hi;
	unsigned char lba_low_register;
	unsigned char lba_low_register_hi;
	unsigned char lba_mid_register;
	unsigned char lba_mid_register_hi;
	unsigned char lba_high_register;
	unsigned char lba_high_register_hi;
	unsigned char device_register;
	unsigned char command_register;

	unsigned char reserved;
	unsigned int timestamp;
} ata_smart_exterrlog_command_t;

// Error data structure
// Table A.10 T13/1699-D Revision 6a
typedef struct ata_smart_exterrlog_error {
	unsigned char device_control_register;
	unsigned char error_register;
	unsigned char count_register;
	unsigned char count_register_hi;
	unsigned char lba_low_register;
	unsigned char lba_low_register_hi;
	unsigned char lba_mid_register;
	unsigned char lba_mid_register_hi;
	unsigned char lba_high_register;
	unsigned char lba_high_register_hi;
	unsigned char device_register;
	unsigned char status_register;

	unsigned char extended_error[19];
	unsigned char state;
	unsigned short timestamp;
} ata_smart_exterrlog_error_t;

// Error log data structure
// Table A.8 of T13/1699-D Revision 6a
typedef struct ata_smart_exterrlog_error_log {
	ata_smart_exterrlog_command_t commands[5];
	ata_smart_exterrlog_error_t error;
} ata_smart_exterrlog_error_log_t;

// Ext. Comprehensive SMART error log
// Table A.7 of T13/1699-D Revision 6a
typedef struct ata_smart_exterrlog {
	unsigned char version;
	unsigned char reserved1;
	unsigned short error_log_index;
	ata_smart_exterrlog_error_log_t error_logs[4];
	unsigned short device_error_count;
	unsigned char reserved2[9];
	unsigned char checksum;
} ata_smart_exterrlog_t;

// Table 45 of T13/1321D Rev 1 spec (Self-test log descriptor entry)
typedef struct ata_smart_selftestlog_struct {
	unsigned char selftestnumber; // Sector number register
	unsigned char selfteststatus;
	unsigned short int timestamp;
	unsigned char selftestfailurecheckpoint;
	unsigned int lbafirstfailure;
	unsigned char vendorspecific[15];
} ata_smart_selftestlog_struct_t;

// Table 44 of T13/1321D Rev 1 spec (Self-test log data structure)
typedef struct ata_smart_selftestlog {
	unsigned short int revnumber;
	ata_smart_selftestlog_struct_t selftest_struct[21];
	unsigned char vendorspecific[2];
	unsigned char mostrecenttest;
	unsigned char reserved[2];
	unsigned char chksum;
} ata_smart_selftestlog_t;

// Extended SMART Self-test log data structures
// See Section A.8 of
//   AT Attachment 8 - ATA/ATAPI Command Set (ATA8-ACS)
//   T13/1699-D Revision 6a (Working Draft), September 6, 2008.

// Extended Self-test log descriptor entry
// Table A.13 of T13/1699-D Revision 6a
typedef struct ata_smart_extselftestlog_desc {
	unsigned char self_test_type;
	unsigned char self_test_status;
	unsigned short timestamp;
	unsigned char checkpoint;
	unsigned char failing_lba[6];
	unsigned char vendorspecific[15];
} ata_smart_extselftestlog_desc_t;

// Extended Self-test log data structure
// Table A.12 of T13/1699-D Revision 6a
typedef struct ata_smart_extselftestlog {
	unsigned char 		version;
	unsigned char 		reserved1;
	unsigned short 		log_desc_index;
	ata_smart_extselftestlog_desc_t log_descs[19];
	unsigned char 		vendor_specifc[2];
	unsigned char 		reserved2[11];
	unsigned char 		chksum;
} ata_smart_extselftestlog_t;

// SMART LOG DIRECTORY Table 52 of T13/1532D Vol 1 Rev 1a
typedef struct ata_smart_log_entry {
	unsigned char		numsectors;
	unsigned char		reserved;
} ata_smart_log_entry_t;

typedef struct ata_smart_log_directory {
	unsigned short int	logversion;
	ata_smart_log_entry_t 	entry[255];
} ata_smart_log_directory_t;

// SMART SELECTIVE SELF-TEST LOG Table 61 of T13/1532D Volume 1
// Revision 3
typedef struct test_span {
	uint64_t	start;
	uint64_t	end;
} test_span_t;

typedef struct ata_selective_self_test_log {
	unsigned short     logversion;
	test_span_t	   span[5];
	unsigned char      reserved1[337-82+1];
	unsigned char      vendor_specific1[491-338+1];
	uint64_t           currentlba;
	unsigned short     currentspan;
	unsigned short     flags;
	unsigned char      vendor_specific2[507-504+1];
	unsigned short     pendingtime;
	unsigned char      reserved2;
	unsigned char      checksum;
} ata_selective_self_test_log_t;

#define SELECTIVE_FLAG_DOSCAN  (0x0002)
#define SELECTIVE_FLAG_PENDING (0x0008)
#define SELECTIVE_FLAG_ACTIVE  (0x0010)


// SCT (SMART Command Transport) data structures
// See Sections 8.2 and 8.3 of:
//   AT Attachment 8 - ATA/ATAPI Command Set (ATA8-ACS)
//   T13/1699-D Revision 3f (Working Draft), December 11, 2006.

// SCT Status response (read with SMART_READ_LOG page 0xe0)
// Table 60 of T13/1699-D Revision 3f 
typedef struct ata_sct_status_response {
	unsigned short format_version;    // 0-1: Status response format version number (2, 3)
	unsigned short sct_version;       // 2-3: Vendor specific version number
	unsigned short sct_spec;          // 4-5: SCT level supported (1)
	unsigned int status_flags;        // 6-9: Status flags (Bit 0: Segment initialized, Bits 1-31: reserved)
	unsigned char device_state;       // 10: Device State (0-5)
	unsigned char bytes011_013[3];    // 11-13: reserved
	unsigned short ext_status_code;   // 14-15: Status of last SCT command (0xffff if executing)
	unsigned short action_code;       // 16-17: Action code of last SCT command
	unsigned short function_code;     // 18-19: Function code of last SCT command
	unsigned char bytes020_039[20];   // 20-39: reserved
	uint64_t lba_current;             // 40-47: LBA of SCT command executing in background
	unsigned char bytes048_199[152];  // 48-199: reserved
	signed char hda_temp;             // 200: Current temperature in Celsius (0x80 = invalid)
	signed char min_temp;             // 201: Minimum temperature this power cycle
	signed char max_temp;             // 202: Maximum temperature this power cycle
	signed char life_min_temp;        // 203: Minimum lifetime temperature
	signed char life_max_temp;        // 204: Maximum lifetime temperature
	unsigned char byte205;            // 205: reserved (T13/e06152r0-2: Average lifetime temperature)
	unsigned int over_limit_count;    // 206-209: # intervals since last reset with temperature > Max Op Limit
	unsigned int under_limit_count;   // 210-213: # intervals since last reset with temperature < Min Op Limit
	unsigned char bytes214_479[266];  // 214-479: reserved
	unsigned char vendor_specific[32];// 480-511: vendor specific
} ata_sct_status_response_t;

// SCT Feature Control command (send with SMART_WRITE_LOG page 0xe0)
// Table 72 of T13/1699-D Revision 3f
typedef struct ata_sct_feature_control_command {
	unsigned short action_code;       // 4 = Feature Control
	unsigned short function_code;     // 1 = Set, 2 = Return, 3 = Return options
	unsigned short feature_code;      // 3 = Temperature logging interval
	unsigned short state;             // Interval
	unsigned short option_flags;      // Bit 0: persistent, Bits 1-31: reserved
	unsigned short words005_255[251]; // reserved 
} ata_sct_feature_control_command_t;

// SCT Data Table command (send with SMART_WRITE_LOG page 0xe0)
// Table 73 of T13/1699-D Revision 3f 
typedef struct ata_sct_data_table_command {
	unsigned short action_code;       // 5 = Data Table
	unsigned short function_code;     // 1 = Read Table
	unsigned short table_id;          // 2 = Temperature History
	unsigned short words003_255[253]; // reserved
} ata_sct_data_table_command_t;

// SCT Temperature History Table (read with SMART_READ_LOG page 0xe1)
// Table 75 of T13/1699-D Revision 3f 
typedef struct ata_sct_temperature_history_table {
	unsigned short format_version;    // 0-1: Data table format version number (2)
	unsigned short sampling_period;   // 2-3: Temperature sampling period in minutes
	unsigned short interval;          // 4-5: Timer interval between history entries
	signed char max_op_limit;         // 6: Maximum recommended continuous operating temperature
	signed char over_limit;           // 7: Maximum temperature limit
	signed char min_op_limit;         // 8: Minimum recommended continuous operating limit
	signed char under_limit;          // 9: Minimum temperature limit
	unsigned char bytes010_029[20];   // 10-29: reserved
	unsigned short cb_size;           // 30-31: Number of history entries (range 128-478)
	unsigned short cb_index;          // 32-33: Index of last updated entry (zero-based)
	signed char cb[478];              // 34-(34+cb_size-1): Circular buffer of temperature values
} ata_sct_temperature_history_table_t;

// Possible values for span_args.mode
enum {
	SEL_RANGE, // MIN-MAX
	SEL_REDO,  // redo this
	SEL_NEXT,  // do next range
	SEL_CONT   // redo or next depending of last test status
};

#if 0
// Arguments for selective self-test
typedef struct ata_selective_selftest_args {
	// Arguments for each span
	struct span_args {
		uint64_t start;   // First block
		uint64_t end;     // Last block
		int mode;         // SEL_*, see above

		span_args()
		: start(0), end(0), mode(SEL_RANGE) { }
	};

	span_args span[5];  // Range and mode for 5 spans
	int num_spans;      // Number of spans
	int pending_time;   // One plus time in minutes to wait after powerup before restarting
        	            // interrupted offline scan after selective self-test.
	int scan_after_select; // Run offline scan after selective self-test:
        	               // 0: don't change,
                    	       // 1: turn off scan after selective self-test,
                      	       // 2: turn on scan after selective self-test.

	ata_selective_selftest_args()
	: num_spans(0), pending_time(0), scan_after_select(0) { }
} ata_selective_selftest_args_t;

// Priority for vendor attribute defs
enum ata_vendor_def_prior
{
	PRIOR_DEFAULT,
	PRIOR_DATABASE,
	PRIOR_USER
};

// Raw attribute value print formats
enum ata_attr_raw_format
{
	RAWFMT_DEFAULT,
	RAWFMT_RAW8,
	RAWFMT_RAW16,
	RAWFMT_RAW48,
	RAWFMT_HEX48,
	RAWFMT_RAW64,
	RAWFMT_HEX64,
	RAWFMT_RAW16_OPT_RAW16,
	RAWFMT_RAW16_OPT_AVG16,
	RAWFMT_RAW24_RAW24,
	RAWFMT_SEC2HOUR,
	RAWFMT_MIN2HOUR,
	RAWFMT_HALFMIN2HOUR,
	RAWFMT_TEMPMINMAX,
	RAWFMT_TEMP10X,
};

// Attribute flags
enum {
	ATTRFLAG_INCREASING = 0x01, // Value not reset (for reallocated/pending counts)
	ATTRFLAG_NO_NORMVAL = 0x02  // Normalized value not valid
};
#endif

// Get information from drive
int ataReadHDIdentity(ds_ata_info_t *aip, ata_identify_device_t *buf);

/* Read S.M.A.R.T information from drive */
int ataReadSmartValues(ds_ata_info_t *aip, ata_smart_values_t *);
int ataReadErrorLog(ds_ata_info_t *aip, ata_smart_errorlog_t *data,
		unsigned char fix_firmwarebug);
int ataReadSelfTestLog(ds_ata_info_t *aip, ata_smart_selftestlog_t *data,
		unsigned char fix_firmwarebug);
int ataReadLogDirectory(ds_ata_info_t *aip, ata_smart_log_directory_t *, bool gpl);

// Read GP Log page(s)
bool ataReadLogExt(ds_ata_info_t *aip, unsigned char logaddr,
                unsigned char features, unsigned page,
                void *data, unsigned nsectors);
// Read SMART Log page(s)
bool ataReadSmartLog(ds_ata_info_t *aip, unsigned char logaddr,
                void *data, unsigned nsectors);
// Read SMART Extended Comprehensive Error Log
bool ataReadExtErrorLog(ds_ata_info_t *aip, ata_smart_exterrlog_t *log,
                unsigned nsectors);
// Read SMART Extended Self-test Log
bool ataReadExtSelfTestLog(ds_ata_info_t *aip, ata_smart_extselftestlog_t *log,
		unsigned nsectors);

// Read SCT information
int ataReadSCTStatus(ds_ata_info_t *aip, ata_sct_status_response_t *sts);
int ataReadSCTTempHist(ds_ata_info_t *aip, ata_sct_temperature_history_table_t *tmh,
                ata_sct_status_response_t *sts);

/* Enable/Disable SMART on device */
int ataEnableSmart(ds_ata_info_t *aip);

/* S.M.A.R.T. test commands */
int ataSmartExtendSelfTest(ds_ata_info_t *aip);
int ataSmartShortSelfTest(ds_ata_info_t *aip);
int ataSmartShortCapSelfTest(ds_ata_info_t *aip);
int ataSmartExtendCapSelfTest(ds_ata_info_t *aip);

// If SMART supported, this is guaranteed to return 1 if SMART is enabled, else 0.
int ataDoesSmartWork(ds_ata_info_t *aip);

// returns 1 if SMART supported, 0 if not supported or can't tell
int ataSmartSupport(const ata_identify_device_t *drive);

// Return values:
//  1: SMART enabled
//  0: SMART disabled
// -1: can't tell if SMART is enabled -- try issuing ataDoesSmartWork command to see
int ataIsSmartEnabled(const ata_identify_device_t *drive);

int isSmartErrorLogCapable(const ata_smart_values_t *data, const ata_identify_device_t *identity);

int isSmartTestLogCapable(const ata_smart_values_t *data, const ata_identify_device_t *identity);

int isGeneralPurposeLoggingCapable(const ata_identify_device_t *identity);

int isSupportSelfTest(const ata_smart_values_t *data);

extern inline bool isSCTCapable(const ata_identify_device_t *drive)
  { return !!(drive->words088_255[206-88] & 0x01); } // 0x01 = SCT support

//inline bool isSCTFeatureControlCapable(const ata_identify_device_t *drive)
//  { return ((drive->words088_255[206-88] & 0x11) == 0x11); } // 0x10 = SCT Feature Control support

//inline bool isSCTDataTableCapable(const ata_identify_device_t *drive)
//  { return ((drive->words088_255[206-88] & 0x21) == 0x21); } // 0x20 = SCT Data Table support

// Attribute state
enum ata_attr_state
{
	ATTRSTATE_NON_EXISTING,   // No such Attribute
	ATTRSTATE_NO_NORMVAL,     // Normalized value not valid
  	ATTRSTATE_BAD_THRESHOLD,  // Threshold not valid
  	ATTRSTATE_NO_THRESHOLD,   // Unknown or no threshold
  	ATTRSTATE_OK,             // Never failed
  	ATTRSTATE_FAILED_PAST,    // Failed in the past
  	ATTRSTATE_FAILED_NOW      // Failed now
};

// External handler function, for when a checksum is not correct.  Can
// simply return if no action is desired, or can print error messages
// as needed, or exit.  Is passed a string with the name of the Data
// Structure with the incorrect checksum.
void checksumwarning(const char *string);

// This are the meanings of the Self-test failure checkpoint byte.
// This is in the self-test log at offset 4 bytes into the self-test
// descriptor and in the SMART READ DATA structure at byte offset
// 371. These codes are not well documented.  The meanings returned by
// this routine are used (at least) by Maxtor and IBM. Returns NULL if
// not recognized.
const char *SelfTestFailureCodeName(unsigned char which);


#define MAX_ATTRIBUTE_NUM 256

// These are two of the functions that are defined in os_*.c and need
// to be ported to get smartmontools onto another OS.
// Moved to C++ interface
//int ata_command_interface(int device, smart_command_set command, int select, char *data);
//int escalade_command_interface(int fd, int escalade_port, int escalade_type, smart_command_set command, int select, char *data);
//int marvell_command_interface(int device, smart_command_set command, int select, char *data);
//int highpoint_command_interface(int device, smart_command_set command, int select, char *data);
//int areca_command_interface(int fd, int disknum, smart_command_set command, int select, char *data);


// Optional functions of os_*.c
#ifdef HAVE_ATA_IDENTIFY_IS_CACHED
// Return true if OS caches the ATA identify sector
//int ata_identify_is_cached(int fd);
#endif

// This function is exported to give low-level capability
int smartcommandhandler(int fd, smart_command_set command, int select, char *data);

// Print one self-test log entry.
bool ataPrintSmartSelfTestEntry(unsigned testnum, unsigned char test_type,
                                unsigned char test_status,
                                unsigned short timestamp,
                                uint64_t failing_lba,
//                                bool print_error_only, bool &print_header);
				bool print_error_only, bool print_header);

// Print Smart self-test log, used by smartctl and smartd.
int ataPrintSmartSelfTestlog(const ata_smart_selftestlog_t *data, bool allentries,
                             unsigned char fix_firmwarebug);

// Get number of sectors from IDENTIFY sector.
uint64_t get_num_sectors(const ata_identify_device_t *drive);

// Convenience function for formatting strings from ata_identify_device.
void format_ata_string(char *out, const char *in, int n, bool fix_swap);
extern inline void format_ata_string_in(char *out, const unsigned char *in, int n, bool fix_swap)
  { format_ata_string(out, (const char *)in, n, fix_swap); }

// Utility routines.
unsigned char checksum(const void * data);

void swapx2(unsigned short *p);
void swapx4(unsigned int *p);
void swapx8(uint64_t *p);

#endif	/* _DS_ATA_IOCTL_H */
