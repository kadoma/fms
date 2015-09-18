/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	test.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-04
 * Description:	
 *				only for test cpumem event source.
 * 				make -f Makefile.test (clean) 
 *
 */

#ifdef TEST_CMES

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>

#include "cpumem_evtsrc.h"
#include "cpumem.h"
#include "list.h"
#include "fmd_nvpair.h"

int g_system_exit;

static void
system_exit(int sig)
{
	g_system_exit = 1;
}

static void
cmes_sig()
{
	struct sigaction sa = {.sa_handler = system_exit};

	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
}

static void
dump_data(void *data)
{
	struct fms_cpumem *fc = (struct fms_cpumem *)data;
	
	if(!data) {
		return ;
	}

	printf("SOCKETID %u CPU %u\n", fc->socketid, fc->cpu);
	printf("STATUS %llX\n", (unsigned long long int)fc->status);
	printf("MISC %llX\n", (unsigned long long int)fc->misc);
	printf("ADDR %llX\n", (unsigned long long int)fc->addr);
	printf("MCGSTATUS %llX\n", (unsigned long long int)fc->mcgstatus);
	printf("IP %llX\n", (unsigned long long int)fc->ip);
	printf("TSC %llX\n", (unsigned long long int)fc->tsc);
	printf("TIME %llX\n", (unsigned long long int)fc->time);
	printf("APCICID %llX\n", (unsigned long long int)fc->apicid);
	printf("CPUID %llX\n", (unsigned long long int)fc->cpuid);
	printf("MCGCAP %llX\n", (unsigned long long int)fc->mcgcap);
	printf("FLAGS %llX\n", (unsigned long long int)fc->flags);
	printf("CS %u\n", fc->cs);
	printf("BANK %u\n", fc->bank);
	printf("CPUVENDOR %u\n", fc->cpuvendor);
	printf("MEM_CH %d\n", fc->mem_ch);
	printf("SOCKETID %u CPU %u\n", fc->socketid, fc->cpu);
	printf("DESC %s\n", fc->desc);
}

int
main(int argc, char *argv[])
{
	struct list_head *head, *pos;
	nvlist_t *nvl;
	
	wr_log_init("./evtsrc.log");

	wr_log_set_loglevel(WR_LOG_DEBUG);
	if (argc > 1) {
		if(!strcmp(argv[1], "debug"))
			wr_log_set_loglevel(WR_LOG_DEBUG);
	}
	
	/* init */
	printf("cpumem event source init\n");
	if (cmes_init_test()) {
		printf("failed to init cmes, Please show fms system log\n");
		exit(0);
	}

	/* sig */
	cmes_sig();
	
	/* test cmes */
	printf("cpumem event source test start...\n");
	while (!g_system_exit) {
		head = cmes_test();

		if(list_empty(head)) {
			printf("no event\n");
			continue;
		}

		list_for_each(pos, head) {
			nvl = list_entry(pos, nvlist_t, nvlist);
			printf("%s\n", nvl->value);
			//dump_data(NULL);
		}
		
		//fms_cpumem_data_release(); /* cpumem.c */
		
		sleep(1);
	}

	/* cmes release */
	printf("cpumem event source release\n");
	cmes_release_test();

	exit(0);
}

#endif
