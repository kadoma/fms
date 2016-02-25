/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	evt_process.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-04
 * Description:	
 *				Machine Check Event Process. 
 *
 */

#include <stdint.h> 
#include <errno.h>
#include <regex.h>
#include <unistd.h>
#include <time.h>
#include <linux/kernel-page-flags.h>

#include "evt_process.h"
#include "cmea_base.h"
#include "libkfm_cpumem.h"
#include "cpumem.h"
#include "logging.h"
#include "memdev.h"
#include "sysfs.h"
#include "tsc.h"
#include "cache.h"
#include "fmd_log.h"

extern double cpumhz;

static int evt_handle_cache(struct cmea_evt_data *edata, void *data);
static int evt_handle_tlb(struct cmea_evt_data *edata, void *data);
static int evt_handle_bus(struct cmea_evt_data *edata, void *data);
static int evt_handle_pcu(struct cmea_evt_data *edata, void *data);
static int evt_handle_qpi(struct cmea_evt_data *edata, void *data);
static int evt_handle_mc(struct cmea_evt_data *edata, void *data);
static int evt_handle_mempage(struct cmea_evt_data *edata, void *data);


static struct evt_handler {
	char *evt_type;
	int (*handler)(struct cmea_evt_data *, void *);
} g_evt_handler[] = {
	{"cache", 		evt_handle_cache 	},
	{"tlb", 		evt_handle_tlb 		},
//	{"bus",         evt_handle_bus      },  //todo.....
//	{"pcu",         evt_handle_pcu      },  //todo.....
	{"qpi",         evt_handle_qpi      },
//	{"mc",          evt_handle_mc       },  //todo.....
	{"mempage", 	evt_handle_mempage 	},
	/* end */
	{NULL, 			NULL}
};

/*
 *	@evt_type: event type, cache tlb bus....
 *	@edata: fmd event data
 * 	@data: cpumem event source private data
 */
int
cmea_evt_processing(char *evt_type, struct cmea_evt_data *edata, void *data)
{
	int i;
	char *etype = NULL;

	for (i = 0; g_evt_handler[i].evt_type; i++) {
		etype = g_evt_handler[i].evt_type;
		if (strncmp(etype, evt_type, strlen(etype)) == 0) {
			wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG, 
				"handle cpuemem event type[%s]", evt_type);
			return g_evt_handler[i].handler(edata, data);
		}
	}
	
	wr_log(CMEA_LOG_DOMAIN, WR_LOG_WARNING, 
		"event type[%s] is no handler", evt_type);
	return LIST_REPAIRED_FAILED;
}

static int 
memory_page_offline(uint64_t addr)
{
	int ret;
	struct kfm_mempage_u mempage;
	
	/* if page already offline, return success. */
	memset(&mempage, 0, sizeof mempage);
	mempage.addr = addr;
	if (kfm_mempage_get_info(&mempage)) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG, 
			"failted to get mempage[0x%llx], %s",
			addr,
			strerror(errno));
		return -1;
	}
	if (!(mempage.flags >> KPF_NOPAGE & 1) &&
		(mempage.flags >> KPF_HWPOISON & 1)) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG, 
			"mempage[0x%llx] already offline", addr);
		return 0;
	}
	
	ret = sysfs_write("/sys/devices/system/memory/soft_offline_page", 
		"%#llx", (unsigned long long)addr);

	if(ret > 0)
		return 0;

	return -1;
}

static int
is_cpu_offline(int cpu)
{
	FILE *f;
	char *line = NULL;
	size_t linesz = 0;
	int u;

	f = fopen("/proc/cpuinfo", "r");
	if (!f) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_ERROR, 
			"failed to open /proc/cpuinfo, %s",
			strerror(errno));

		return 0;
	}

	while (getdelim(&line, &linesz, '\n', f) > 0) {
		if (sscanf(line, "processor : %u\n", &u) == 1) { 
			if (u == cpu) {
				xfree(line);
				fclose(f);
				return 0;
			}	
		}
	}
	
	xfree(line);
	fclose(f);
	return 1;
}

static int
do_cpu_offline(int cpu)
{
	char buf[256];
	int ret;

	if (is_cpu_offline(cpu)) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG, 
			"CPU %d already is offine", cpu);
		return 0;
	}
	
	sprintf(buf, "/sys/devices/system/cpu/cpu%d/online", cpu);

	wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG, 
		"%s", buf);
	
	if (!sysfs_available(buf, F_OK)) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_WARNING, 
			"CPU %d is no allowed offline", cpu);
		return -1;
	}
	
	ret = sysfs_write(buf, "0"); /* 0: offline, 1: online */

	if(ret > 0)
		return 0;

	return -1;
}


#define BITS_PER_U (sizeof(unsigned) * 8)
#define test_bit(i, a) (((unsigned *)(a))[(i) / BITS_PER_U] & (1U << ((i) % BITS_PER_U)))

static int
cpu_offine_on_cache(int cpu, unsigned clevel, unsigned ctype)
{
	unsigned *cpumask;
	int cpumasklen;
	int i;
	int r =0;
	int ret = 0;
		
	if (cache_to_cpus(cpu, clevel, ctype, &cpumasklen, &cpumask) == 0)
	{
		for (i = 0; i < cpumasklen * 8; i++) {
			if (test_bit(i, cpumask)) {
				r = do_cpu_offline(i);

				/* write log, Easy to see */ 
				if(!r)
					wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
						"cpu %d offline sucess", 
						i);
				else
					wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
						"cpu %d offline fail, %s", 
						i, strerror(errno));
				
				/* try to offline all fault CPUs, if there is a failure as failure */
				if(r)
					ret = -1;
			}
		}
	} else { /* no find cache */
		ret = do_cpu_offline(cpu);

		/* write log, Easy to see */ 
		if(!ret)
			wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
				"cpu %d offline sucess", 
				cpu);
		else
			wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
				"cpu %d offline fail, %s", 
				cpu, strerror(errno));
	}

	return ret;
}

static inline int
__handle_cache(struct cmea_evt_data *edata, void *data)
{
	int ret = LIST_REPAIRED_FAILED;
	struct fms_cpumem *fc;
	struct kfm_cache_u cache;

	if(!edata || !data)
		return ret;
	
	fc = (struct fms_cpumem*)data;
	switch (edata->cpu_action) {
	case 1:
		/* flush cache */
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG, "flush cpu %d cache", fc->cpu);
		memset(&cache, 0, sizeof cache);
		cache.cpu = fc->cpu;
		if (kfm_cache_flush(&cache) == 0)
			ret = LIST_REPAIRED_SUCCESS;
		else
			ret = LIST_REPAIRED_FAILED;
		break;
	case 2:
		/* cpu offline */
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG, "CPU offline, cpu: %u, level: %d, type: %d", 
			fc->cpu, fc->clevel, fc->ctype);
		
		if(cpu_offine_on_cache(fc->cpu, fc->clevel, fc->ctype) == 0)
			ret = LIST_ISOLATED_SUCCESS;
		else
			ret = LIST_ISOLATED_FAILED;
		break;
	}
	
	return ret;
}

static int 
evt_handle_cache(struct cmea_evt_data *edata, void *data)
{
	wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG, "handle cache fault");
	
	return __handle_cache(edata, data);
}

static int 
evt_handle_tlb(struct cmea_evt_data *edata, void *data)
{
	wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG, "handle tlb fault");
	
	return __handle_cache(edata, data);
}

static int 
evt_handle_bus(struct cmea_evt_data *edata, void *data)
{
	struct fms_cpumem *fc;
	int cpu;
	
	if(!edata || !data)
		return -1;

	fc = (struct fms_cpumem*)data;
	cpu = fc->cpu;
	
	return do_cpu_offline(cpu);
}

static int 
evt_handle_pcu(struct cmea_evt_data *edata, void *data)
{
	struct fms_cpumem *fc;
	int cpu;
	
	if(!edata || !data)
		return -1;

	fc = (struct fms_cpumem*)data;
	cpu = fc->cpu;
	
	return do_cpu_offline(cpu);
}

static int 
evt_handle_qpi(struct cmea_evt_data *edata, void *data)
{
	struct fms_cpumem *fc;
	int cpu;
	int ret = LIST_REPAIRED_FAILED;
	
	if (!edata || !data)
		return ret;

	fc = (struct fms_cpumem*)data;
	cpu = fc->cpu;
	
	if (do_cpu_offline(cpu) == 0)
		ret = LIST_ISOLATED_SUCCESS;
	else
		ret = LIST_ISOLATED_FAILED;

	return ret;
}

static int 
evt_handle_mc(struct cmea_evt_data *edata, void *data)
{
	struct fms_cpumem *fc;
	int cpu;
	
	if(!edata || !data)
		return -1;

	fc = (struct fms_cpumem*)data;
	cpu = fc->cpu;
	
	return do_cpu_offline(cpu);
}

static int 
evt_handle_mempage(struct cmea_evt_data *edata, void *data)
{	
	int ret = LIST_REPAIRED_FAILED;
	struct fms_cpumem *fc;

	if (!data) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_ERROR, 
			"data is null in handle mempage");
		return ret;
	}
	
	fc = (struct fms_cpumem*)data;

	if (fc->flags & FMS_CPUMEM_PAGE_ERROR) {
		ret = memory_page_offline(fc->addr);

		/* write log, Easy to see */ 
		if (!ret) {
			wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
				"mempage %llx offline sucess", 
				fc->maddr);
			ret = LIST_ISOLATED_SUCCESS;
		} else {
			wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
				"mempage %llx offline fail, %s", 
				fc->maddr, strerror(errno));
			ret = LIST_ISOLATED_FAILED;
		}
	}
		
	return ret;
}


/* process, for log */
static char *
extended_bankname(unsigned bank) 
{
	static char buf[64];
	
	switch (bank) { 
	case MCE_THERMAL_BANK:
		return "THERMAL EVENT";
	case MCE_TIMEOUT_BANK:
		return "Timeout waiting for exception on other CPUs";

		/* add more extended banks here */
	
	default:
		sprintf(buf, "Undecoded extended event %x", bank);
		return buf;
	} 
}

char *
intel_bank_name(int num)
{
	static char bname[64];
	sprintf(bname, "BANK %d", num);
	return bname;
}

static char *
bankname(unsigned bank, enum cputype cputype) 
{ 
	static char numeric[64] = {0};
	
	if (bank >= MCE_EXTENDED_BANK) 
		return extended_bankname(bank);

	switch (cputype) { 
	CASE_INTEL_CPUS:
		return intel_bank_name(bank);
	/* add banks of other cpu types here */
	default:
		sprintf(numeric, "BANK %d", bank); 
		return numeric;
	}

	return numeric;
} 

static void 
print_tsc(int fd, char *buf, int cpunum, __u64 tsc, unsigned long time, 
        enum cputype cputype) 
{ 
	int ret = -1;
	char *buffer = NULL;

	if (!time) 
		ret = decode_tsc_current(&buffer, cpunum, cputype, cpumhz, tsc);

	line_indent(fd);
	sprintf(buf, "TSC: %llx %s\n", tsc, ret >= 0 && buffer ? buffer : "");
	fmd_log_write(fd, buf, strlen(buf));
	if(buffer)
		free(buffer);
}

static void
print_mcg(int fd, char *buf, __u64 mcgstatus)
{
	line_indent(fd);
	sprintf(buf, "MCG status:");
	if (mcgstatus & MCG_STATUS_RIPV)
		sprintf(buf+strlen(buf), " RIPV");
	if (mcgstatus & MCG_STATUS_EIPV)
		sprintf(buf+strlen(buf), " EIPV");
	if (mcgstatus & MCG_STATUS_MCIP)
		sprintf(buf+strlen(buf), " MCIP");
	if (mcgstatus & MCG_STATUS_LMCES)
		sprintf(buf+strlen(buf), " LMCE");
	sprintf(buf+strlen(buf), "\n");
	fmd_log_write(fd, buf, strlen(buf));
}

static const char *arstate[4] = { 
	[0] = "UCNA",		/* UnCorrected No Action reauired */
	[1] = "AR", 		/* Action Required  */
	[2] = "SRAO",		/* Software Recoverable Action Optional */
	[3] = "SRAR"		/* Software Recoverable Action Required */
};

static void
print_mci(int fd, char *buf, __u64 status, unsigned mcgcap)
{	
	line_indent(fd);
	sprintf(buf, "MCi status:\n");
	fmd_log_write(fd, buf, strlen(buf));
	
	if (!(status & MCI_STATUS_VAL)) {
		line_indent(fd);
		sprintf(buf, "  Machine check not valid\n");
		fmd_log_write(fd, buf, strlen(buf));
	}
	
	if (status & MCI_STATUS_OVER) {
		line_indent(fd);
		sprintf(buf, "  Error overflow\n");
		fmd_log_write(fd, buf, strlen(buf));
	}	

	line_indent(fd);
	if (status & MCI_STATUS_UC) 
		sprintf(buf, "  Uncorrected error\n");
	else
		sprintf(buf, "  Corrected error\n");
	fmd_log_write(fd, buf, strlen(buf));
	
	if (status & MCI_STATUS_EN) {
		line_indent(fd);
		sprintf(buf, "  Error enabled\n");
		fmd_log_write(fd, buf, strlen(buf));
	}
	
	if (status & MCI_STATUS_MISCV) {
		line_indent(fd);
		sprintf(buf, "  MCi_MISC register valid\n");
		fmd_log_write(fd, buf, strlen(buf));
	}
	
	if (status & MCI_STATUS_ADDRV) {
		line_indent(fd);
		sprintf(buf, "  MCi_ADDR register valid\n");
		fmd_log_write(fd, buf, strlen(buf));
	}
	
	if (status & MCI_STATUS_PCC) {
		line_indent(fd);
		sprintf(buf, "  Processor context corrupt\n");
		fmd_log_write(fd, buf, strlen(buf));
	}
	
	if (status & (MCI_STATUS_S|MCI_STATUS_AR)) {
		line_indent(fd);
		sprintf(buf, "%s\n", arstate[(status >> 55) & 3]);
		fmd_log_write(fd, buf, strlen(buf));
	}

	if ((mcgcap & MCG_SER_P) && (status & MCI_STATUS_FWST)) {
		line_indent(fd);
		sprintf(buf, "Firmware may have updated this error\n");
		fmd_log_write(fd, buf, strlen(buf));
	}
}

static char *vendor[] = {
	[0] = "Intel",
	[1] = "Cyrix",
	[2] = "AMD",
	[3] = "UMC", 
	[4] = "vendor 4",
	[5] = "Centaur",
	[6] = "vendor 6",
	[7] = "Transmeta",
	[8] = "NSC"
};

static char *
cpuvendor_name(uint32_t cpuvendor)
{
	return (cpuvendor < NELE(vendor)) ? vendor[cpuvendor] : "Unknown vendor";
}

inline void
line_indent(int fd)
{
#define INDENT_SPACE_NUM	19
	char buf[64];
	int i;
	
	for (i = 0; i < INDENT_SPACE_NUM; i++) {
		buf[i] = ' ';
	}
	buf[i++] = '\t';
	buf[i] = '\0';
	
	fmd_log_write(fd, buf, strlen(buf));
#undef INDENT_SPACE_NUM
}

void
cpumem_err_printf(int fd, struct fms_cpumem *fc)
{
	char buf[512];
	
	if(!fc)
		return ;

	line_indent(fd);
	sprintf(buf, "CPU %d %s\n", fc->cpu, bankname(fc->bank, fc->cputype));
	fmd_log_write(fd, buf, strlen(buf));

	if (fc->tsc)
		print_tsc(fd, buf, fc->cpu, fc->tsc, fc->time, fc->cputype);
	
	if (fc->ip) {
		line_indent(fd);
		sprintf(buf, "RIP%s %02x:%llx\n", 
			!(fc->mcgstatus & MCG_STATUS_EIPV) ? " !INEXACT!" : "",
			fc->cs, (long long unsigned int)fc->ip);
		fmd_log_write(fd, buf, strlen(buf));
	}

	if (fc->status & MCI_STATUS_MISCV) {
		line_indent(fd);
		sprintf(buf, "MISC: %llx\n", (long long unsigned int)fc->misc);
		fmd_log_write(fd, buf, strlen(buf));
	}
	if (fc->status & MCI_STATUS_ADDRV) {
		line_indent(fd);
		sprintf(buf, "ADDR: %llx\n", (long long unsigned int)fc->addr);
		fmd_log_write(fd, buf, strlen(buf));
	}

	if (fc->time) {
		time_t t = fc->time;
		line_indent(fd);
		sprintf(buf, "TIME: %s", ctime(&t));
		fmd_log_write(fd, buf, strlen(buf));
	}

	if (fc->bank == MCE_THERMAL_BANK) {
		line_indent(fd);
		fmd_log_write(fd, fc->desc, strlen(fc->desc));
		fmd_log_write(fd, "\n", 1);
		return ;
	}
	
	print_mcg(fd, buf, fc->mcgstatus);
	print_mci(fd, buf, fc->status, fc->mcgcap);

	line_indent(fd);
	fmd_log_write(fd, "MCA: ", 5);
	fmd_log_write(fd, fc->desc, strlen(fc->desc));
	fmd_log_write(fd, "\n", 1);
	
	/* decode all status bits here */
	line_indent(fd);
	sprintf(buf, "STATUS: %llx\n", 
		(long long unsigned int)fc->status);
	fmd_log_write(fd, buf, strlen(buf));

	line_indent(fd);
	sprintf(buf, "MCGSTATUS: %llx\n", 
		(long long unsigned int)fc->mcgstatus);
	fmd_log_write(fd, buf, strlen(buf));
	
	if (fc->mcgcap) {
		line_indent(fd);
		sprintf(buf, "MCGCAP: %llx\n", (long long unsigned int)fc->mcgcap);
		fmd_log_write(fd, buf, strlen(buf));
	}
	if (fc->apicid) {
		line_indent(fd);
		sprintf(buf, "APICID: %x\n", fc->apicid);
		fmd_log_write(fd, buf, strlen(buf));
	}
	if (fc->socketid) {
		line_indent(fd);
		sprintf(buf, "SOCKETID: %x\n", fc->socketid);
		fmd_log_write(fd, buf, strlen(buf));
	}
	if (fc->cpuid) {
		uint32_t fam, mod;
		parse_cpuid(fc->cpuid, &fam, &mod);
		line_indent(fd);
		sprintf(buf, "CPUID Vendor %s Family %u Model %u\n",
			cpuvendor_name(fc->cpuvendor), 
			fam,
			mod);
		fmd_log_write(fd, buf, strlen(buf));
	}

	/* dump memory device, if any */
	if (fc->memdev) {
		dump_memdev(fd, buf, fc->memdev, fc->addr);
	}
}

#ifdef TEST_CMEA

int
test_cpu_offine_on_cache(int cpu, unsigned clevel, unsigned ctype)
{
	return cpu_offine_on_cache(cpu, clevel, ctype);
}

int
test_memory_page_offline(uint64_t addr)
{
	return memory_page_offline(addr);
}

#endif /* TEST_CMEA */