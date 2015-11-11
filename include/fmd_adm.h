
#ifndef	_FMD_ADM_H
#define	_FMD_ADM_H 1

#include <mqueue.h>

#include "list.h"
#include "fmd_fmadm.h"
#include "fmd_topo.h"

/*
 * Fault Management Daemon Administrative Interfaces
 *
 * Note: The contents of this file are private to the implementation of the
 * Solaris system and FMD subsystem and are subject to change at any time
 * without notice.  Applications and drivers using these interfaces will fail
 * to run on future releases.  These interfaces should not be used for any
 * purpose until they are publicly documented for use outside of Sun. <--- (Ben: OMG!!! ^O^!!!)
 */

#define	FMD_ADM_VERSION	1			/* library ABI interface version */
#define	FMD_ADM_PROGRAM	0			/* connect library to system fmd */

#define FMD_ADM_BUFFER_SIZE 128 * 1024

typedef struct fmd_adm {
	mqd_t adm_mqd;				/* message queue handle for connection */
	char adm_buf[FMD_ADM_BUFFER_SIZE];	/* data buffer */
	int adm_svcerr;				/* server-side error from last call */
	int adm_errno;				/* client-side error from last call */
	struct list_head cs_list;		/* case info list */
    struct list_head mod_list;
} fmd_adm_t;

extern fmd_adm_t *fmd_adm_open(void);
extern void fmd_adm_close(fmd_adm_t *);
extern const char *fmd_adm_errmsg(fmd_adm_t *);

enum fmd_adm_error {
	FMD_ADM_ERR_NOMEM,
	FMD_ADM_ERR_PERM,
	FMD_ADM_ERR_MODSRCH,
	FMD_ADM_ERR_MODBUSY,
	FMD_ADM_ERR_MODFAIL,
	FMD_ADM_ERR_MODNOENT,
	FMD_ADM_ERR_MODEXIST,
	FMD_ADM_ERR_MODINIT,
	FMD_ADM_ERR_MODLOAD,
	FMD_ADM_ERR_RSRCSRCH,
	FMD_ADM_ERR_RSRCNOTF,
	FMD_ADM_ERR_SERDSRCH,
	FMD_ADM_ERR_SERDFIRED,
	FMD_ADM_ERR_ROTSRCH,
	FMD_ADM_ERR_ROTFAIL,
	FMD_ADM_ERR_ROTBUSY,
	FMD_ADM_ERR_CASESRCH,
	FMD_ADM_ERR_CASEOPEN,
	FMD_ADM_ERR_XPRTSRCH,
	FMD_ADM_ERR_CASEXPRT,
	FMD_ADM_ERR_RSRCNOTR
};

#if 0
typedef int fmd_adm_module_f(const fmd_adm_modinfo_t *, void *);

extern int fmd_adm_module_iter(fmd_adm_t *, fmd_adm_module_f *, void *);
extern int fmd_adm_module_load(fmd_adm_t *, const char *);
extern int fmd_adm_module_unload(fmd_adm_t *, const char *);
extern int fmd_adm_module_reset(fmd_adm_t *, const char *);
extern int fmd_adm_module_stats(fmd_adm_t *, const char *, fmd_adm_stats_t *);
extern int fmd_adm_module_gc(fmd_adm_t *, const char *);
#endif

typedef struct fmd_adm_caseinfo {
	char aci_fru[256];
	char aci_asru[256];
	faf_case_t fafc;
	struct list_head aci_list;
} fmd_adm_caseinfo_t;

typedef struct fmd_adm_modinfo {
	faf_module_t fafm;
	struct list_head mod_list;
} fmd_adm_modinfo_t;

extern int check_module(char*);
extern int check_module_conf(char*);
extern int fmd_adm_case_iter(fmd_adm_t *);
extern int fmd_adm_load_module(fmd_adm_t * ,char *);
extern int fmd_adm_mod_iter(fmd_adm_t *);
extern int fmd_adm_unload_module(fmd_adm_t *,char *);
#endif	/* _FMD_ADM_H */

