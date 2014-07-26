/*
 * conf.h
 *
 *  Created on: Feb 25, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	conf.h
 */

#ifndef CONF_H_
#define CONF_H_

#include <time.h>
#include <fmd_list.h>

typedef struct fmd_conf {
	time_t cf_interval;	/* timer intervals */
	float cf_ver;		/* a copy of module version string */
	struct list_head *list_eclass;
} fmd_conf_t;

#endif /* CONF_H_ */
