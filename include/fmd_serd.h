/*
 * fmd_serd.h
 *
 *  Created on: Feb 10, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	fmd_serd.h
 */

#ifndef FMD_SERD_H_
#define FMD_SERD_H_

#include <stdint.h>
#include <fmd_list.h>

/* fmd event type */
struct fmd_serd_type {
	union {
		struct {
			uint64_t class0:10;
			uint64_t class1:10;
			uint64_t class2:10;
			uint64_t class3:10;
			uint64_t class4:10;
			uint64_t class5:10;
			uint64_t levels:4;	/* starts from 1 */
		} _st_serdtype;
		uint64_t fmd_serdid;
	} _un_serdtype;

	uint32_t N;
	uint32_t T;

	struct list_head list;
};

#define fmd_serdid _un_serdtype.fmd_serdid
#define fmd_serd_class0 _un_serdtype._st_serdtype.class0
#define fmd_serd_class1 _un_serdtype._st_serdtype.class1
#define fmd_serd_class2 _un_serdtype._st_serdtype.class2
#define fmd_serd_class3 _un_serdtype._st_serdtype.class3
#define fmd_serd_class4 _un_serdtype._st_serdtype.class4
#define fmd_serd_class5 _un_serdtype._st_serdtype.class5
#define fmd_serd_levels _un_serdtype._st_serdtype.levels

extern struct fmd_serd_type *fmd_query_serd(uint64_t sid);

#endif /* FMD_SERD_H_ */
