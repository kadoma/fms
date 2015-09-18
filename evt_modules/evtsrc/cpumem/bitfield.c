/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	bitfield.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-04
 * Description:	
 *				Parse bit field. 
 *
 */

#define _GNU_SOURCE 1
#include <string.h>
#include <stdio.h>

#include "cpumem_evtsrc.h"
#include "bitfield.h"


char *reserved_3bits[8];
char *reserved_1bit[2];
char *reserved_2bits[4];

static u64 
bitmask(u64 i)
{
	u64 mask = 1;
	
	while (mask < i) 
		mask = (mask << 1) | 1; 
	
	return mask;
}

void 
decode_bitfield(u64 status, struct field *fields, char *buf, 
		struct mc_msg *mm, int *mm_num, char *type, int is_mnemonic)
{
	struct field *f;
	char tmp[60];
	int mn = *mm_num;
	
	for (f = fields; f->str; f++) { 
		u64 v = (status >> f->start_bit) & bitmask(f->stringlen - 1);
		char *s = NULL;
		
		if (v < f->stringlen)
			s = f->str[v]; 
		if (!s) { 
			if (v == 0) 
				continue;
			s = tmp; 
			tmp[(sizeof tmp)-1] = 0;
			if(!is_mnemonic)
				snprintf(tmp, (sizeof tmp) - 1, "<%u:%llx>", f->start_bit, v);
			else
				snprintf(tmp, (sizeof tmp) - 1, "unknown");
		}
		
		if (mn >= MCE_MSG_MAXNUM) {
			wr_log(CMES_LOG_DOMAIN, WR_LOG_ERROR, 
				"MCE msg num[%d] is too small", MCE_MSG_MAXNUM);
			goto out;
		}
		
		if (!is_mnemonic) {
			asprintf(&mm[mn].desc, "%s%s", buf ? buf : "", s);
		} else {
			asprintf(&mm[mn].mnemonic, "%s%s", type, FMS_MCI_STATUS_UC(status));
			strcpy(mm[mn].name, type);
		}
		
		mn++;
	}

out:
	if(!is_mnemonic)
		*mm_num = mn;
}
