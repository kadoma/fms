/*
 * fmd_log.h
 *
 *  Created on: Feb 25, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	fmd_log.h
 */

#ifndef FMD_LOG_H_
#define FMD_LOG_H_

#include <fmd_event.h>

#define FMD_LOG_ERROR	0
#define FMD_LOG_FAULT	1
#define FMD_LOG_LIST	2

extern void fmd_log_event(fmd_event_t *);

extern void fmd_get_time(char *, time_t);

#endif /* FMD_LOG_H_ */
