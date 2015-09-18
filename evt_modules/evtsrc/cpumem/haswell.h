#ifndef __HASWELL_H__
#define __HASWELL_H__

void hsw_decode_model(int cputype, int bank, u64 status, u64 misc, 
			struct mc_msg *mm, int *mm_num);

#endif
