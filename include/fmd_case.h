#ifndef __FMD_CASE_H__
#define __FMD_CASE_H__ 1


#include <time.h>
#include <stdint.h>
#include <fcntl.h>

#include "tchdb.h"
#include "tcutil.h"
#include "wrap.h"
#include "fmd_esc.h"
#include "list.h"
#include "fmd.h"
#include "fmd_event.h"

#define CASE_CREATE 0x00
#define CASE_FIRED 0x01
#define CASE_CLOSED 0x02

#define CASE_DB  "/usr/lib/fms/db/rep_case.db"

typedef struct _fmd_case_{
	char      *dev_name;
	uint64_t   dev_id;
	char       last_eclass[128];

	uint8_t    cs_flag;   /* look above*/
	uint32_t   cs_total_count;  /*on this case . total events*/
	uint32_t   cs_fire_counts;  /*agent to process times */
	time_t     cs_create;       /*create time*/
	time_t     cs_first_fire;   /*first to process time*/
	time_t     cs_last_fire;    /*last time*/
	time_t     cs_close;        /*close time*/

	/* link to global fmd case list */
	struct list_head cs_list;
	struct list_head cs_event;
}fmd_case_t;

int fmd_case_insert(fmd_event_t *pevt);
fmd_event_t * fmd_create_casefault(fmd_t *p_fmd, fmd_case_t *pcase);
int fmd_case_find_and_delete(fmd_event_t *pevt);

#endif // fmd_case.h
