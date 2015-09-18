#ifndef __CPUMEM_EVTSRC_H__
#define __CPUMEM_EVTSRC_H__

#include "cpumem.h"
#include "cmes_base.h"
#include "logging.h"


typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef long long s64;
typedef int s32;
typedef short s16;
typedef char s8;


#define __u64 u64
#define __u32 u32
#define __u16 u16
#define __u8  u8
#define __s64 s64
#define __s32 s32
#define __s16 s16
#define __s8  s8


/* kernel structure: */

/* Fields are zero when not available */
struct mce {
	__u64 status;
	__u64 misc;
	__u64 addr;
	__u64 mcgstatus;
	__u64 ip;
	__u64 tsc;	/* cpu time stamp counter */
	__u64 time;	/* wall time_t when error was detected */
	__u8  cpuvendor;	/* cpu vendor as encoded in system.h */
	__u8  pad1;
	__u16 pad2;
	__u32 cpuid;	/* CPUID 1 EAX */
	__u8  cs;		/* code segment */
	__u8  bank;	/* machine check bank */
	__u8  cpu;	/* cpu number; obsolete; use extcpu now */
	__u8  finished;   /* entry is valid */
	__u32 extcpu;	/* linux cpu number that detected the error */
	__u32 socketid;	/* CPU socket ID */
	__u32 apicid;	/* CPU initial apic ID */
	__u64 mcgcap;	/* MCGCAP MSR: machine check capabilities of CPU */
};

struct mc_msg {
	char name[FMS_CPUMEM_MSG_NAME_LEN];
	__u64 id;
	char *mnemonic;
	char *desc;
	__s8 mem_ch;
	__s32 clevel;	/* cache level*/
	__s32 ctype;	/* cache type*/
	__u64 flags;
};

#define X86_VENDOR_INTEL		0
#define X86_VENDOR_CYRIX		1
#define X86_VENDOR_AMD			2
#define X86_VENDOR_UMC			3
#define X86_VENDOR_CENTAUR		5
#define X86_VENDOR_TRANSMETA	7
#define X86_VENDOR_NSC			8
#define X86_VENDOR_NUM			9

#define MCE_OVERFLOW 	0	/* bit 0 in flags means overflow */


#define MCE_GET_RECORD_LEN   _IOR('M', 1, int)
#define MCE_GET_LOG_LEN      _IOR('M', 2, int)
#define MCE_GETCLEAR_FLAGS   _IOR('M', 3, int)


#define sizeof_field(t, f) (sizeof(((t *)0)->f))
#define endof_field(t, f) (sizeof(((t *)0)->f) + offsetof(t, f))


#define MCE_MSG_MAXNUM		64

#define FMS_MCI_STATUS_UC(status)	((status & MCI_STATUS_UC) ? "_uc" : "_ce")


#ifdef TEST_CMES
extern int cmes_init_test();
extern struct list_head *cmes_test();
extern void cmes_release_test();
#endif	/* CMES_TEST */

#endif /* __CPUMEM_EVTSRC_H__ */
