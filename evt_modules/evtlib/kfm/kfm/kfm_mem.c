/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	kfm_mem.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-26
 * Description:	
 *				Memory operation.
 *
 */

#include <linux/page-flags.h>
#include <linux/kernel-page-flags.h>
#include <linux/mm.h>

#include "kfm.h"

static inline u64 kpf_copy_bit(u64 kflags, int ubit, int kbit)
{
	return ((kflags >> kbit) & 1) << ubit;
}

u64 kfm_stable_page_flags(struct page *page)
{
	u64 k;
	u64 u;

	/*
	 * pseudo flag: KPF_NOPAGE
	 * it differentiates a memory hole from a page with no flags
	 */
	if (!page)
		return 1 << KPF_NOPAGE;

	k = page->flags;
	u = 0;

	/*
	 * pseudo flags for the well known (anonymous) memory mapped pages
	 *
	 * Note that page->_mapcount is overloaded in SLOB/SLUB/SLQB, so the
	 * simple test in page_mapped() is not enough.
	 */
	if (!PageSlab(page) && page_mapped(page))
		u |= 1 << KPF_MMAP;
	if (PageAnon(page))
		u |= 1 << KPF_ANON;
//	if (PageKsm(page))
//		u |= 1 << KPF_KSM;

	/*
	 * compound pages: export both head/tail info
	 * they together define a compound page's start/end pos and order
	 */
	if (PageHead(page))
		u |= 1 << KPF_COMPOUND_HEAD;
	if (PageTail(page))
		u |= 1 << KPF_COMPOUND_TAIL;
//	if (PageHuge(page))
//		u |= 1 << KPF_HUGE;
	/*
	 * PageTransCompound can be true for non-huge compound pages (slab
	 * pages or pages allocated by drivers with __GFP_COMP) because it
	 * just checks PG_head/PG_tail, so we need to check PageLRU/PageAnon
	 * to make sure a given page is a thp, not a non-huge compound page.
	 */
	else if (PageTransCompound(page)) {
		struct page *head = compound_head(page);

		if (PageLRU(head) || PageAnon(head))
			u |= 1 << KPF_THP;
//		else if (is_huge_zero_page(head)) {
//			u |= 1 << KPF_ZERO_PAGE;
//			u |= 1 << KPF_THP;
//		}
	} 
//	else if (is_zero_pfn(page_to_pfn(page)))
//		u |= 1 << KPF_ZERO_PAGE;


	/*
	 * Caveats on high order pages: page->_count will only be set
	 * -1 on the head page; SLUB/SLQB do the same for PG_slab;
	 * SLOB won't set PG_slab at all on compound pages.
	 */
	if (PageBuddy(page))
		u |= 1 << KPF_BUDDY;

//	if (PageBalloon(page))
//		u |= 1 << KPF_BALLOON;

	u |= kpf_copy_bit(k, KPF_LOCKED,	PG_locked);

	u |= kpf_copy_bit(k, KPF_SLAB,		PG_slab);

	u |= kpf_copy_bit(k, KPF_ERROR,		PG_error);
	u |= kpf_copy_bit(k, KPF_DIRTY,		PG_dirty);
	u |= kpf_copy_bit(k, KPF_UPTODATE,	PG_uptodate);
	u |= kpf_copy_bit(k, KPF_WRITEBACK,	PG_writeback);

	u |= kpf_copy_bit(k, KPF_LRU,		PG_lru);
	u |= kpf_copy_bit(k, KPF_REFERENCED,	PG_referenced);
	u |= kpf_copy_bit(k, KPF_ACTIVE,	PG_active);
	u |= kpf_copy_bit(k, KPF_RECLAIM,	PG_reclaim);

	u |= kpf_copy_bit(k, KPF_SWAPCACHE,	PG_swapcache);
	u |= kpf_copy_bit(k, KPF_SWAPBACKED,	PG_swapbacked);

	u |= kpf_copy_bit(k, KPF_UNEVICTABLE,	PG_unevictable);
	u |= kpf_copy_bit(k, KPF_MLOCKED,	PG_mlocked);

#ifdef CONFIG_MEMORY_FAILURE
	u |= kpf_copy_bit(k, KPF_HWPOISON,	PG_hwpoison);
#endif

#ifdef CONFIG_ARCH_USES_PG_UNCACHED
	u |= kpf_copy_bit(k, KPF_UNCACHED,	PG_uncached);
#endif

	u |= kpf_copy_bit(k, KPF_RESERVED,	PG_reserved);
	u |= kpf_copy_bit(k, KPF_MAPPEDTODISK,	PG_mappedtodisk);
	u |= kpf_copy_bit(k, KPF_PRIVATE,	PG_private);
	u |= kpf_copy_bit(k, KPF_PRIVATE_2,	PG_private_2);
	u |= kpf_copy_bit(k, KPF_OWNER_PRIVATE,	PG_owner_priv_1);
	u |= kpf_copy_bit(k, KPF_ARCH,		PG_arch_1);

	return u;
};

/*
 * @addr: memory physical address.
 *
 */
int
kfm_get_mempage_info(u64 addr, struct kfm_mempage_u *umempage)
{
	struct page *ppage;
	unsigned long pfn;
	
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	pfn = addr >> PAGE_SHIFT;
	
	if (pfn_valid(pfn))
		ppage = pfn_to_page(pfn);
	else
		ppage = NULL;

	umempage->pfn = pfn;
	umempage->flags = kfm_stable_page_flags(ppage);
	
	return 0;
}

int
kfm_mempage_online(u64 addr)
{
/* num_poisoned_pages, no EXPORT_SYMBOL(), need*/
#if 0
	struct page *ppage;
	unsigned long pfn;

	EnterFunction(7);
	
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	
	pfn = addr >> PAGE_SHIFT;
	
	if (!pfn_valid(pfn))
		return -ENXIO;

	ppage = pfn_to_page(pfn);

	KFM_DBG(7, "num_poisoned_pages: %lu", atomic_long_read(&num_poisoned_pages));
	lock_page(ppage);
	if (PageHWPoison(ppage)) {
		atomic_long_sub(1, &num_poisoned_pages);
		ClearPageHWPoison(ppage);
		
		KFM_DBG(7, "onfine page, num_poisoned_pages: %lu", 
			atomic_long_read(&num_poisoned_pages));
	}
	unlock_page(ppage);
	LeaveFunction(7);
#endif	
	return 0;
}
