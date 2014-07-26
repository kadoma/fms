/*
 * fmtmsg.h
 *
 *  Created on: Nov 1, 2010
 *      Author: Inspur OS Team
 *
 *  Description:
 *      fmtmsg.h
 *
 */

#ifndef	_FMTMSG_H
#define	_FMTMSG_H

#include <fmd_msg.h>

/* fmtmsg [-i code][-c class][-n class] */
typedef enum {
	FMTMSG_ECODE,
	FMTMSG_CCLASS,
	FMTMSG_NCLASS
} fmtmsg_type_t;

#endif	/* _FMTMSG_H */

