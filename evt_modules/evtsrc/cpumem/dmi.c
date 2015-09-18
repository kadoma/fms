/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	dmi.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-04
 * Description:	
 *				parse dmi memory data.
 *
 */

#define _GNU_SOURCE 1
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/mman.h>

#include "cpumem_evtsrc.h"
#include "cpumem.h"
#include "dmi.h"

int do_dmi = 1;

struct anchor { 
	char str[4];	/* _SM_ */
	char csum;
	char entry_length;
	char major;
	char minor;
	short maxlength;
	char rev;
	char fmt[5];
	char str2[5]; /* _DMI_ */
	char csum2;
	unsigned short length;
	unsigned table;
	unsigned short numentries;
	char bcdrev;
} __attribute__((packed));

static struct dmi_entry *entries;
static int entrieslen;
static int numentries;
static int dmi_length;
static struct dmi_entry **handle_to_entry;

struct dmi_memdev **dmi_dimms; 
struct dmi_memarray **dmi_arrays;
struct dmi_memdev_addr **dmi_ranges; 
struct dmi_memarray_addr **dmi_array_ranges; 

static void collect_dmi_dimms(void);
static struct dmi_entry **dmi_collect(int type, int minsize, int *len);
static void dump_ranges(struct dmi_memdev_addr **, struct dmi_memdev **);
static void dmi_get_memarray_addr64(struct dmi_memarray_addr* marray, u64 *s_addr, u64 *e_addr);
static void dmi_get_memdev_addr64(struct dmi_memdev_addr* mdev, u64 *s_addr, u64 *e_addr);

static unsigned 
checksum(unsigned char *s, int len)
{
	unsigned char csum = 0;
	int i;
	for (i = 0; i < len; i++)
		csum += s[i];
	return csum;
}

/* Check if entry is valid */
static int 
check_entry(struct dmi_entry *e, struct dmi_entry **next) 
{
	char *end = (char *)entries + dmi_length;
	char *s;
	
	if (!e)
		return 0;
	
 	s = (char *)e + e->length;
	
//	wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG,
//		"length %d handle %x", e->length, e->handle);
	
	do {
		while (s < end-1 && *s)
			s++;
		if (s >= end-1) { 
			wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG,
				"handle %x length %d truncated",
				e->handle, e->length);
			return 0;
		}
		s++;
	} while (*s);
	
	if (s >= end) 
		*next = NULL;
	else
		*next = (struct dmi_entry *)(s + 1);
	
	return 1;
}

static void 
fill_handles(void)
{
	int i;
	struct dmi_entry *e, *next;
	
	e = entries;
	handle_to_entry = xcalloc(1, sizeof(void *) * 0xffff);
	for (i = 0; i < numentries; i++, e = next) { 
		if (!check_entry(e, &next))
			break;
		handle_to_entry[e->handle] = e; 
	}
}

static int 
get_efi_base_addr(size_t *address)
{
	FILE *efi_systab;
	const char *filename;
	char linebuf[64];
	int ret = 0;

	*address = 0; /* Prevent compiler warning */

	/* Linux 2.6.7 and up: /sys/firmware/efi/systab */
	filename = "/sys/firmware/efi/systab";
	if ((efi_systab = fopen(filename, "r")) != NULL)
		goto check_symbol;

	/* Linux up to 2.6.6: /proc/efi/systab */
	filename = "/proc/efi/systab";
	if ((efi_systab = fopen(filename, "r")) != NULL)
		goto check_symbol;

	/* Failed to open EFI interfaces */
	wr_log(CMES_LOG_DOMAIN, WR_LOG_WARNING,
			"Failed to open EFI interfaces");
	return ret;

check_symbol:
	while ((fgets(linebuf, sizeof(linebuf) - 1, efi_systab)) != NULL) {
		char *addrp = strchr(linebuf, '=');
		if (!addrp)
			break;
		*(addrp++) = '\0';

		if (strcmp(linebuf, "SMBIOS") == 0) {
			*address = strtoul(addrp, NULL, 0);
			ret = 1;
			break;
		}
	}

	if (fclose(efi_systab) != 0)
		wr_log(CMES_LOG_DOMAIN, WR_LOG_ERROR,
			"%s, %s", filename, strerror(errno));
	
	if (!ret || !*address){
		wr_log(CMES_LOG_DOMAIN, WR_LOG_WARNING,
			"No valid SMBIOS entry point: Continue without DMI decoding");
		return 0;
	}

	wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG, 
			"%s: SMBIOS entry point at 0x%08lx", 
			filename,
			(unsigned long)*address);
	return ret;
}

static int 
is_efi_based_system()
{
	const char *filename;
	
	/* Linux 2.6.7 and up: /sys/firmware/efi/systab */
	filename = "/sys/firmware/efi/systab";
	if (access(filename, F_OK) == 0)
		return 1;

	/* Linux up to 2.6.6: /proc/efi/systab */
	filename = "/proc/efi/systab";
	if (access(filename, F_OK) == 0)
		return 1;

	return 0;
}

int 
opendmi(void)
{
	struct anchor *a, *abase;
	void *p, *q;
	int pagesize = getpagesize();
	int memfd; 
	unsigned corr;
	int err = -1;
	const int segsize = 0x10000;
	size_t entry_point_addr = 0;
	size_t length = 0;

	if (entries)
		return 0;
	
	memfd = open("/dev/mem", O_RDONLY);
	if (memfd < 0) { 
		wr_log(CMES_LOG_DOMAIN, WR_LOG_WARNING,
				"Cannot open /dev/mem for DMI decoding: %s",
				strerror(errno));
		return -1;
	}	

	/*
	 * On EFI-based systems, the SMBIOS Entry Point structure can be
	 * located by looking in the EFI Configuration Table.
	 */
	if (is_efi_based_system()) {
		if (get_efi_base_addr(&entry_point_addr)) {
			size_t addr_start = round_down(entry_point_addr, pagesize);
			size_t addr_end = round_up(entry_point_addr + 0x20, pagesize);
			length = addr_end - addr_start;

			/* mmap() the address of SMBIOS structure table entry point. */
			abase = mmap(NULL, length, PROT_READ, MAP_SHARED, memfd,
						addr_start);
			if (abase == (struct anchor *)-1) {
				wr_log(CMES_LOG_DOMAIN, WR_LOG_WARNING,
					"Cannot mmap 0x%lx for efi mode: %s",
					(unsigned long)entry_point_addr,
					strerror(errno));
				goto legacy;
			}
			a = (struct anchor*)((char*)abase + (entry_point_addr - addr_start));
			goto fill_entries;
		} else {
			return -1;
		}
	}

legacy:
	/*
	 * On non-EFI systems, the SMBIOS Entry Point structure can be located
	 * by searching for the anchor-string on paragraph (16-byte) boundaries
	 * within the physical memory address range 000F0000h to 000FFFFFh
	 */
	length = segsize - 1;
	abase = mmap(NULL, length, PROT_READ, MAP_SHARED, memfd, 0xf0000);

	if (abase == (struct anchor *)-1) {
		wr_log(CMES_LOG_DOMAIN, WR_LOG_WARNING,
			"Cannot mmap 0xf0000 for legacy mode: %s",
			strerror(errno));
		goto out;
	}   

	for (p = abase, q = p + segsize; p < q; p += 0x10) { 
		if (!memcmp(p, "_SM_", 4) && 
		    (checksum(p, ((struct anchor *)p)->entry_length) == 0)) 
			break;
	}

	if (p >= q) {
		wr_log(CMES_LOG_DOMAIN, WR_LOG_WARNING,
			"Cannot find SMBIOS DMI tables");
		goto out;
	}

	a = p;

fill_entries:
	wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG,
		"DMI tables at %x, %u bytes, %u entries", 
		a->table, a->length, a->numentries);
	
	corr = a->table - round_down(a->table, pagesize); 
	entrieslen = round_up(a->table + a->length, pagesize) -
		round_down(a->table, pagesize);
 	entries = mmap(NULL, entrieslen, 
		   		PROT_READ, MAP_SHARED, memfd, 
			round_down(a->table, pagesize));
	if (entries == (struct dmi_entry *)-1) {
		wr_log(CMES_LOG_DOMAIN, WR_LOG_WARNING,
			"Cannot mmap SMBIOS tables at %x", a->table);
		goto out_mmap;
	}
	entries = (struct dmi_entry *)(((char *)entries) + corr);
	numentries = a->numentries;
	dmi_length = a->length;
	fill_handles();
	collect_dmi_dimms(); 
	err = 0;

out_mmap:
	munmap(abase, length);
out:
	close(memfd);
	return err;	
}

static int 
cmp_range(const void *a, const void *b)
{
	struct dmi_memdev_addr *ap = *(struct dmi_memdev_addr **)a; 
	struct dmi_memdev_addr *bp = *(struct dmi_memdev_addr **)b;
	u64 asa, aea, bsa, bea;
	s64 res;

	dmi_get_memdev_addr64(ap, &asa, &aea);
	dmi_get_memdev_addr64(bp, &bsa, &bea);

	if ((s64)asa < 0 && (s64)bea > 0)
		return 1;
	if ((s64)asa > 0 && (s64)bea < 0)
		return -1;
	
	res = asa - bea;
	if (res > 0)
		return 1;
	else if (res < 0)
		return -1;
	
	return 0; 
}

static int 
cmp_arr_range(const void *a, const void *b)
{
	struct dmi_memarray_addr *ap = *(struct dmi_memarray_addr **)a; 
	struct dmi_memarray_addr *bp = *(struct dmi_memarray_addr **)b;
	u64 asa, aea, bsa, bea;
	s64 res;
	
	dmi_get_memarray_addr64(ap, &asa, &aea);
	dmi_get_memarray_addr64(bp, &bsa, &bea);

	if ((s64)asa < 0 && (s64)bea > 0)
		return 1;
	if ((s64)asa > 0 && (s64)bea < 0)
		return -1;
	
	res = asa - bea;
	if (res > 0)
		return 1;
	else if (res < 0)
		return -1;
	
	return 0; 
}

#define COLLECT(var, id, ele) {					 \
	typedef typeof (**(var)) T;					 \
	var = (T **)dmi_collect(id,					 \
		        offsetof(T, ele) + sizeof_field(T, ele), \
			&len);						  \
}

static void
collect_dmi_dimms(void)
{
	int len; 
	
	COLLECT(dmi_ranges, DMI_MEMORY_MAPPED_ADDR, dev_handle);
	qsort(dmi_ranges, len, sizeof(struct dmi_entry *), cmp_range);
	COLLECT(dmi_dimms, DMI_MEMORY_DEVICE, device_locator);
	dump_ranges(dmi_ranges, dmi_dimms);	//DEBUG
	COLLECT(dmi_arrays, DMI_MEMORY_ARRAY, location);
	COLLECT(dmi_array_ranges, DMI_MEMORY_ARRAY_ADDR, array_handle); 
	qsort(dmi_array_ranges, len, sizeof(struct dmi_entry *),cmp_arr_range);
}

#undef COLLECT

static struct dmi_entry **
dmi_collect(int type, int minsize, int *len)
{
	struct dmi_entry **r; 
	struct dmi_entry *e, *next;
	int i, k;
	
	r = xcalloc(1, sizeof(struct dmi_entry *) * (numentries + 1));
	k = 0;
	e = entries;
	next = NULL;
	for (i = 0; i < numentries; i++, e = next) { 
		if (!check_entry(e, &next))
			break; 
		if (e->type != type)
			continue;
		if (e->length < minsize) { 
			wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG, 
					"hnd %x size %d expected %d",
					e->handle, e->length, minsize);
			continue;
		}		
		if (type == DMI_MEMORY_DEVICE && 
		    ((struct dmi_memdev *)e)->size == 0) { 
		    wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG,
				"entry %x disabled", e->handle);
			continue;
		}
		r[k++] = e; 
	}
	
	*len = k;
	return r;
} 

#define FAILED " SMBIOS DIMM sanity check failed"

int 
dmi_sanity_check(void)
{
	int i, k;
	int numdmi_dimms = 0;
	int numranges = 0;

	if (dmi_ranges[0] == NULL)
		return 0;

	for (k = 0; dmi_dimms[k]; k++)
		numdmi_dimms++;

	/* Do we have multiple ranges? */
	for (k = 1; dmi_ranges[k]; k++) {
		u64 s_addr, s_addr1, e_addr, e_addr1;

		dmi_get_memdev_addr64(dmi_ranges[k-1], &s_addr, &e_addr);
		dmi_get_memdev_addr64(dmi_ranges[k], &s_addr1, &e_addr1);
		if (s_addr1 <= e_addr) { 
			return 0;
		}
		if (s_addr1 >= e_addr)
			numranges++;
	}
	if (numranges == 1 && numdmi_dimms > 2) {
		wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG, 
			"Not enough unique address ranges." FAILED);
		return 0;
	}

	/* Unique locators? */
	for (k = 0; dmi_dimms[k]; k++) {
		char *loc;
		loc  = dmi_getstring(&dmi_dimms[k]->header,
				     dmi_dimms[k]->device_locator);
		if (!loc) {
			wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG, 
				"Missing locator." FAILED);
			return 0; 
		}
		for (i = 0; i < k; i++) {
			char *b = dmi_getstring(&dmi_dimms[i]->header,
						dmi_dimms[i]->device_locator);
			if (!strcmp(b, loc)) {
				wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG, 
					"Ambigious locators `%s'<->`%s'."
					FAILED, b, loc);
				return 0;
			}
		}
	}
				
	return 1;
}

#define DMIGET(p, member) \
(offsetof(typeof(*(p)), member) + sizeof((p)->member) <= (p)->header.length ? \
	(p)->member : 0)

static void
dmi_get_memdev_addr64(struct dmi_memdev_addr* mdev, u64 *s_addr, u64 *e_addr)
{
	u64 sa = mdev->start_addr;
	u64 ea = mdev->end_addr;
	
	if (mdev->header.length >= 0x23 && sa == 0xFFFFFFFF) {
		*s_addr = mdev->start_addr64;
		*e_addr = mdev->end_addr64;
	} else { 
		*s_addr = (((u64)sa >> 2) << 12) + ((sa & 0x3) << 10);
		*e_addr = (((u64)ea >> 2) << 12) + ((ea & 0x3) << 10) + 0x3FF;
	}
}

static void
dmi_get_memarray_addr64(struct dmi_memarray_addr* marray, u64 *s_addr, u64 *e_addr)
{
	u64 sa = marray->start_addr;
	u64 ea = marray->end_addr;
	
	if (marray->header.length >= 0x1F && sa == 0xFFFFFFFF) {
		*s_addr = marray->start_addr64;
		*e_addr = marray->end_addr64;
	} else { 
		*s_addr = (((u64)sa >> 2) << 12) + ((sa & 0x3) << 10);
		*e_addr = (((u64)ea >> 2) << 12) + ((ea & 0x3) << 10) + 0x3FF;
	}
}

static void 
dump_ranges(struct dmi_memdev_addr **ranges, struct dmi_memdev **dmi_dimms)
{
	int i;
	wr_loglevel_t ll;

	ll = wr_log_get_loglevel();

	if(ll == WR_LOG_DEBUG) {
		wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG, "RANGES:");
		for (i = 0; ranges[i]; i++) {
			u64 sa, ea;
			dmi_get_memdev_addr64(ranges[i], &sa, &ea);
			wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG,
				"type:%d length:%d handle:%x"
				"    range %llx-%llx h %x a %x row %u ilpos %u ildepth %u",
				ranges[i]->header.type,
				ranges[i]->header.length,
				ranges[i]->header.handle,
				sa,
				ea,
				ranges[i]->dev_handle,
				DMIGET(ranges[i], memarray_handle),
				DMIGET(ranges[i], row),
				DMIGET(ranges[i], interleave_pos),
				DMIGET(ranges[i], interleave_depth));
		}
		
		wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG, "DMI_DIMMS:");
		for (i = 0; dmi_dimms[i]; i++) 
			wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG,
				"type:%d length:%d handle:%x"
				"    dimm width %u datawidth %u size %u set %u", 
				dmi_dimms[i]->header.type,
				dmi_dimms[i]->header.length,
				dmi_dimms[i]->header.handle,
				dmi_dimms[i]->total_width,
				DMIGET(dmi_dimms[i],data_width),
				DMIGET(dmi_dimms[i],size),
				DMIGET(dmi_dimms[i],device_set));
	}
}

struct dmi_memdev **
dmi_find_addr(unsigned long addr)
{
	struct dmi_memdev **devs; 
	int i, k;
	
	devs = xcalloc(1, sizeof(void *) * (numentries+1));
	k = 0;
	for (i = 0; dmi_ranges[i]; i++) {
		u64 s_addr, e_addr;
		struct dmi_memdev_addr *da = dmi_ranges[i];

		dmi_get_memdev_addr64(da, &s_addr, &e_addr);
		if (addr < s_addr || addr >= e_addr)
			continue;
		
		devs[k] = (struct dmi_memdev *)handle_to_entry[da->dev_handle];
		if (devs[k]) 
			k++;
	} 
	
	devs[k] = NULL;
	return devs;
}

void 
dmi_decodeaddr(unsigned long addr, struct fms_cpumem *fc)
{
	struct dmi_memdev **devs = dmi_find_addr(addr);
	
	if (devs[0]) {
		fc->memdev = devs[0];
	} else {
		fc->memdev = NULL;
		xfree(devs);
		wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG,
			"No DIMM found for %lx in SMBIOS", addr);
	}
} 

void 
checkdmi(void)
{			
	if (opendmi() < 0) {
		do_dmi = 0;
		return; 
	}
	
	do_dmi = dmi_sanity_check();
}

#define FREE(x) free(x), (x) = NULL

void 
closedmi(void)
{
	if (!entries) 
		return;
	munmap(entries, entrieslen);
	entries = NULL;
	FREE(dmi_dimms);
	FREE(dmi_arrays);
	FREE(dmi_ranges);
	FREE(dmi_array_ranges);
	FREE(handle_to_entry);
}
