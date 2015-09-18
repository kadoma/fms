/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	test.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-04
 * Description:	
 *				only for test cpumem event agent.
 * 				make -f Makefile.test (clean) 
 *
 */

#ifdef TEST_CMEA

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>

#include "evt_process.h"
#include "cpumem.h"
#include "logging.h"

char *processor_flags;
enum cputype cputype;
double cpumhz;

char *program;

void
usage(void)
{
	printf("usage:\n");
	printf("    %s cache [cpu]{0,1,2...} {type}[0,1,2] {level}[0,1,2,3]\n", program);
	printf("    %s mempage {addr}[0xabcdef]\n", program);
	exit(0);
}

int
main(int argc, char *argv[])
{
	wr_log_init("./evtagent.log");
	wr_log_set_loglevel(WR_LOG_DEBUG);

	program = argv[0];
	
	if (argc > 1) {
		if (!strcmp(argv[1], "cache")) {
			int cpu, type, level;
			
			if (argc < 5) usage();
			
			cpu = atoi(argv[2]);
			type = atoi(argv[3]);
			level = atoi(argv[4]);

			if (test_cpu_offine_on_cache(cpu, level, type)) {
				printf("cpu %d cache level %d type %d, failed to offline\n",
					cpu, level, type);
				exit(0);
			}
			printf("cpu %d cache level %d type %d, success to offline\n",
				cpu, level, type);
			exit(0);
		} else if (!strcmp(argv[1], "mempage")) {
			unsigned long long addr;
			char *end;
			
			if (argc < 3) usage();

			addr = strtoull(argv[2], &end, 16);

			if (test_memory_page_offline(addr)) {
				printf("memory %llx, failed to offline, %s\n", 
					addr, strerror(errno));
				exit(0);
			}
			printf("memory %llx, success to offline\n",
				addr);
			exit(0);
		}
	}
	usage();
	
	exit(0);
}

#endif /* TEST_CMEA */
