/*
 * cpumem_evtsrc.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description: 
 * 	Cpu Memory error evtsrc module
 *
 *	This evtsrc module is respon
 */

#include <sys/types.h>
#include <alloca.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#include <fmd.h>
#include <fmd_evtsrc.h>
#include <fmd_module.h>
#include <protocol.h>
#include <nvpair.h>
#include "mca_fm.h"

#define LOG_DROPPED 30*60           /* log dropped records every 30 minutes */
static unsigned alarm_left = LOG_DROPPED;
static volatile unsigned alarm_popped;
static volatile unsigned more_to_do = 1;

static fmd_t *fmdp;

void
cpumem_fm_event_post(nvlist_t *nvl, char *fullclass,
		int cpuid, uint64_t addr)
{
	if (cpuid != -1) {
		nvl = nvlist_alloc();
		sprintf(nvl->name, FM_CLASS);
		strcpy(nvl->value, fullclass);
		nvl->rscid = topo_get_cpuid(fmdp, cpuid);
		return;
	} else if (addr != -1) {
		nvl = nvlist_alloc();
		sprintf(nvl->name, FM_CLASS);
		strcpy(nvl->value, fullclass);
		nvl->rscid = topo_get_memid(fmdp, addr);
		return;
	} else {
		nvl = NULL;
	}
}

/*
 * Talk to /proc/sal/type/data to read and get SAL
 * records buffer.
 */
static sal_log_record_header_t *
salinfo_buffer(int fd, int *bufsize)
{
	int nbytes, size, alloc;
	sal_log_record_header_t *buffer;

	lseek(fd, 0, 0);
	buffer = NULL;
	alloc = 16 * 1024;	// total buffer size
	size = 0;		// amount of buffer used so far
	do {
		buffer = realloc(buffer, alloc);
		if (!buffer) {
			printf("Can't alloc %d bytes\n", alloc);
			exit(1);
		}

		nbytes = read(fd, buffer + size, alloc - size);
		if (nbytes < 0) {
			perror("salinfo_buffer read");
			exit(1);
		}

		if (nbytes == alloc - size)
			alloc *= 2;

		size += nbytes;
	} while (nbytes);

	if (size) {
		if (bufsize)
			*bufsize = size;
		return buffer;
	}

	free(buffer);
	return NULL;
}

static int
clear_cpu(int fd_data, int cpu, const char *data_filename, int have_data)
{
	char text[400];
	int l;
	static int prev_cpu = -1, loop = 0, do_clear = 0;

	if (have_data)
		loop = 0;
	if (!do_clear) {
		if (cpu <= prev_cpu) {
			++loop;
			if (loop == 2)
				do_clear = 1;
		}
		prev_cpu = cpu;
	}
	if (!have_data && !do_clear)
		return 0;

	snprintf(text, sizeof(text), "clear %d\n", cpu);
	l = strlen(text);
	if (write(fd_data, text, l) != l) {
		fprintf(stderr, "%s: Error writing '%s' to %s\n",
			__FUNCTION__, text, data_filename);
		perror(data_filename);
		return 1;
	}
	return 0;
}

/* Log the number of dropped records every LOG_DROPPED seconds.  The only time
 * that we want to test for the logging interval expiring is while we are
 * waiting for the kernel to provide a new SAL record, so disable the alarm
 * everywhere else.  This avoids having to check for the alarm except on the
 * read path.
 */
static void
disable_alarm(void)
{
	signal(SIGALRM, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	alarm_left = alarm(0);
}

static void
sig_handler(int sig)
{   
	alarm_popped = 1;
	if (sig != SIGALRM)
		more_to_do = 0;
	disable_alarm();
}   

static void
enable_alarm(void)
{
	signal(SIGHUP, sig_handler);
	signal(SIGALRM, sig_handler);
	alarm(alarm_left > 0 ? alarm_left : 1);
}

/* Talk to /proc/sal/type/{event,data} to extract, save, decode and clear SAL
 * records.
 */
static int
talk_to_sal(cpumem_monitor_t *cmp, char *type)
{
	sal_log_record_header_t *buffer;
	char event_filename[PATH_MAX], data_filename[PATH_MAX], text[200];
	int fd_event = -1, fd_data = -1, i, cpu, ret = 1;
	static const char *rd[] = { "raw", "decoded" };

	char *directory = strdup("/var/log/salinfo");

	for (i = 0; i < 2; ++i) {
		int fd;
		char filename[PATH_MAX];
		snprintf(filename, sizeof(filename), "%s/%s/.check", directory, rd[i]);
		if ((fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC)) < 0) {
			perror(filename);
                         goto out;
		}
		close(fd);
		unlink(filename);
	}

	snprintf(event_filename, sizeof(event_filename), "/proc/sal/%s/event", type);
	if ((fd_event = open(event_filename, O_RDONLY|O_NONBLOCK)) < 0) {
		perror(event_filename);
		goto out;
	}
	snprintf(data_filename, sizeof(data_filename), "/proc/sal/%s/data", type);
	if ((fd_data = open(data_filename, O_RDWR|O_NONBLOCK)) < 0) {
		perror(data_filename);
		goto out;
	}
	
	/* Run until we are killed */
	while (more_to_do) {
		int i, l, fd, bufsize, suffix, ret;
		char filename[PATH_MAX], basename[PATH_MAX];
		enable_alarm();
		ret = read(fd_event, text, sizeof(text));
		if (alarm_popped || ret <= 0) {
			if (alarm_popped) {
//				log_dropped_records();
				alarm_popped = 0;
				alarm_left = LOG_DROPPED;
				continue;
			}
			if (ret == 0 || errno == EAGAIN)
				goto out;
			if (errno == EINTR)
				continue;
			perror(event_filename);
			goto out;
		}
		disable_alarm();
		if (sscanf(text, "read %d\n", &cpu) != 1) {
			fprintf(stderr, "%s: Unknown text '%s' from %s\n",
				__FUNCTION__, text, event_filename);
			goto out;
		}
		l = strlen(text);
		if (write(fd_data, text, l) != l) {
			fprintf(stderr, "%s: Error writing '%s' to %s\n",
				__FUNCTION__, text, data_filename);
			perror(data_filename);
			goto out;
		}
		if (!(buffer = salinfo_buffer(fd_data, &bufsize))) {
			if (clear_cpu(fd_data, cpu, data_filename, 0))
				goto out;
			continue;       /* event but no data is normal at boot */
		}

		printf("BEGIN HARDWARE ERROR STATE from %s on cpu %d\n", type, cpu);
		platform_info_check(cmp, buffer);
		printf("END HARDWARE ERROR STATE from %s on cpu %d\n", type, cpu);

		if (clear_cpu(fd_data, cpu, data_filename, 1))
			goto out;
		free(buffer);
	}

out:
	if (fd_event > 0)
		close(fd_event);
	if (fd_data > 0)
		close(fd_data);
	return ret;
}

/*
 * Free cpumem_monitor_t struct
 */
static void
cpumem_close_salinfo(cpumem_monitor_t *cmp)
{
	nvlist_free(cmp->cache_err);
	nvlist_free(cmp->tlb_err);
	nvlist_free(cmp->bus_err);
	nvlist_free(cmp->reg_file_err);
	nvlist_free(cmp->ms_err);
	nvlist_free(cmp->mem_dev_err);
	nvlist_free(cmp->sel_dev_err);
	nvlist_free(cmp->pci_bus_err);
	nvlist_free(cmp->smbios_dev_err);
	nvlist_free(cmp->pci_comp_err);
	nvlist_free(cmp->plat_specific_err);
	nvlist_free(cmp->host_ctlr_err);
	nvlist_free(cmp->plat_bus_err);

	cmp->cache_err = cmp->tlb_err = cmp->bus_err = NULL;
	cmp->reg_file_err = cmp->ms_err = NULL;
	cmp->mem_dev_err = cmp->sel_dev_err = cmp->pci_bus_err = NULL;
	cmp->smbios_dev_err = cmp->pci_comp_err = cmp->plat_specific_err = NULL;
	cmp->host_ctlr_err = cmp->plat_bus_err = NULL;

	free(cmp);
	cmp = NULL;
}

/*
 * Decode SAL error records, and generates any ereports as necessary.
 */
static int
cpumem_analyze_salinfo(cpumem_monitor_t *cmp)
{
	int i;
	char *type[3] = {"cmc", "cpe", "mca"};

	struct list_head *faults =
		(struct list_head *)malloc(sizeof(struct list_head));

	if (faults == NULL) {
		printf("Cpumem Module: failed to malloc\n");
		return (-1);
	}

	cmp->faults = faults;
	cmp->cache_err = cmp->tlb_err = cmp->bus_err = NULL;
	cmp->reg_file_err = cmp->ms_err = NULL;
	cmp->mem_dev_err = cmp->sel_dev_err = cmp->pci_bus_err = NULL;
	cmp->smbios_dev_err = cmp->pci_comp_err = cmp->plat_specific_err = NULL;
	cmp->host_ctlr_err = cmp->plat_bus_err = NULL;
	INIT_LIST_HEAD(cmp->faults);

	for (i = 0; i < 3; ++i) {
		talk_to_sal(cmp, type[i]);
	}

	if (cmp->cache_err != NULL) {
		nvlist_add_nvlist(cmp->faults, cmp->cache_err);
	}

	if (cmp->tlb_err != NULL) {
		nvlist_add_nvlist(cmp->faults, cmp->tlb_err);
	}

	if (cmp->bus_err != NULL) {
		nvlist_add_nvlist(cmp->faults, cmp->bus_err);
	}

	if (cmp->reg_file_err != NULL) {
		nvlist_add_nvlist(cmp->faults, cmp->reg_file_err);
	}

	if (cmp->ms_err != NULL) {
		nvlist_add_nvlist(cmp->faults, cmp->ms_err);
	}

	if (cmp->mem_dev_err != NULL) {
		nvlist_add_nvlist(cmp->faults, cmp->mem_dev_err);
	}

	if (cmp->sel_dev_err != NULL) {
		nvlist_add_nvlist(cmp->faults, cmp->sel_dev_err);
	}

	if (cmp->pci_bus_err != NULL) {
		nvlist_add_nvlist(cmp->faults, cmp->pci_bus_err);
	}

	if (cmp->smbios_dev_err != NULL) {
		nvlist_add_nvlist(cmp->faults, cmp->smbios_dev_err);
	}

	if (cmp->pci_comp_err != NULL) {
		nvlist_add_nvlist(cmp->faults, cmp->pci_comp_err);
	}

	if (cmp->plat_specific_err != NULL) {
		nvlist_add_nvlist(cmp->faults, cmp->plat_specific_err);
	}

	if (cmp->host_ctlr_err != NULL) {
		nvlist_add_nvlist(cmp->faults, cmp->host_ctlr_err);
	}

	if (cmp->plat_bus_err != NULL) {
		nvlist_add_nvlist(cmp->faults, cmp->plat_bus_err);
	}

	return 0;
}

/*
 * Periodic timeout.
 * Calling cpumem_analyze_salinfo().
 */
struct list_head *
cpumem_probe(evtsrc_module_t *emp)
{
	struct list_head *faults = NULL;
	fmdp = emp->module.mod_fmd;

	cpumem_monitor_t *cmp =
		(cpumem_monitor_t *)malloc(sizeof(cpumem_monitor_t));

	if (cmp == NULL) {
		printf("Cpumem Module: failed to malloc\n");
		return NULL;
	}
	memset(cmp, 0, sizeof(cpumem_monitor_t));

	if (cpumem_analyze_salinfo(cmp) != 0) {
		printf("Cpumem Module: failed to check Cpumem\n");
		return NULL;
	}

	faults = cmp->faults;
	cpumem_close_salinfo(cmp);

	return faults;
}

static evtsrc_modops_t cpumem_mops = {
	.evt_probe = cpumem_probe,
};

fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	return (fmd_module_t *)evtsrc_init(&cpumem_mops, path, pfmd);
}

void
fmd_fini(fmd_module_t *mp)
{
}

