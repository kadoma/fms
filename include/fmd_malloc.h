/*
 * fmd_malloc.h
 *
 *  Created on: Feb 11, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	fmd_malloc.h
 */

#ifndef FMD_MALLOC_H_
#define FMD_MALLOC_H_

#include <stdint.h>
#include <pthread.h>
#include <cpuid.h>
#include <fmd_list.h>

/* fmd' page_size */
#define PAGE_SIZE 0x1000

/* memory management unit. */
#define BLOCK_SIZE 		CACHE_LINE_SIZE
#define BLOCK_SIZE_MASK (CACHE_LINE_SIZE - 1)
#define BLOCK_BSIZE		CACHE_LINE_BSIZE

/*Block <-> VM*/
#define Block2VM(startvm, nrblock) \
	((char *)(startvm) + ((nrblock) << 6))
#define VM2Block(startvm, vm) \
	(((char *)(vm) - (char *)(startvm)) >> 6)

/**
 * memory requirement unit
 *
 * Size depends on bitmap.
 */
#define FMD_SLAB_SIZE (1 << (2 * CACHE_LINE_BSIZE + 3))

/**
 *  The basic definition
 *
 */
struct uslab_area {
	char bitmap[BLOCK_SIZE];

	/* Most-often visited member. */
	pthread_mutex_t mutex;
	void *vma;
	struct list_head list;
} __attribute__ ((aligned (CACHE_LINE_SIZE)));
LIST_HEAD(list_uslab);


#define USLAB_MAX_AREA (PAGE_SIZE / sizeof(struct uslab_area))

/**
 * The top-level definition
 *
 * struct uslab = [struct uslab_area]+
 */
static struct uslab {
	pthread_mutex_t mutex;
	int areas;
	struct uslab_area uslab_areas[USLAB_MAX_AREA];
} uslab;


/**
 * @param nbytes
 *
 * @return
 */
void *
fmd_alloc(int nbytes);


/**
 * @param
 *
 * @return
 */
int
fmd_free(void *ptr);


/**
 * Init
 *
 */
void
uslab_init(void) __attribute__ ((constructor));


/**
 * Destroy
 *
 */
void
uslab_exit(void) __attribute__ ((destructor));

#endif /* FMD_MALLOC_H_ */
