
#ifndef _FMD_EVTSRC_H
#define _FMD_EVTSRC_H 1

#include "wrap.h"
#include "fmd_nvpair.h"
#include "fmd_module.h"
#include "fmd_event.h"
#include "fmd.h"
#include "list.h"

struct _evtsrc_module_;

typedef struct _evtsrc_modops_{
    fmd_modops_t       modops;     // init finit
    struct list_head   *(*evt_probe)(struct _evtsrc_module_ *);
}evtsrc_modops_t;

typedef struct _evtsrc_module_{
    fmd_module_t module;    /* base module properties */
    fmd_timer_t timer;
    evtsrc_modops_t *mops;    // probe
}evtsrc_module_t;

fmd_module_t *evtsrc_init(evtsrc_modops_t *, char *, fmd_t *);

#endif // evtsrc.h
