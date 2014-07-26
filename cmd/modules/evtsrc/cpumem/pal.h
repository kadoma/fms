/*
 * pal.h
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 *
 * Processor Abstraction Layer definitions.
 *
 * This is based on Intel IA-64 Architecture Software Developer's Manual rev 1.0
 * chapter 11 IA-64 Processor Abstraction Layer
 *
 * Copyright (C) 1998-2001 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 *	Stephane Eranian <eranian@hpl.hp.com>
 * Copyright (C) 1999 VA Linux Systems
 * Copyright (C) 1999 Walt Drummond <drummond@valinux.com>
 * Copyright (C) 1999 Srinivasa Prasad Thirumalachar <sprasad@sprasad.engr.sgi.com>
 *
 * 99/10/01	davidm	Make sure we pass zero for reserved parameters.
 * 00/03/07	davidm	Updated pal_cache_flush() to be in sync with PAL v2.6.
 * 00/03/23     cfleck  Modified processor min-state save area to match updated PAL & SAL info
 * 00/05/24     eranian Updated to latest PAL spec, fix structures bugs, added
 * 00/05/25	eranian Support for stack calls, and static physical calls
 * 00/06/18	eranian Support for stacked physical calls
 * 2003-11-05 Strip down to the bare minimum required to decode SAL records.
 *	      Update check_info structures to revision 2.1.
 *	      Keith Owens <kaos@sgi.com>
 */

#ifndef _PAL_H
#define _PAL_H

#include "mca_fm.h"

typedef struct pal_process_state_info_s {
	u64		reserved1	: 2,
			rz		: 1,	/* PAL_CHECK processor
						 * rendezvous
						 * successful.
						 */

			ra		: 1,	/* PAL_CHECK attempted
						 * a rendezvous.
						 */
			me		: 1,	/* Distinct multiple
						 * errors occurred
						 */

			mn		: 1,	/* Min. state save
						 * area has been
						 * registered with PAL
						 */

			sy		: 1,	/* Storage integrity
						 * synched
						 */


			co		: 1,	/* Continuable */
			ci		: 1,	/* MC isolated */
			us		: 1,	/* Uncontained storage
						 * damage.
						 */


			hd		: 1,	/* Non-essential hw
						 * lost (no loss of
						 * functionality)
						 * causing the
						 * processor to run in
						 * degraded mode.
						 */

			tl		: 1,	/* 1 => MC occurred
						 * after an instr was
						 * executed but before
						 * the trap that
						 * resulted from instr
						 * execution was
						 * generated.
						 * (Trap Lost )
						 */
			mi		: 1,	/* More information available
						 * call PAL_MC_ERROR_INFO
						 */
			pi		: 1,	/* Precise instruction pointer */
			pm		: 1,	/* Precise min-state save area */

			dy		: 1,	/* Processor dynamic
						 * state valid
						 */


			in		: 1,	/* 0 = MC, 1 = INIT */
			rs		: 1,	/* RSE valid */
			cm		: 1,	/* MC corrected */
			ex		: 1,	/* MC is expected */
			cr		: 1,	/* Control regs valid*/
			pc		: 1,	/* Perf cntrs valid */
			dr		: 1,	/* Debug regs valid */
			tr		: 1,	/* Translation regs
						 * valid
						 */
			rr		: 1,	/* Region regs valid */
			ar		: 1,	/* App regs valid */
			br		: 1,	/* Branch regs valid */
			pr		: 1,	/* Predicate registers
						 * valid
						 */

			fp		: 1,	/* fp registers valid*/
			b1		: 1,	/* Preserved bank one
						 * general registers
						 * are valid
						 */
			b0		: 1,	/* Preserved bank zero
						 * general registers
						 * are valid
						 */
			gr		: 1,	/* General registers
						 * are valid
						 * (excl. banked regs)
						 */
			dsize		: 16,	/* size of dynamic
						 * state returned
						 * by the processor
						 */

			reserved2	: 11,
			cc		: 1,	/* Cache check */
			tc		: 1,	/* TLB check */
			bc		: 1,	/* Bus check */
			rc		: 1,	/* Register file check */
			uc		: 1;	/* Uarch check */

} pal_processor_state_info_t;

typedef struct pal_cache_check_info_s {
	u64		op		: 4,	/* Type of cache
						 * operation that
						 * caused the machine
						 * check.
						 */
			level		: 2,	/* Cache level */
			reserved1	: 2,
			dl		: 1,	/* Failure in data part
						 * of cache line
						 */
			tl		: 1,	/* Failure in tag part
						 * of cache line
						 */
			dc		: 1,	/* Failure in dcache */
			ic		: 1,	/* Failure in icache */
			mesi		: 3,	/* Cache line state */
			mv		: 1,	/* mesi valid */
			way		: 5,	/* Way in which the
						 * error occurred
						 */
			wiv		: 1,	/* Way field valid */
			reserved2	: 1,
			dp		: 1,	/* Data poisoned on MBE */
			reserved3	: 8,

			index		: 20,	/* Cache line index */
			reserved4	: 2,

			is		: 1,	/* instruction set (1 == ia32) */
			iv		: 1,	/* instruction set field valid */
			pl		: 2,	/* privilege level */
			pv		: 1,	/* privilege level field valid */
			mcc		: 1,	/* Machine check corrected */
			tv		: 1,	/* Target address
						 * structure is valid
						 */
			rq		: 1,	/* Requester identifier
						 * structure is valid
						 */
			rp		: 1,	/* Responder identifier
						 * structure is valid
						 */
			pi		: 1;	/* Precise instruction pointer
						 * structure is valid
						 */
} pal_cache_check_info_t;

typedef struct pal_tlb_check_info_s {

	u64		tr_slot		: 8,	/* Slot# of TR where
						 * error occurred
						 */
			trv		: 1,	/* tr_slot field is valid */
			reserved1	: 1,
			level		: 2,	/* TLB level where failure occurred */
			reserved2	: 4,
			dtr		: 1,	/* Fail in data TR */
			itr		: 1,	/* Fail in inst TR */
			dtc		: 1,	/* Fail in data TC */
			itc		: 1,	/* Fail in inst. TC */
			op		: 4,	/* Cache operation */
			reserved3	: 30,

			is		: 1,	/* instruction set (1 == ia32) */
			iv		: 1,	/* instruction set field valid */
			pl		: 2,	/* privilege level */
			pv		: 1,	/* privilege level field valid */
			mcc		: 1,	/* Machine check corrected */
			tv		: 1,	/* Target address
						 * structure is valid
						 */
			rq		: 1,	/* Requester identifier
						 * structure is valid
						 */
			rp		: 1,	/* Responder identifier
						 * structure is valid
						 */
			pi		: 1;	/* Precise instruction pointer
						 * structure is valid
						 */
} pal_tlb_check_info_t;

typedef struct pal_bus_check_info_s {
	u64		size		: 5,	/* Xaction size */
			ib		: 1,	/* Internal bus error */
			eb		: 1,	/* External bus error */
			cc		: 1,	/* Error occurred
						 * during cache-cache
						 * transfer.
						 */
			type		: 8,	/* Bus xaction type*/
			sev		: 5,	/* Bus error severity*/
			hier		: 2,	/* Bus hierarchy level */
			dp		: 1,	/* Data poisoned on MBE */
			bsi		: 8,	/* Bus error status
						 * info
						 */
			reserved2	: 22,

			is		: 1,	/* instruction set (1 == ia32) */
			iv		: 1,	/* instruction set field valid */
			pl		: 2,	/* privilege level */
			pv		: 1,	/* privilege level field valid */
			mcc		: 1,	/* Machine check corrected */
			tv		: 1,	/* Target address
						 * structure is valid
						 */
			rq		: 1,	/* Requester identifier
						 * structure is valid
						 */
			rp		: 1,	/* Responder identifier
						 * structure is valid
						 */
			pi		: 1;	/* Precise instruction pointer
						 * structure is valid
						 */
} pal_bus_check_info_t;

typedef struct pal_reg_file_check_info_s {
	u64		id		: 4,	/* Register file identifier */
			op		: 4,	/* Type of register
						 * operation that
						 * caused the machine
						 * check.
						 */
			reg_num		: 7,	/* Register number */
			rnv		: 1,	/* reg_num valid */
			reserved2	: 38,

			is		: 1,	/* instruction set (1 == ia32) */
			iv		: 1,	/* instruction set field valid */
			pl		: 2,	/* privilege level */
			pv		: 1,	/* privilege level field valid */
			mcc		: 1,	/* Machine check corrected */
			reserved3	: 3,
			pi		: 1;	/* Precise instruction pointer
						 * structure is valid
						 */
} pal_reg_file_check_info_t;

typedef struct pal_uarch_check_info_s {
	u64		sid		: 5,	/* Structure identification */
			level		: 3,	/* Level of failure */
			array_id	: 4,	/* Array identification */
			op		: 4,	/* Type of
						 * operation that
						 * caused the machine
						 * check.
						 */
			way		: 6,	/* Way of structure */
			wv		: 1,	/* way valid */
			xv		: 1,	/* index valid */
			reserved1	: 8,
			index		: 8,	/* Index or set of the uarch
						 * structure that failed.
						 */
			reserved2	: 24,

			is		: 1,	/* instruction set (1 == ia32) */
			iv		: 1,	/* instruction set field valid */
			pl		: 2,	/* privilege level */
			pv		: 1,	/* privilege level field valid */
			mcc		: 1,	/* Machine check corrected */
			tv		: 1,	/* Target address
						 * structure is valid
						 */
			rq		: 1,	/* Requester identifier
						 * structure is valid
						 */
			rp		: 1,	/* Responder identifier
						 * structure is valid
						 */
			pi		: 1;	/* Precise instruction pointer
						 * structure is valid
						 */
} pal_uarch_check_info_t;

typedef union pal_mc_error_info_u {
	u64				pmei_data;
	pal_processor_state_info_t	pme_processor;
	pal_cache_check_info_t		pme_cache;
	pal_tlb_check_info_t		pme_tlb;
	pal_bus_check_info_t		pme_bus;
	pal_reg_file_check_info_t	pme_reg_file;
	pal_uarch_check_info_t		pme_uarch;
} pal_mc_error_info_t;

/*
 * NOTE: this min_state_save area struct only includes the 1KB
 * architectural state save area.  The other 3 KB is scratch space
 * for PAL.
 */

typedef struct pal_min_state_area_s {
	u64	pmsa_nat_bits;		/* nat bits for saved GRs  */
	u64	pmsa_gr[15];		/* GR1	- GR15		   */
	u64	pmsa_bank0_gr[16];	/* GR16 - GR31		   */
	u64	pmsa_bank1_gr[16];	/* GR16 - GR31		   */
	u64	pmsa_pr;		/* predicate registers	   */
	u64	pmsa_br0;		/* branch register 0	   */
	u64	pmsa_rsc;		/* ar.rsc		   */
	u64	pmsa_iip;		/* cr.iip		   */
	u64	pmsa_ipsr;		/* cr.ipsr		   */
	u64	pmsa_ifs;		/* cr.ifs		   */
	u64	pmsa_xip;		/* previous iip		   */
	u64	pmsa_xpsr;		/* previous psr		   */
	u64	pmsa_xfs;		/* previous ifs		   */
	u64	pmsa_br1;		/* branch register 1	   */
	u64	pmsa_reserved[70];	/* pal_min_state_area should total to 1KB */
} pal_min_state_area_t;

#endif /* _PAL_H */
