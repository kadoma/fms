/*
 * bits.h
 *
 *  Created on: Feb 11, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	bits.h
 */

#ifndef BITS_H_
#define BITS_H_

#include <linux/types.h>


/**
 * Get bits of a num
 */
int inline
getbits(__u32 num)
{
	int power;

	for(power = 0; num; num = (num >> 1), power++) ;

	return power;
}

/**
 * Bits operation macros
 * Normal condition
 */
#define setbit(mem, offset) \
	((mem)[(offset) / 8] |= (1 << ((offset) % 8)))
#define clrbit(mem, offset) \
	((mem)[(offset) / 8] &= ~(1 << ((offset) % 8)))
#define isset(mem, offset) \
	((mem)[(offset) / 8] & (1 << ((offset) % 8)))
#define isclr(mem, offset) \
	((mem)[(offset) / 8] & (1 << ((offset) % 8)) == 0)

/**
 * Bits operation macros
 * For Completely Big-Endian format,
 * Especially for bitmap operations.
 */
#define setbit_b(mem, offset) \
	((mem)[(offset) / 8] |= (1 << (7 - (offset) % 8)))
#define clrbit_b(mem, offset) \
	((mem)[(offset) / 8] &= ~(1 << (7 - (offset) % 8)))
#define isset_b(mem, offset) \
	((mem)[(offset) / 8] & (1 << (7 - (offset) % 8)))
#define isclr_b(mem, offset) \
	((mem)[(offset) / 8] & (1 << (7 - (offset) % 8)) == 0)

#endif /* BITS_H_ */
