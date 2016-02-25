/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	memdev.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-04
 * Description:	
 *				parse memory device. 
 *
 */

#include <stddef.h>

#include "memdev.h"
#include "logging.h"
#include "cmea_base.h"
#include "fmd_log.h"
#include "evt_process.h"

static char *form_factors[] = { 
	"?",
	"Other", "Unknown", "SIMM", "SIP", "Chip", "DIP", "ZIP", 
	"Proprietary Card", "DIMM", "TSOP", "Row of chips", "RIMM",
	"SODIMM", "SRIMM"
};
static char *memory_types[] = {
	"?",
	"Other", "Unknown", "DRAM", "EDRAM", "VRAM", "SRAM", "RAM",
	"ROM", "FLASH", "EEPROM", "FEPROM", "EPROM", "CDRAM", "3DRAM",
	"SDRAM", "SGRAM", "RDRAM", "DDR", "DDR2"
};

static char *type_details[16] = {
	"Reserved", "Other", "Unknown", "Fast-paged", "Static Column",
	"Pseudo static", "RAMBUS", "Synchronous", "CMOS", "EDO",
	"Window DRAM", "Cache DRAM", "Non-volatile", "Res13", "Res14", "Res15"
}; 

#define LOOKUP(array, val, buf) \
	((val) >= NELE(array) ? \
	 (sprintf(buf,"<%u>",(val)), (buf)) : \
	 (array)[val])

static unsigned 
dmi_dimm_size(unsigned short size, char *unit)
{
	unsigned mbflag = !(size & (1<<15));
	
	size &= ~(1<<15);
	strcpy(unit, "KB");
	if (mbflag) {
		unit[0] = 'M';
		if (size >= 1024) {
			unit[0] = 'G';
			size /= 1024;
		}
	}
	return size;
}

static void 
dump_type_details(char *buf, unsigned short td)
{
	int i;
	if (!td)
		return;
	for (i = 0; i < 16; i++) 
		if (td & (1<<i))
			sprintf(buf+strlen(buf), "%s ", type_details[i]);
}

void 
dump_memdev(int fd, char *buf, struct dmi_memdev *md, unsigned long addr)
{
	char tmp[20];
	char unit[10];
	char *s;

	if (md->header.length < 
			offsetof(struct dmi_memdev, manufacturer)) { 
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_WARNING,
				"Memory device for address %lx too short %u",
				addr, md->header.length);
		return;
	}	

	line_indent(fd);
	sprintf(buf, "%s ", LOOKUP(memory_types, md->memory_type, tmp));
	if (md->form_factor >= 3) 
		sprintf(buf+strlen(buf), "%s ", LOOKUP(form_factors, md->form_factor, tmp));
	if (md->speed != 0)
		sprintf(buf+strlen(buf), "%hu Mhz ", md->speed);
	dump_type_details(buf, md->type_details);
	sprintf(buf+strlen(buf), "Width %hu Data Width %hu Size %u %s\n",
		md->total_width, md->data_width, 
		dmi_dimm_size(md->size, unit), unit);
	fmd_log_write(fd, buf, strlen(buf));
	
#define DUMPSTR(n,x) \
	if (md->x) { \
		s = dmi_getstring(&md->header, md->x);	\
		if (s && *s && strcmp(s,"None")) {	\
			line_indent(fd);	\
			sprintf(buf, n ": %s\n", s); \
			fmd_log_write(fd, buf, strlen(buf));\
		}	\
	}
	
	DUMPSTR("Device Locator", device_locator);
	DUMPSTR("Bank Locator", bank_locator);
	if (md->header.length < offsetof(struct dmi_memdev, manufacturer))
		return;
	
	DUMPSTR("Manufacturer", manufacturer);
	DUMPSTR("Serial Number", serial_number);
	DUMPSTR("Asset Tag", asset_tag);
	DUMPSTR("Part Number", part_number);
}
