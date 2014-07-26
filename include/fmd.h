/*
 * fmd.h
 *
 *  Created on: Jan 14, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	main header
 */

#ifndef _FMD_H
#define _FMD_H

#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <syslog.h>
#include <fmd_list.h>
#include <fmd_dispq.h>
#include <fmd_topo.h>
#include <fmd_esc.h>

/* Change this to whatever your daemon is called */
#define DAEMON_NAME "fmd"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

/* conf file */
#define EVTSRC_CONF	0
#define AGENT_CONF	1

#define fmd_debug \
	syslog(LOG_DEBUG, "%s %s %d\n", \
			__FILE__, __func__, __LINE__);


/* thread's module */
pthread_key_t t_module;
pthread_key_t t_fmd;

#define ERR_PTR(n) (void *)((uint64_t)(n))

#define BASE_DIR 	"/usr/lib/fm"
#define FMD_DIR 	"fmd"
#define PLUGIN_DIR 	"plugins"

typedef struct fmd_timer {
	struct list_head 	list;	/* fmd_t keeps this list's head. */
	time_t		 		interval;	/* timer intervals */
	uint32_t			ticks;
	pthread_mutex_t 	mutex;
	pthread_cond_t 		cv;
	pthread_t 			tid;
} fmd_timer_t;


typedef struct fmd {
	/* timer object used by fmd_module_evtsrc_t */
	struct list_head fmd_timer;

	/* module list */
	struct list_head fmd_module;

	/* dispatch queue */
	fmd_dispq_t fmd_dispq;

	/* topo */
	fmd_topo_t fmd_topo;

	/* eversholt */
	fmd_esc_t fmd_esc;

	/* case list */
	struct list_head list_case;
} fmd_t;


/**************************************************************
 * 	Functions
 *************************************************************/
int _fmd_topo(fmd_t *);	/* topo entry point */
int topo_tree_create(fmd_t *); /* create topo tree */
void _stat_topo(void);	/* topo statistics info */
int _update_topo(fmd_t *); /* update topology */

/* print topo info to stdio, not syslog */
void _print_cpu_topo(fmd_topo_t *);
void _print_pci_topo(fmd_topo_t *);
void print_topo_tree(fmd_topo_t *);

int esc_main(fmd_t *);	/* eversholt entry point */
void _stat_esc(void);	/* eversholt statistics info */

/* convert rscid */
uint64_t topo_get_cpuid(fmd_t *, int);
uint64_t topo_get_pcid(fmd_t *, char *);
uint64_t topo_get_memid(fmd_t *, uint64_t);
uint64_t topo_get_storageid(fmd_t *, char *);

int topo_cpu_by_cpuid(fmd_t *, uint64_t);
uint64_t topo_start_by_memid(fmd_t *, uint64_t);
uint64_t topo_end_by_memid(fmd_t *, uint64_t);
char *topo_pci_by_pcid(fmd_t *, uint64_t);
char *topo_storage_by_storageid(fmd_t *, uint64_t);
#endif
