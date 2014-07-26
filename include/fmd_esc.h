/*
 * esc.h
 *
 *  Created on: Feb 10, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	esc
 */

#ifndef ESC_H_
#define ESC_H_

#include <fmd_hash.h>
#include <fmd_list.h>

/**
 * node defs for parse
 *
 */
struct esc_node {
	char *fullclass;
	char *nvlist;
};

struct rend_node {
	char *evtlist;
	char *serd;
	char *fault;
};

struct prop_node {
	char *ereport;
	char *evtlist;
};


/* global hashtable */
typedef struct fmd_esc {
	struct fmd_hash hash_clsname;
	struct list_head list_evtype;
	struct list_head list_serdtype;
	struct list_head list_casetype;
} fmd_esc_t;

#endif /* ESC_H_ */
