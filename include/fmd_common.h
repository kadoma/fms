#ifndef __FMD_COMMON_H__
#define __FMD_COMMON_H__ 1

#include "wrap.h"
#include "logging.h"
#include "fmd.h"
#include "fmd_queue.h"
#include "fmd_ring.h"
#include "fmd_module.h"
#include "list.h"

#define FMS_PATH_MAX 128

#define min(x,y) ({ typeof(x) _x = (x); typeof(y) _y = (y); (void)(&_x == &_y);_x<_y ? _x : _y; })

#define FMD_START 1
#define FMD_STOP 2

void usage();

int analy_start_para(int argc, char **argv, int *debug, int *run_mode);

int file_lock(char *name);

int wait_lock_free(char *name);

pid_t mylock(int fd);

void rv_b2daemon();

void  fmd_exitproc();

void fmd_timer_fire(fmd_timer_t *ptimer);

void fmd_load_esc();

#endif  /* __FMD_COMMON_H__ */
