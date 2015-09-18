/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	haswell.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-04
 * Description:	
 *				parse cpu(Haswell) error. 
 *
 */

#define _GUN_SOURCE	1
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cpumem_evtsrc.h"
#include "bitfield.h"
#include "haswell.h"

/* for fms define */
static char *fms_pcu_1[] = {
	[0x00] = "pcu_none",
	[0x09] = "pcu_mmct",
	[0x0D] = "pcu_mifsst",
	[0x0E] = "pcu_mcust",
	[0x13] = "pcu_mdtt",
	[0x15] = "pcu_mdcrat",
	[0x1E] = "pcu_mvimlfim",
	[0x25] = "pcu_msct",
	[0x29] = "pcu_mvvmlfs",
	[0x2B] = "pcu_mpwhcd",
	[0x2C] = "pcu_mpwhcu",
	[0x39] = "pcu_mpwhcus",
	[0x44] = "pcu_mcvf",
	[0x45] = "pcu_mimn",
	[0x46] = "pcu_mvrdf",
	[0x47] = "pcu_memnpc",
	[0x48] = "pcu_msrrimf",
	[0x49] = "pcu_mswrvmf",
	[0x4B] = "pcu_mbvtd0",
	[0x4C] = "pcu_mbvtd1",
	[0x4D] = "pcu_mbvtd2",
	[0x4E] = "pcu_mbvtd3",
	[0x4F] = "pcu_msce",
	[0x52] = "pcu_mfcovf",
	[0x53] = "pcu_mfcocf",
	[0x57] = "pcu_msprf",
	[0x58] = "pcu_msirf",
	[0x59] = "pcu_msarf",
	[0x60] = "pcu_miprep",
	[0x61] = "pcu_mipreq",
	[0x62] = "pcu_miprsq",
	[0x63] = "pcu_miprsp",
	[0x64] = "pcu_mipsc",
	[0x67] = "pcu_mhirbat",
	[0x68] = "pcu_mirst",
	[0x69] = "pcu_mhfcd",
	[0x6A] = "pcu_mmpct",
	[0x70] = "pcu_mwtps",
	[0x71] = "pcu_mwtpcm",
	[0x72] = "pcu_mwtpsm",
	[0x7C] = "pcu_mbrcis",
	[0x7D] = "pcu_mmtota",
	[0x81] = "pcu_mrdtth"
};

static struct field fms_pcu_mc4[] = {
	FIELD(24, fms_pcu_1),
	{}
};

static char *fms_qpi[] = {
	[0x02] = "qpi_plddba",
	[0x03] = "qpi_pldlbr",
	[0x10] = "qpi_lldcefr",
	[0x11] = "qpi_relasoce",
	[0x12] = "qpi_uoup",
	[0x13] = "qpi_llce",
	[0x15] = "qpi_ruuv",
	[0x20] = "qpi_pldqir",
	[0x21] = "qpi_lfdsh",
	[0x22] = "qpi_pdir",
	[0x23] = "qpi_lfcf",
	[0x30] = "qpi_rdcea",
	[0x31] = "qpi_rdcew",
};

static struct field fms_qpi_mc[] = {
	FIELD(16, fms_qpi),
	{}
};

static struct field fms_memctrl_mc9[] = {
	SBITFIELD(16, "mc_d3addr_parity"),
	SBITFIELD(17, "mc_uha_wd"),
	SBITFIELD(18, "mc_uha_dbe"),
	SBITFIELD(19, "mc_cps"),
	SBITFIELD(20, "mc_ups"),
	SBITFIELD(21, "mc_cspare"),
	SBITFIELD(22, "mc_uspare"),
	SBITFIELD(23, "mc_memrd"),
	SBITFIELD(24, "mc_wdb_parity"),
	SBITFIELD(25, "mc_d4caddr_parity"),
	{}
};


/* See IA32 SDM Vol3B Table 16-20 */

static char *pcu_1[] = {
	[0x00] = "No Error",
	[0x09] = "MC_MESSAGE_CHANNEL_TIMEOUT",
	[0x0D] = "MC_IMC_FORCE_SR_S3_TIMEOUT",
	[0x0E] = "MC_CPD_UNCPD_SD_TIMEOUT",
	[0x13] = "MC_DMI_TRAINING_TIMEOUT",
	[0x15] = "MC_DMI_CPU_RESET_ACK_TIMEOUT",
	[0x1E] = "MC_VR_ICC_MAX_LT_FUSED_ICC_MAX",
	[0x25] = "MC_SVID_COMMAN_TIMEOUT",
	[0x29] = "MC_VR_VOUT_MAC_LT_FUSED_SVID",
	[0x2B] = "MC_PKGC_WATCHDOG_HANG_CBZ_DOWN",
	[0x2C] = "MC_PKGC_WATCHDOG_HANG_CBZ_UP",
	[0x39] = "MC_PKGC_WATCHDOG_HANG_C3_UP_SF",
	[0x44] = "MC_CRITICAL_VR_FAILED",
	[0x45] = "MC_ICC_MAX_NOTSUPPORTED",
	[0x46] = "MC_VID_RAMP_DOWN_FAILED",
	[0x47] = "MC_EXCL_MODE_NO_PMREQ_CMP",
	[0x48] = "MC_SVID_READ_REG_ICC_MAX_FAILED",
	[0x49] = "MC_SVID_WRITE_REG_VOUT_MAX_FAILED",
	[0x4B] = "MC_BOOT_VID_TIMEOUT_DRAM_0",
	[0x4C] = "MC_BOOT_VID_TIMEOUT_DRAM_1",
	[0x4D] = "MC_BOOT_VID_TIMEOUT_DRAM_2",
	[0x4E] = "MC_BOOT_VID_TIMEOUT_DRAM_3",
	[0x4F] = "MC_SVID_COMMAND_ERROR",
	[0x52] = "MC_FIVR_CATAS_OVERVOL_FAULT",
	[0x53] = "MC_FIVR_CATAS_OVERCUR_FAULT",
	[0x57] = "MC_SVID_PKGC_REQUEST_FAILED",
	[0x58] = "MC_SVID_IMON_REQUEST_FAILED",
	[0x59] = "MC_SVID_ALERT_REQUEST_FAILED",
	[0x60] = "MC_INVALID_PKGS_REQ_PCH",
	[0x61] = "MC_INVALID_PKGS_REQ_QPI",
	[0x62] = "MC_INVALID_PKGS_RSP_QPI",
	[0x63] = "MC_INVALID_PKGS_RSP_PCH",
	[0x64] = "MC_INVALID_PKG_STATE_CONFIG",
	[0x67] = "MC_HA_IMC_RW_BLOCK_ACK_TIMEOUT",
	[0x68] = "MC_IMC_RW_SMBUS_TIMEOUT",
	[0x69] = "MC_HA_FAILSTS_CHANGE_DETECTED",
	[0x6A] = "MC_MSGCH_PMREQ_CMP_TIMEOUT",
	[0x70] = "MC_WATCHDOG_TIMEOUT_PKGC_SLAVE",
	[0x71] = "MC_WATCHDOG_TIMEOUT_PKGC_MASTER",
	[0x72] = "MC_WATCHDOG_TIMEOUT_PKGS_MASTER",
	[0x7C] = "MC_BIOS_RST_CPL_INVALID_SEQ",
	[0x7D] = "MC_MORE_THAN_ONE_TXT_AGENT",
	[0x81] = "MC_RECOVERABLE_DIE_THERMAL_TOO_HOT"
};

static struct field pcu_mc4[] = {
	FIELD(24, pcu_1),
	{}
};

/* See IA32 SDM Vol3B Table 16-21 */

static char *qpi[] = {
	[0x02] = "Intel QPI physical layer detected drift buffer alarm",
	[0x03] = "Intel QPI physical layer detected latency buffer rollover",
	[0x10] = "Intel QPI link layer detected control error from R3QPI",
	[0x11] = "Rx entered LLR abort state on CRC error",
	[0x12] = "Unsupported or undefined packet",
	[0x13] = "Intel QPI link layer control error",
	[0x15] = "RBT used un-initialized value",
	[0x20] = "Intel QPI physical layer detected a QPI in-band reset but aborted initialization",
	[0x21] = "Link failover data self healing",
	[0x22] = "Phy detected in-band reset (no width change)",
	[0x23] = "Link failover clock failover",
	[0x30] = "Rx detected CRC error - successful LLR after Phy re-init",
	[0x31] = "Rx detected CRC error - successful LLR wihout Phy re-init",
};

static struct field qpi_mc[] = {
	FIELD(16, qpi),
	{}
};

/* See IA32 SDM Vol3B Table 16-22 */

static struct field memctrl_mc9[] = {
	SBITFIELD(16, "DDR3 address parity error"),
	SBITFIELD(17, "Uncorrected HA write data error"),
	SBITFIELD(18, "Uncorrected HA data byte enable error"),
	SBITFIELD(19, "Corrected patrol scrub error"),
	SBITFIELD(20, "Uncorrected patrol scrub error"),
	SBITFIELD(21, "Corrected spare error"),
	SBITFIELD(22, "Uncorrected spare error"),
	SBITFIELD(23, "Corrected memory read error"),
	SBITFIELD(24, "iMC write data buffer parity error"),
	SBITFIELD(25, "DDR4 command address parity error"),
	{}
};


void 
hsw_decode_model(int cputype, int bank, u64 status, u64 misc, 
			struct mc_msg *mm, int *mm_num)
{
	char buf[64] = {0};
	
	switch (bank) {
	case 4:
		switch (EXTRACT(status, 0, 15) & ~(1ull << 12)) {
		case 0x402: case 0x403:
			sprintf(buf, "Internal errors ");
			break;
		case 0x406:
			sprintf(buf, "Intel TXT errors ");
			break;
		case 0x407:
			sprintf(buf, "Other UBOX Internal errors ");
			break;
		}
		if (EXTRACT(status, 16, 19))
			sprintf(buf+strlen(buf), "PCU internal error ");
		decode_bitfield(status, fms_pcu_mc4, buf, mm, mm_num, "pcu", 1);	/* mnemonic */
		decode_bitfield(status, pcu_mc4, buf, mm, mm_num, NULL, 0);
		break;
	case 5:
	case 20:
	case 21:
		decode_bitfield(status, fms_qpi_mc, NULL, mm, mm_num, "qpi", 1);
		decode_bitfield(status, qpi_mc, NULL, mm, mm_num, NULL, 0);
		break;
	case 9: case 10: case 11: case 12:
	case 13: case 14: case 15: case 16:
		decode_bitfield(status, fms_memctrl_mc9, NULL, mm, mm_num, "mc", 1);
		decode_bitfield(status, memctrl_mc9, NULL, mm, mm_num, NULL, 0);
		break;
	}
}
