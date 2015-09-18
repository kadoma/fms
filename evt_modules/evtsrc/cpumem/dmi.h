#ifndef __DMI_H__
#define __DMI_H__

#include "cpumem.h"

enum { 
	DMI_MEMORY_ARRAY = 16,
	DMI_MEMORY_DEVICE = 17,
	DMI_MEMORY_ARRAY_ADDR = 19,
	DMI_MEMORY_MAPPED_ADDR = 20,
};

struct dmi_memdev_addr {
	struct dmi_entry header;
	unsigned start_addr;
	unsigned end_addr;
	unsigned short dev_handle;
	unsigned short memarray_handle;
	unsigned char row;	
	unsigned char interleave_pos;
	unsigned char interleave_depth;
	u64 start_addr64;
	u64 end_addr64;
} __attribute__((packed)); 

struct dmi_memarray {
	struct dmi_entry header;
	unsigned char location;
	unsigned char use;
	unsigned char error_correction;
	unsigned int maximum_capacity;
	unsigned short error_handle;
	short num_devices;
} __attribute__((packed));

struct dmi_memarray_addr {
	struct dmi_entry header;
	unsigned start_addr;
	unsigned end_addr;
	unsigned short array_handle;
	unsigned partition_width;
	u64 start_addr64;
	u64 end_addr64;
}  __attribute__((packed));


extern int opendmi(void);
extern void closedmi(void);

extern void checkdmi(void);
extern void dmi_decodeaddr(unsigned long addr, struct fms_cpumem *fc);
extern int dmi_sanity_check(void);
extern struct dmi_memdev **dmi_find_addr(unsigned long addr);


/* valid after opendmi: */
extern struct dmi_memdev **dmi_dimms; 
extern struct dmi_memdev_addr **dmi_ranges; 
extern struct dmi_memarray **dmi_arrays; 
extern struct dmi_memarray_addr **dmi_array_ranges; 

extern int do_dmi;

#endif
