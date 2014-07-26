/*
 * fmdump.h
 *
 *  Created on: Nov 1, 2010
 *      Author: Inspur OS Team
 *
 *  Description:
 *      fmdump.h
 *
 */

#ifndef	_FMDUMP_H
#define	_FMDUMP_H

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

#include <fmd_msg.h>

/* fmdump [-a|e|V][-c eclass][-t time][-T time][-u uuid][file] */
typedef enum {
	FMDUMP_AXXCTXX,	FMDUMP_AXXCXTX,	FMDUMP_AXXCXXX,
	FMDUMP_AXXXTXU,	FMDUMP_AXXXXTU,	FMDUMP_AXXXXXU,
	FMDUMP_AXXXTXX,	FMDUMP_AXXXXTX,	FMDUMP_AXXXXXX,
	FMDUMP_XEXCTXX,	FMDUMP_XEXCXTX,	FMDUMP_XEXCXXX,
	FMDUMP_XEXXTXU,	FMDUMP_XEXXXTU,	FMDUMP_XEXXXXU,
	FMDUMP_XEXXTXX,	FMDUMP_XEXXXTX,	FMDUMP_XEXXXXX,
	FMDUMP_XXVCTXX,	FMDUMP_XXVCXTX,	FMDUMP_XXVCXXX,
	FMDUMP_XXVXTXU,	FMDUMP_XXVXXTU,	FMDUMP_XXVXXXU,
	FMDUMP_XXVXTXX,	FMDUMP_XXVXXTX,	FMDUMP_XXVXXXX
} fmd_dump_type_t;

typedef struct fmd_log_record {
	char time[50];
	char eclass[50];
	uint64_t rscid;
	uint64_t eid;
	uint64_t uuid;
	struct fmd_log_record *next;
} fmd_log_record_t;

typedef struct fmd_log {
	struct fmd_log_record *fst_rec;
	struct fmd_log *next;
} fmd_log_t;

#endif	/* _FMDUMP_H */

