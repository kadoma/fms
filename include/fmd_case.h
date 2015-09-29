#ifndef __FMD_CASE_H__
#define __FMD_CASE_H__ 1


#include <time.h>
#include <stdint.h>
#include "wrap.h"
#include "fmd_esc.h"
#include "list.h"

#define CASE_CREATE 0x00
#define CASE_FIRED 0x01
#define CASE_CLOSED 0x02

struct fmd_case_type{
    struct list_head    case_list;
    int                 size;
    uint64_t            fault;
    uint64_t            serd;
    uint64_t            ereport[16];
};

typedef struct _fmd_case_{
	uint64_t   cs_uuid;
	uint64_t   cs_rscid;

	char      *dev_name;
	uint64_t   err_id;
	char       last_eclass[64];

	uint8_t    cs_flag;	/* see above */
	struct     fmd_case_type *cs_type;
	
	uint32_t   cs_count;

	uint32_t   cs_fire_times;      /* init zero must be*/
	
	time_t     cs_create;
	time_t     cs_first_fire;
	time_t     cs_last_fire;
	time_t     cs_close;
	
	// event list link all event include same. 
	struct list_head cs_event;

	/* link to global fmd case list */
	struct list_head cs_list;

}fmd_case_t;


#endif // fmd_case.h
