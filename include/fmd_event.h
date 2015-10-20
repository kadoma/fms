
#ifndef __FMD_EVENT_H__
#define __FMD_EVENT_H__ 1

#include <stdint.h>
#include <sys/time.h>

#include "wrap.h"
#include "list.h"


/* actions */
#define LIST_ISOLATED	0x01
#define LIST_REPAIRED	0x02
#define LIST_RECOVER	0x03
#define LIST_LOG	0x04


/**
 * ereport.* --> diagnosis module
 * fault.*   --> agent module
 * list.*    --> diagnosis module
 */
#define DESTO_INVAL	-1
#define DESTO_SELF	0
#define DESTO_AGENT	1
#define DESTO_AGENT_AND_CASE 2

// event list . report, fault, info
#define FM_EREPORT_CLASS    "ereport"
#define FM_FAULT_CLASS      "fault"
#define FM_INFO_CLASS       "info"

// event action and result
// isolated, repaired, logged
#define LIST_ISOLATED_SUCCESS  0x0100
#define LIST_ISOLATED_FAILED  0x0101

#define LIST_REPAIRED_SUCCESS  0x0102
#define LIST_REPAIRED_FAILED  0x0103

#define LIST_LOGED_SUCCESS  0x0104
#define LIST_LOGED_FAILED  0x0105

#define AGENT_TODO         0x0106  //action  fault file
#define AGENT_TOLOG        0x0107   // to log serd file
#define AGENT_TOLIST       0X0108   // to log list file     

typedef struct _fmd_event_{
	uint64_t 			ev_flag;       // agent todo some , to log.
	struct list_head    ev_list;
	time_t              ev_create;    // the time for event occur
	int                 ev_refs;      // occur counts
	char                *ev_desc;     // description for event
    
	char                *dev_name;
	uint64_t            ev_err_id;    // cpu num or mem num
	char 				*ev_class;
	uint64_t             agent_result;
	char                 *data;  //private data
}fmd_event_t;

#include "fmd_case.h"
#include "fmd.h"

extern fmd_event_t * fmd_create_listevent(fmd_event_t *fault, int action);
extern fmd_event_t * fmd_create_casefault(fmd_t *p_fmd, fmd_case_t *pcase);
extern void fmd_create_fault(fmd_event_t *pevt);

#endif // fmd_event.h
