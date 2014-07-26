/*
 * fmd_fmadm.h
 *
 *  Created on: Oct 18, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *      fmd_fmadm.h
 */

#ifndef FMD_FMADM_H_
#define FMD_FMADM_H_

#include <stdint.h>
#include <pthread.h>
#include <mqueue.h>
#include <errno.h>
#include <fmd_hash.h>
#include <fmd_list.h>
#include <fmd_case.h>

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

#define FAF_GET_CASELIST	"GET CASELIST"	/* get caselist command */

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

#define FAF_CASE_CREATE		0x00
#define FAF_CASE_FIRED		0x01
#define FAF_CASE_CLOSED		0x02

/*
 * The Fault Management Daemon Administration provides a very simple set of 
 * interfaces to the reset of fmd.
 * In the reference implementation, these are implemented to use FAF message.
 */

extern void fmd_adm_init(void);

#endif /* FMD_FMADM_H_ */
