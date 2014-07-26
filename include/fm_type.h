/*
 * fm_type.h
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 * 	fm_type.h
 */

#ifndef	_FM_TYPE_H
#define	_FM_TYPE_H

#include <stdint.h>
#include <stdarg.h>
#include <byteswap.h>
#include <sys/types.h>

/*
 * POSIX Extensions
 */
typedef unsigned char           uchar_t;
typedef unsigned short          ushort_t;
typedef unsigned int            uint_t;
typedef unsigned long           ulong_t;
typedef long int                hrtime_t;
typedef enum { B_FALSE,B_TRUE } boolean_t;

/*
 * Macros to reverse byte order
 */
//#define BSWAP_8(x)		((x) & 0xff)
//#define BSWAP_16(x)		((BSWAP_8(x) << 8) | (BSWAP_8(x) >> 8))
//#define BSWAP_32(x)
//		(((uint32_t)(x) << 24) | \
//		(((uint32_t)(x) << 8) & 0xff0000) | \
//		(((uint32_t)(x) >> 8) & 0xff00) | \
//		((uint32_t)(x) >> 24))
#define BE_16(x)		bswap_16(x)
#define BE_32(x)                bswap_32(x)
#define BE_64(x)		bswap_64(x)

#endif	/* _FM_TYPE_H */
