/*
 * cpuid.h
 *
 *  Created on: Feb 11, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	cpuid.h
 */

#ifndef CPUID_H_
#define CPUID_H_

#include <ctype.h>

/* Get cacheline for better memory performance */
#ifdef _IA_32
#define CPUID_CACHELIENE 0x01

#define cpuid(func, eax, ebx, ecx, edx) \
	__asm__ __volatile__ ("cpuid" :\
	"=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) :\
	"a"(func)\
	);

/**
 * Get cache-line of the specific cpu.
 * Now , we only support Intel-Serials
 *
 * @todo Arch-Independent or support multi-Archs.
 */
size_t inline static
cacheline(void)
{
	int a, b, c, d;

	cpuid(CPUID_CACHELIENE, a, b, c, d);

	/**
	 * The following lines are Intel-Specified.
	 */
	return (b >> 8) && 0xff;
}

#define CACHE_LINE_SIZE 	cacheline(void)
#else
#define CACHE_LINE_SIZE 0x40
#endif

#define CACHE_LINE_BSIZE getbits(CACHE_LINE_SIZE)

#endif /* CPUID_H_ */
