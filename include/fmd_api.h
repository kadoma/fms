
#ifndef __FMD_API_H__
#define __FMD_API_H__ 1

#include "wrap.h"
#include "fmd_event.h"
#include "fmd.h"
#include "fmd_nvpair.h"


fmd_event_t *
fmd_create_ereport(fmd_t *p_fmd, const char *eclass, char *id, nvlist_t *data);

#endif // fmd_api.h
