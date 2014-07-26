/*
 * fmd_event.h
 *
 *  Created on: Jan 16, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	Contains all event-types
 */

#ifndef FMD_EVENT_H_
#define FMD_EVENT_H_

#include <stdint.h>
#include <sys/time.h>
#include <nvpair.h>
#include <fmd_list.h>

/**
 * ereport.* --> diagnosis module
 * fault.*   --> agent module
 * list.*    --> diagnosis module
 */
#define DESTO_INVAL	-1
#define DESTO_SELF	0
#define DESTO_AGENT	1


/* fmd event type */
struct fmd_event_type {
	union {
		struct {
			uint64_t class0:10;
			uint64_t class1:10;
			uint64_t class2:10;
			uint64_t class3:10;
			uint64_t class4:10;
			uint64_t class5:10;
			uint64_t levels:4;	/* starts from 1 */
		} _st_evtype;
		uint64_t fmd_evtid;
	} _un_evtype;

	struct list_head list;
};

#define fmd_evtid _un_evtype.fmd_evtid
#define fmd_evt_class0 _un_evtype._st_evtype.class0
#define fmd_evt_class1 _un_evtype._st_evtype.class1
#define fmd_evt_class2 _un_evtype._st_evtype.class2
#define fmd_evt_class3 _un_evtype._st_evtype.class3
#define fmd_evt_class4 _un_evtype._st_evtype.class4
#define fmd_evt_class5 _un_evtype._st_evtype.class5
#define fmd_evt_levels _un_evtype._st_evtype.levels


/* actions */
#define LIST_ISOLATED	0x01
#define LIST_REPAIRED	0x02
#define LIST_RECOVER	0x03
#define LIST_LOG	0x04

#define LIST_ISOLATED_SUCCESS	0x0100
#define LIST_ISOLATED_FAILED	0x0101
#define LIST_REPAIRED_SUCCESS	0x0200
#define LIST_REPAIRED_FAILED	0x0201
#define LIST_RECOVER_SUCCESS	0x0300
#define LIST_RECOVER_FAILED	0x0301
#define LIST_LOG_SUCCESS	0x0400
#define LIST_LOG_FAILED		0x0401


/* fmd event */
typedef struct fmd_event {
	time_t 	ev_create; /* created time */

	/**
	 * reference count, if refs==0, then free the allocated memory
	 * refs == number of case(s) which hold(s) this event.
	 */
	int 		ev_refs;
	char *		ev_class;

	/**
	 * event id
	 * event (list.*)'s ev_eid = 0
	 */
	uint64_t	ev_eid;
	uint64_t	ev_uuid;	/* event (fault.* & list.*)'s case id */
	uint64_t	ev_rscid;	/* resource id */
	nvlist_t *	ev_data;	/* data */

	struct list_head ev_list;
} fmd_event_t;


/**
 * Get the sending direction for evt
 *
 * @param
 * @return
 */
static inline int
event_disp_dest(fmd_event_t *evt)
{
	if(strncmp(evt->ev_class, "ereport.", 8) == 0 ||
			strncmp(evt->ev_class, "list.", 5) == 0) {
		return DESTO_SELF;
	} else if(strncmp(evt->ev_class, "fault.", 6) == 0) {
		return DESTO_AGENT;
	} else {
		return DESTO_INVAL;
	}
}

#endif /* FMD_EVENT_H_ */
