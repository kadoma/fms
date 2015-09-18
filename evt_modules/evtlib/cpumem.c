/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	cpumem.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-04
 * Description:	
 *				cpumem common base function.
 *
 */

#include "cpumem.h"

struct cpuid1 {
	unsigned stepping : 4;
	unsigned model : 4;
	unsigned family : 4;
	unsigned type : 2;
	unsigned res1 : 2;
	unsigned ext_model : 4;
	unsigned ext_family : 8; 
	unsigned res2 : 4;
};

void 
parse_cpuid(uint32_t cpuid, uint32_t *family, uint32_t *model)
{
	union { 
		struct cpuid1 c;
		uint32_t v;
	} c;

	/* Algorithm from IA32 SDM 2a 3-191 */
	c.v = cpuid;
	*family = c.c.family; 
	if (*family == 0xf) 
		*family += c.c.ext_family;
	*model = c.c.model;
	
	if (*family == 6 || *family == 0xf) 
		*model += c.c.ext_model << 4;
}

/* Relies on sanity checks in check_entry */
char *
dmi_getstring(struct dmi_entry *e, unsigned number)
{
	char *s = (char *)e + e->length;
	if (number == 0) 
		return "";
	do { 
		if (--number == 0) 
			return s;
		while (*s)
			s++;
		s++;
	} while (*s);
	return NULL;
}

int 
is_intel_cpu(int cpu)
{
	switch (cpu) {
	CASE_INTEL_CPUS:
		return 1;
	} 
	return 0;
}
