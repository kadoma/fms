
#ifndef __FMD_EVENT_H__
#define __FMD_EVENT_H__ 1

#include <stdint.h>
#include <sys/time.h>

#include "wrap.h"
#include "list.h"


/* actions */
#define LIST_ISOLATED   0x01
#define LIST_REPAIRED   0x02
#define LIST_RECOVER    0x03
#define LIST_LOG    0x04


/**
 * ereport.* --> diagnosis module
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
#define LIST_RESULT_MASK       0x0001

#define LIST_ISOLATED_FAILED   0x0101
#define LIST_ISOLATED_SUCCESS  0x0102

#define LIST_REPAIRED_FAILED  0x0103
#define LIST_REPAIRED_SUCCESS  0x0104


#define AGENT_TODO         0x0301  //action  fault file
#define AGENT_TOLOG        0x0302   // to log serd file
#define AGENT_TOLIST       0x0303   // to log list file 

#define EVENT_REPORT       0x0200
#define EVENT_SERD         0x0201
#define EVENT_FAULT        0x0202
#define EVENT_LIST         0x0203

typedef struct _fmd_event_{
    char                *dev_name;  //strdup
    int64_t             dev_id;
    int64_t             evt_id;
    char               *ev_class;  //strdup
    int                 event_type;
    int                 N;
    time_t              T;
	int                 repaired_N;
	time_t              repaired_T;
    time_t              ev_create;    // the time for event occur
    int                 ev_count;     //this event counts
    int                 ev_refs;      // occur counts
    int64_t             ev_flag;       // agent todo some , to log.
    int64_t             agent_result;   //agent to do result

    struct list_head    ev_list;
	struct list_head    ev_repaired_list;
    void               *p_case;
    char               *data;  //private data
}fmd_event_t;

#include "fmd_case.h"
#include "fmd.h"

extern fmd_event_t * fmd_create_listevent(fmd_event_t *fault, int action);
extern void fmd_create_fault(fmd_event_t *pevt);

#endif // fmd_event.h
