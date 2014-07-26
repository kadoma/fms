/*
 * fmd_case.h
 *
 *  Created on: Feb 20, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	fmd_case.h
 */

#ifndef FMD_CASE_H_
#define FMD_CASE_H_

#include <stdint.h>
#include <time.h>
#include <fmd_list.h>

struct fmd_case_type {
	uint64_t ereport[16];
	int size;

	uint64_t fault;
	uint64_t serd;

	struct list_head list;
};


#define CASE_CREATE 	0x00
#define CASE_FIRED	0x01
#define CASE_CLOSED	0x02

typedef struct fmd_case {
	uint64_t cs_uuid;
	uint64_t cs_rscid;

	uint8_t cs_flag;	/* see above */
	struct fmd_case_type *cs_type;
	
	uint32_t cs_count;
	time_t cs_create;
	time_t cs_fire;
	time_t cs_close;
	
	/* event list */
	struct list_head cs_event;

	/* link to global case list */
	struct list_head cs_list;
} fmd_case_t;

#endif /* FMD_CASE_H_ */
