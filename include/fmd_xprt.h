/*
 * fmd_xprt.h
 *
 *  Created on: Feb 25, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	fmd_xprt.h
 */

#ifndef FMD_XPRT_H_
#define FMD_XPRT_H_

#include <fmd_event.h>
#include <fmd_module.h>

int
fmd_xprt_dispatch(fmd_module_t *, fmd_event_t *);

#endif /* FMD_XPRT_H_ */
