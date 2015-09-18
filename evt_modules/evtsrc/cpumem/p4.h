#ifndef __CPUMEM_P4__
#define __CPUMEM_P4__

void decode_intel_mc(struct mce *log, int cpu, unsigned len, 
		struct mc_msg *mm, int *mm_num);

#endif
