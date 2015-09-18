#ifndef __CPUMEM_H__
#define __CPUMEM_H__

#include <stdint.h>

#include "wrap.h"
#include "fmd_common.h"

#define xcalloc(nmemb, size)	def_calloc(nmemb, size)
#define xfree(ptr)		def_free(ptr)

#ifndef __unused
#define __unused __attribute__ ((used))
#endif

#define NELE(x) (sizeof(x)/sizeof(*(x)))

#define round_up(x,y) (((x) + (y) - 1) & ~((y)-1))
#define round_down(x,y) ((x) & ~((y)-1))

#define BITS_PER_INT (sizeof(unsigned) * 8)
#define BITS_PER_LONG (sizeof(unsigned long) * 8)


#define FMS_CPUMEM_PAGE_ERROR			0X00000001
#define FMS_CPUMEM_UC					0X00000002		/* uncorrected error */
#define FMS_CPUMEM_YELLOW				0X00000004		

#define FMS_CPUMEM_MSG_NAME_LEN			32
#define FMS_CPUMEM_FAULT_DESC_LEN		1024

struct fms_cpumem {
	uint32_t socketid;	/* CPU socket ID */
	uint32_t cpu;	/* linux cpu number that detected the error */
	uint64_t status;
	uint64_t misc;
	uint64_t addr;
	uint64_t maddr;		/* fault memory addr, page alignment, used to statistics page error */
	uint64_t mcgstatus;
	uint64_t ip;
	uint64_t tsc;	/* cpu time stamp counter */
	uint64_t time;	/* wall time_t when error was detected */
	uint32_t apicid;	/* CPU initial apic ID */
	uint32_t cpuid;	/* CPUID 1 EAX */
	uint64_t mcgcap;	/* MCGCAP MSR: machine check capabilities of CPU */
	char mnemonic[FMS_PATH_MAX];
	char desc[FMS_CPUMEM_FAULT_DESC_LEN];
	struct dmi_memdev *memdev;
	uint64_t flags;
	uint8_t	cs;		/* code segment */
	uint8_t	bank;		/* machine check bank */
	uint8_t	cpuvendor;	/* cpu vendor as encoded in system.h */
	int8_t	mem_ch;		/* memory channel */
	int32_t clevel;	/* cache level*/
	int32_t ctype;		/* cache type*/
	char fname[FMS_CPUMEM_MSG_NAME_LEN]; /* fault name */
	uint64_t fid; /* fault id */
};

struct dmi_entry { 
	unsigned char type;
	unsigned char length;
	unsigned short handle;
};

struct dmi_memdev {
	struct dmi_entry header;
	unsigned short array_handle;
	unsigned short memerr_handle;
	unsigned short total_width;
	unsigned short data_width;
	unsigned short size;
	unsigned char form_factor;
	unsigned char device_set;
	unsigned char device_locator;
	unsigned char bank_locator;
	unsigned char memory_type;
	unsigned short type_details;
	unsigned short speed;
	unsigned char manufacturer;
	unsigned char serial_number;
	unsigned char asset_tag;
	unsigned char part_number;	
} __attribute__((packed));

/* Software defined banks */
#define MCE_EXTENDED_BANK	128

#define MCE_THERMAL_BANK	(MCE_EXTENDED_BANK + 0)
#define MCE_TIMEOUT_BANK    (MCE_EXTENDED_BANK + 90)

#define MCI_THRESHOLD_OVER  (1ULL<<48)  /* threshold error count overflow */

#define MCI_STATUS_VAL   	(1ULL<<63)  /* valid error */
#define MCI_STATUS_OVER  	(1ULL<<62)  /* previous errors lost */
#define MCI_STATUS_UC    	(1ULL<<61)  /* uncorrected error */
#define MCI_STATUS_EN    	(1ULL<<60)  /* error enabled */
#define MCI_STATUS_MISCV 	(1ULL<<59)  /* misc error reg. valid */
#define MCI_STATUS_ADDRV 	(1ULL<<58)  /* addr reg. valid */
#define MCI_STATUS_PCC   	(1ULL<<57)  /* processor context corrupt */
#define MCI_STATUS_S	 	(1ULL<<56)  /* signalled */
#define MCI_STATUS_AR	 	(1ULL<<55)  /* action-required */
#define MCI_STATUS_FWST  	(1ULL<<37)  /* Firmware updated status indicator */

#define MCG_STATUS_RIPV  	(1ULL<<0)   /* restart ip valid */
#define MCG_STATUS_EIPV  	(1ULL<<1)   /* eip points to correct instruction */
#define MCG_STATUS_MCIP  	(1ULL<<2)   /* machine check in progress */
#define MCG_STATUS_LMCES 	(1ULL<<3)   /* local machine check signaled */

#define MCG_CMCI_P			(1ULL<<10)   /* CMCI supported */
#define MCG_TES_P			(1ULL<<11)   /* Yellow bit cache threshold supported */
#define MCG_SER_P			(1ULL<<24)   /* MCA recovery / new status */
#define MCG_ELOG_P			(1ULL<<26)   /* Extended error log supported */
#define MCG_LMCE_P			(1ULL<<27)   /* Local machine check supported */

/* Don't forget to update cpumem_evtsrc.c:cputype_name[] too */
enum cputype {
	CPU_GENERIC,
	CPU_P6OLD,
	CPU_CORE2, /* 65nm and 45nm */
	CPU_K8,
	CPU_P4,
	CPU_NEHALEM,
	CPU_DUNNINGTON,
	CPU_TULSA,
	CPU_INTEL, /* Intel architectural errors */
	CPU_XEON75XX, 
	CPU_SANDY_BRIDGE, 
	CPU_SANDY_BRIDGE_EP, 
	CPU_IVY_BRIDGE, 
	CPU_IVY_BRIDGE_EPEX, 
	CPU_HASWELL,
	CPU_HASWELL_EPEX,
	CPU_BROADWELL,
	CPU_KNIGHTS_LANDING,
	CPU_ATOM,
};

#define CASE_INTEL_CPUS \
	case CPU_P6OLD: \
	case CPU_CORE2: \
	case CPU_NEHALEM: \
	case CPU_DUNNINGTON: \
	case CPU_TULSA:	\
	case CPU_P4: \
	case CPU_INTEL: \
	case CPU_XEON75XX: \
	case CPU_SANDY_BRIDGE_EP: \
	case CPU_SANDY_BRIDGE: \
	case CPU_IVY_BRIDGE: \
	case CPU_IVY_BRIDGE_EPEX: \
	case CPU_HASWELL: \
	case CPU_HASWELL_EPEX: \
	case CPU_BROADWELL: \
	case CPU_KNIGHTS_LANDING


extern char *processor_flags;
extern enum cputype cputype;
extern double cpumhz;
extern void parse_cpuid(uint32_t cpuid, uint32_t *family, uint32_t *model);
extern char *dmi_getstring(struct dmi_entry *e, unsigned number);
extern int is_intel_cpu(int cpu);

#endif
