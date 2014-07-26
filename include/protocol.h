/*
 * protocol.h
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 *      protocol.h
 */

#ifndef _FM_PROTOCOL_H
#define _FM_PROTOCOL_H

/* FM common member names */
#define FM_CLASS                        "class"
#define FM_VERSION                      "version"

/* FM event class values */
#define FM_EREPORT_CLASS                "ereport"

/* ereport version and payload member names */
#define FM_EREPORT_VERS0                0
#define FM_EREPORT_VERSION              FM_EREPORT_VERS0

/* ereport payload member names */
#define FM_EREPORT_DETECTOR             "detector"
#define FM_EREPORT_ENA                  "ena"

/*
 * FM ENA Format Macros
 */
#define ENA_FORMAT_MASK					0x3
#define ENA_FORMAT(ena)					((ena) & ENA_FORMAT_MASK)

/* ENA format types */
#define FM_ENA_FMT0						0
#define FM_ENA_FMT1						1
#define FM_ENA_FMT2						2

/* Format 1 */
#define ENA_FMT1_GEN_MASK				0x00000000000003FCull
#define ENA_FMT1_ID_MASK				0xFFFFFFFFFFFFFC00ull
#define ENA_FMT1_CPUID_MASK				0x00000000000FFC00ull
#define ENA_FMT1_TIME_MASK				0xFFFFFFFFFFF00000ull
#define ENA_FMT1_GEN_SHFT				2
#define ENA_FMT1_ID_SHFT                10
#define ENA_FMT1_CPUID_SHFT 			ENA_FMT1_ID_SHFT
#define ENA_FMT1_TIME_SHFT				20

/* Format 2 */
#define ENA_FMT2_GEN_MASK               0x00000000000003FCull
#define ENA_FMT2_ID_MASK                0xFFFFFFFFFFFFFC00ull
#define ENA_FMT2_TIME_MASK              ENA_FMT2_ID_MASK
#define ENA_FMT2_GEN_SHFT               2
#define ENA_FMT2_ID_SHFT                10
#define ENA_FMT2_TIME_SHFT				ENA_FMT2_ID_SHFT

#endif  /* _FM_PROTOCOL_H */
