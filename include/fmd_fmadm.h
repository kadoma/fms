
#ifndef FMD_FMADM_H_
#define FMD_FMADM_H_ 1

#include <stdint.h>
#include <pthread.h>
#include <mqueue.h>
#include <errno.h>
#include <assert.h>
#include <list.h>
#include <fmd.h>
/*
 * Fault Management Daemon Administration Message Queue Format (FAF)
 *
 * Fault Management Daemon communicate with Administration through Message Queue.
 * The message layout is structured as follows:
 *
 * +------------------+----- ... ----+---- ... ------+
 * |    faf_hdr_t     |   section    |   section     |
 * | (message header) |   #1 data    |   #N data     |
 * +------------------+---- ... -----+---- ... ------+
 * |<--------------- faf_hdr.fafh_msgsz ------------>|
 *
 */

typedef struct faf_hdr {
	char	 fafh_cmd[32];		/* message command (see below) */
	uint32_t fafh_hdrsize;		/* size of message header in bytes */
	uint32_t fafh_secnum;		/* number of sections */
	uint64_t fafh_msgsz;		/* message size of entire FAF message */
} faf_hdr_t;

#define FAF_GET_CASELIST "GET CASELIST"	/* get caselist command */
#define FAF_GET_MODLIST "GET MODLIST"
#define FAF_NUM 20
typedef struct faf_case {
		char     fafc_fault[128];	/* for fmd_case_type use */
        uint64_t fafc_uuid;		/* case uuid */
        uint64_t fafc_rscid;
        uint8_t  fafc_state;		/* case state (see below) */
        uint32_t fafc_count;
        uint64_t fafc_create;
        uint64_t fafc_fire;
        uint64_t fafc_close;
} faf_case_t;

typedef struct faf_module {
	//char             *mod_name;
	int	          mod_vers;
	int               mod_interval;
	char              mod_name[128];
} faf_module_t;


#define FAF_CASE_CREATE		0x00
#define FAF_CASE_FIRED		0x01
#define FAF_CASE_CLOSED		0x02

/*
 * The Fault Management Daemon Administration provides a very simple set of
 * interfaces to the reset of fmd.
 * In the reference implementation, these are implemented to use FAF message.
 */

extern void fmd_adm_init(void);
extern int evt_load_module(fmd_t * ,char *);
extern int evt_unload_module(fmd_t * ,char *);
extern int fmd_init_module(fmd_t * ,char *);
//extern int free_module(fmd_module_t *);
#endif /* FMD_FMADM_H_ */
