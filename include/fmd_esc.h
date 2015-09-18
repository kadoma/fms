#ifndef __FMD_ESC_H__
#define __FMD_ESC_H__ 1

#include "fmd_hash.h"
#include "list.h"

#define CLASS_PATH 64

typedef struct fmd_event_type{
	char   eclass[CLASS_PATH];
	struct list_head list;
}fmd_event_type_t;

typedef struct fmd_fault_type{
	char   eclass[CLASS_PATH];
	struct list_head list;
}fmd_fault_type_t;

typedef struct fmd_serd_type{
	char   eclass[CLASS_PATH];
	uint32_t N;
	uint32_t T;
	struct list_head list;
}fmd_serd_type_t;

typedef struct fmd_esc{
	struct fmd_hash hash_clsname;

	struct list_head list_event;
	struct list_head list_serd;
	struct list_head list_fault;
}fmd_esc_t;

extern struct fmd_serd_type *fmd_query_serd(char  *eclass);

#endif // fmd_esc.h
