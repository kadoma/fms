/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	intel.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-04
 * Description:	
 *				parse intel cpu. 
 *
 */

#include <stddef.h>
#include "cpumem_evtsrc.h"
#include "intel.h"
#include "haswell.h"


enum cputype 
select_intel_cputype(int family, int model)
{
	if (family == 15) { 
		if (model == 6) 
			return CPU_TULSA;
		return CPU_P4;
	} 
	
	if (family == 6) { 
		if (model < 0xf) 
			return CPU_P6OLD;
		else if (model == 0xf || model == 0x17) /* Merom/Penryn */
			return CPU_CORE2;
		else if (model == 0x1d)
			return CPU_DUNNINGTON;
		else if (model == 0x1a || model == 0x2c || model == 0x1e ||
			 model == 0x25)
			return CPU_NEHALEM;
		else if (model == 0x2e || model == 0x2f)
			return CPU_XEON75XX;
		else if (model == 0x2a)
			return CPU_SANDY_BRIDGE;
		else if (model == 0x2d)
			return CPU_SANDY_BRIDGE_EP;
		else if (model == 0x3a)
			return CPU_IVY_BRIDGE;
		else if (model == 0x3e)
			return CPU_IVY_BRIDGE_EPEX;
		else if (model == 0x3c || model == 0x45 || model == 0x46)
			return CPU_HASWELL;
		else if (model == 0x3f)
			return CPU_HASWELL_EPEX;
		else if (model == 0x3d || model == 0x4f || model == 0x56)
			return CPU_BROADWELL;
		else if (model == 0x57)
			return CPU_KNIGHTS_LANDING;
		else if (model == 0x1c || model == 0x26 || model == 0x27 ||
			 model == 0x35 || model == 0x36 || model == 0x36 ||
			 model == 0x37 || model == 0x4a || model == 0x4c ||
			 model == 0x4d || model == 0x5a || model == 0x5d)
			return CPU_ATOM;
		if (model > 0x1a) {
			wr_log(CMES_LOG_DOMAIN, WR_LOG_WARNING, "Family 6 Model %x CPU: only decoding architectural errors",
				model);
			return CPU_INTEL; 
		}
	}
	if (family > 6) { 
		wr_log(CMES_LOG_DOMAIN, WR_LOG_WARNING, "Family %u Model %x CPU: only decoding architectural errors\n",
				family, model);
		return CPU_INTEL;
	}
	
	wr_log(CMES_LOG_DOMAIN, WR_LOG_WARNING, "Unknown Intel CPU type family %x model %x\n", 
		family, model);
	return family == 6 ? CPU_P6OLD : CPU_GENERIC;
}
