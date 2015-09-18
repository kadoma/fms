/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	tsc.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-04
 * Description:	
 *				parse tsc.
 *
 */

#define _GNU_SOURCE 1
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "tsc.h"
#include "cpumem.h"

static unsigned 
scale(uint64_t *tsc, unsigned unit, double mhz)
{
	uint64_t v = (uint64_t)(mhz * 1000000) * unit;
	unsigned u = *tsc / v;
	*tsc -= u * v;
	
	return u;
}

static int 
fmt_tsc(char **buf, uint64_t tsc, double mhz)
{
	unsigned days, hours, mins, secs;
	if (mhz == 0.0)
		return -1;
	days = scale(&tsc, 3600 * 24, mhz);
	hours = scale(&tsc, 3600, mhz);
	mins = scale(&tsc, 60, mhz);
	secs = scale(&tsc, 1, mhz);
	asprintf(buf, "[at %.0f Mhz %u days %u:%u:%u uptime (unreliable)]", 
		mhz, days, hours, mins, secs);
	
	return 0;
}

static double 
cpufreq_mhz(int cpu, double infomhz)
{
	double mhz;
	FILE *f;
	char *fn;
	asprintf(&fn, "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", cpu);
	f = fopen(fn, "r");
	free(fn);
	if (!f) {
		/* /sys exists, but no cpufreq -- use value from cpuinfo */
		if (access("/sys/devices", F_OK) == 0)
			return infomhz;
		/* /sys not mounted. We don't know if cpufreq is active
		   or not, so must fallback */
		return 0.0;
	}
	if (fscanf(f, "%lf", &mhz) != 1)
		mhz = 0.0;
	mhz /= 1000;
	fclose(f);
	
	return mhz;
}

static int 
deep_sleep_states(int cpu)
{
	int ret;
	char *fn;
	FILE *f;
	char *line = NULL;
	size_t linelen = 0;

	/* When cpuidle is active assume there are deep sleep states */
	asprintf(&fn, "/sys/devices/system/cpu/cpu%d/cpuidle", cpu);
	ret = access(fn, X_OK);
	free(fn);
	if (ret == 0)
		return 1;

	asprintf(&fn, "/proc/acpi/processor/CPU%d/power", cpu);
	f = fopen(fn, "r");
	free(fn);
	if (!f)
		return 0;

	while ((getline(&line, &linelen, f)) > 0) {
		int n;
		if ((sscanf(line, " C%d:", &n)) == 1) {
			if (n > 1) {
				char *p = strstr(line, "usage");
				if (p && sscanf(p, "usage[%d]", &n) == 1 && n > 0)
					return 1;					
			}
		}
	}
	free(line);
	fclose(f);
	return 0;
}


/* Try to figure out if this CPU has a somewhat reliable TSC clock */
static int 
tsc_reliable(int cputype, int cpunum)
{
	if (!processor_flags)
		return 0;
	
	/* Trust the kernel */
	if (strstr(processor_flags, "nonstop_tsc"))
		return 1;
	
	/* TSC does not change frequency TBD: really old kernels don't set that */
	if (!strstr(processor_flags, "constant_tsc"))
		return 0;	
	/* We don't know the frequency on non Intel CPUs because the
	   kernel doesn't report them (e.g. AMD GH TSC doesn't run at highest
	   P-state). But then the kernel can just report the real time too. 
	   Also a lot of AMD and VIA CPUs have unreliable TSC, so would
	   need special rules here too. */
	if (!is_intel_cpu(cputype))
		return 0;
	if (deep_sleep_states(cpunum) && cputype != CPU_NEHALEM)
		return 0;
	
	return 1;
}

int 
decode_tsc_current(char **buf, int cpunum, enum cputype cputype, double mhz, 
		       unsigned long long tsc)
{
	double cmhz;
	
	if (!tsc_reliable(cputype, cpunum))
		return -1;
	
	cmhz = cpufreq_mhz(cpunum, mhz);
	if (cmhz != 0.0)
		mhz = cmhz;
	
	return fmt_tsc(buf, tsc, mhz);
}

