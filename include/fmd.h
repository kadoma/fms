
#ifndef __FMD_H__
#define __FMD_H__ 1

#include "wrap.h"
#include "list.h"
#include "logging.h"
#include "fmd_esc.h"
#include "fmd_queue.h"
#include "fmd_topo.h"

#define BASE_DIR "/usr/lib/fms"
#define ESC_DIR "/usr/lib/fms/escdir"
#define PLUGIN_DIR "/usr/lib/fms/plugins"

/* thread's module */
pthread_key_t key_module;
pthread_key_t key_fmd;

typedef struct _fmd_timer_{
    struct list_head timer_list;                // every module have a timer struct
    uint32_t         interval;
    pthread_mutex_t  timer_lock;
    pthread_cond_t   cond_signal;
    pthread_t        tid;
    uint32_t         ticks;     //times
}fmd_timer_t;

typedef struct _fmd_t_{
    struct list_head     fmd_timer;
    struct list_head     fmd_module;// cpu module. mem module.....
    
    struct list_head     list_case;   //  serd case
    struct list_head     list_repaired_case; // agent case
    
    fmd_queue_t          fmd_queue;  // event queue
    fmd_esc_t            fmd_esc;
    fmd_topo_t           fmd_topo;
}fmd_t;

extern fmd_t fmd;

#endif // fmd.h
