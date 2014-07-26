/*
 * fmd_malloc.c
 *
 *  Created on: Sep 24, 2009
 *      Author: Inspur OS Team
 *
 *	Description:
 *		Userland slab memory allocator, which should be
 *	automatically loaded while starting.
 *		This is a special module of fmd to refactoring.
 *		We use the simplest algorithm to allocate pages,
 *	that's, one unit by another, no reserving.
 *
 * @TODO Stupid lock & unlock
 */

#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <syslog.h>
#include <fmd.h>
#include <bits.h>
#include <cpuid.h>
#include <fmd_list.h>
#include <fmd_malloc.h>


static int
fmd_expand(void)
{
	int max = USLAB_MAX_AREA;
	struct uslab_area *area;

	if(uslab.areas == max) {
		syslog(LOG_INFO, "Fms has used up memory quotas.\n");
		return -ENOMEM;
	}

	/* Requires new memory unit from OS */
	area = uslab.uslab_areas + uslab.areas++;
	area->vma = malloc(FMD_SLAB_SIZE);
	if(!area->vma) {
		syslog(LOG_INFO, "Fms has used up memory quotas.\n");
		return -ENOMEM;
	}
	pthread_mutex_init(&area->mutex, NULL);

	return 0;
}


/**
 * Inner function
 * To find consequential blocks.
 *
 * @todo KMP or other algorithms should be applied.
 */
static size_t
__get_blocks(char *bitmap, int nblocks)
{
	int i, maxoff = (BLOCK_SIZE << 3);
	int pos, startpos;
	int flag = 0;

	pos = 0;
	while(pos < maxoff) {
		while(isset_b(bitmap, pos)) {
			if(++pos == maxoff)
				goto outofbounds;
			continue;
		}

		if(pos + nblocks > maxoff)
			goto outofbounds;

		flag = 1;
		startpos = pos;
		for(i = 1; i < nblocks; i++) {
			if(isset_b(bitmap, ++pos)) {
				flag = 0;
				break;
			}
		}

		if(flag)
			break;

		pos++;
	}

	return startpos;

outofbounds:
	syslog(LOG_INFO, "Out of bounds.\n");
	return -1;
}

/**
 * @param area
 * @param blocknr
 * 	The starting block number in the @area
 * @param count
 * 	The total blcok value.
 */
static void *
internal_alloc(struct uslab_area *area, int blocknr, int count)
{
	int i;

	for(i = 0; i < count; i++) {
		setbit_b(area->bitmap, blocknr + i);
	}

	return Block2VM(area->vma, blocknr);
}


/**
 * @param nbytes
 *
 * @return
 */
void *
fmd_alloc(int nbytes)
{
	size_t sz = (nbytes + BLOCK_SIZE_MASK) & BLOCK_SIZE;
	size_t blocks;
	int i, nrblock;
	struct uslab_area *area;

	if(sz >= FMD_SLAB_SIZE) {
		syslog(LOG_INFO, "Allocated memory are too large for fmd.\n");
		return ERR_PTR(-EINVAL);
	}
	if(sz < 0) {
		syslog(LOG_INFO, "Invalid number, should be greater than zero.\n");
		return ERR_PTR(-EINVAL);
	}

	/* stupid lock */
	pthread_mutex_lock(&uslab.mutex);

	blocks = (sz >> BLOCK_BSIZE);

	/* Search for empty blocks */
	for(i = 0; i < uslab.areas; i++) {
		area = uslab.uslab_areas + i;
		if(nrblock = __get_blocks(area->bitmap, blocks)) {
			return internal_alloc(area, nrblock, blocks);
		}
	}

	if(i == USLAB_MAX_AREA) {
		syslog(LOG_INFO, "Exceeding memory quotas.\n");
		return NULL;
	} else { // expanding...
		if(fmd_expand() == 0) {
			/* Expanding succeeded */
			area = uslab.uslab_areas + uslab.areas - 1;
			return internal_alloc(area, 0, blocks);
		}
		else {
			syslog(LOG_INFO, "Failed to expanding.\n");
			return NULL;
		}
	}

	/* stupid unlock */
	pthread_mutex_unlock(&uslab.mutex);
}


/**
 * @param
 *
 * @return
 */
int
fmd_free(void *ptr)
{
	int i;
	char *p, *_ptr;
	struct uslab_area *area;

	p = (char *)ptr;

	/* ptr must be cacheline aligned */


	/* Search for empty blocks */
	for(i = 0; i < uslab.areas; i++) {
		area = uslab.uslab_areas + i;
		_ptr = (char *)area->vma;
		if(p < _ptr || p >= _ptr + FMD_SLAB_SIZE)
			continue;
		else
			break;
	}

	if(i == USLAB_MAX_AREA) {
		syslog(LOG_INFO, "The memory are not allocated by fmd.\n");
		return -EINVAL;
	}

	clrbit_b(area->bitmap, VM2Block(_ptr, p));
	return 0;
}


/**
 * Init
 *
 */
void __attribute__ ((constructor))
uslab_init(void)
{
	memset((void *)&uslab, 0, sizeof(uslab));
}

/**
 * Destroy
 *
 */
void __attribute__ ((destructor))
uslab_exit(void)
{
	int i;
	struct uslab_area *area;

	for(i = 0; i < USLAB_MAX_AREA; i++) {
		area = uslab.uslab_areas + i;
		if(area->vma)
			free(area->vma);
	}
}
