/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	kfmadm.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-17
 * Description:	
 *				Kernel Fault Mamagement ADMinistration program.
 *
 */

#ifdef KFM_TEST
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <linux/kernel-page-flags.h>

#include "libkfm_cpumem.h"
#include "kfm_nl.h"

static int list_cache_info(struct kfm_cache_u *cache);
static int list_mempage_info(struct kfm_mempage_u *mempage);

static int kfm_get_version(void);

static const char *program;
char version[64];
extern struct nla_policy kfm_info_nla_policy[KFM_INFO_ATTR_MAX + 1];

#define CMD_NONE				0x0000U
#define CMD_DCACHE				0x0001U
#define CMD_DACACHE				0x0002U
#define CMD_ECACHE				0x0004U
#define CMD_EACACHE				0x0008U
#define CMD_FCACHE				0x0010U
#define CMD_FACACHE				0x0020U
#define CMD_LIST				0x0040U
#define CMD_MEMPAGE_ONLINE		0x0080U
#define NUMBER_OF_CMD			8

static const char *cmdnames[] = {
	"disable-cache",
	"disable-all-cache",
	"enable-cache",
	"enable-all-cache",
	"flush-cache",
	"flush-all-cache",
	"list",
	"mempage-online",
};

#define OPT_NONE		0x00000
#define OPT_CPU			0x00001
#define OPT_CACHE		0x00002
#define OPT_MEMPAGE		0x00004
#define NUMBER_OF_OPT	3

static const char *optnames[] = {
	"cpu",
	"cache",
	"mempage",
};

/*
 * Table of legal combinations of commands and options.
 * Key:
 *  '+'  compulsory
 *  'x'  illegal
 *  '1'  exclusive (only one '1' option can be supplied)
 *  ' '  optional
 */
static const char commands_v_options[NUMBER_OF_CMD][NUMBER_OF_OPT] = {
/*               -u  -c  -m  */
/*DCACHE*/      {'+', 'x', 'x'},
/*DACACHE*/     {'x', 'x', 'x'},
/*ECACHE*/      {'+', 'x', 'x'},
/*EACACHE*/     {'x', 'x', 'x'},
/*FCACHE*/      {'+', 'x', 'x'},
/*FACACHE*/  	{'x', 'x', 'x'},
/*LIST*/ 		{' ', ' ', ' '},
/*MP-ONLINE*/   {'x', 'x', '+'},
};

static struct option long_options[] = {
	{"dcache", 1, 0, 'd'},
	{"dacache", 0, 0, 'D'},
	{"ecache", 1, 0, 'e'},
	{"eacache", 0, 0, 'E'},
	{"fcache", 1, 0, 'f'},
	{"facache", 0, 0, 'F'},
	{"list", 0, 0, 'L'},
	{"cpu", 1, 0, 'u'},
	{"cache", 2, 0, 'c'},
	{"help", 0, 0, 'h'},
	{"mempage", 1, 0, 'm'},
	{"mempage-online", 0, 0, 'o'},
	{0, 0, 0, 0}
};


static int
modprobe_kfm(void)
{
	char *argv[] =
	    { "/sbin/modprobe",  "--", "kfm", NULL };
	int child;
	int status;

	if (!(child = fork())) {
		execv(argv[0], argv);
		exit(1);
	}

	waitpid(child, &status, 0);

	if (!WIFEXITED(status) || WEXITSTATUS(status)) {
		return 1;
	}

	return 0;
}

static void
fail(int err, char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
	fprintf(stderr, "\n");
	exit(err);
}

static void
generic_opt_check(int command, int options)
{
	int i, j;
	int last = 0, count = 0;

	/* Check that commands are valid with options. */
	for (i = 0; i < NUMBER_OF_CMD; i++) {
		if (command & (1 << i))
			break;
	}

	for (j = 0; j < NUMBER_OF_OPT; j++) {
		if (!(options & (1 << j))) {
			if (commands_v_options[i][j] == '+')
				fail(2, "You need to supply the '%s' "
				     "option for the '%s' command",
				     optnames[j], cmdnames[i]);
		} else {
			if (commands_v_options[i][j] == 'x')
				fail(2, "Illegal '%s' option with "
				     "the '%s' command",
				     optnames[j], cmdnames[i]);
			if (commands_v_options[i][j] == '1') {
				count++;
				if (count == 1) {
					last = j;
					continue;
				}
				fail(2,
				     "The option '%s' conflicts with the "
				     "'%s' option in the '%s' command",
				     optnames[j], optnames[last],
				     cmdnames[i]);
			}
		}
	}
}

static inline const char *
opt2name(int option)
{
	const char **ptr;
	for (ptr = optnames; option > 1; option >>= 1, ptr++);

	return *ptr;
}

static void
set_command(unsigned int *cmd, unsigned int newcmd)
{
	if (*cmd != CMD_NONE)
		fail(2, "multiple commands specified");
	*cmd = newcmd;
}

static void
set_option(unsigned int *options, unsigned int option)
{
	if (*options & option)
		fail(2, "multiple '%s' options specified",
		     opt2name(option));
	*options |= option;
}

static void
tryhelp_exit(const int status)
{
	fprintf(stderr,
		"Try `%s -h' or '%s --help' for more information.\n",
		program, program);
	exit(status);
}

static void
usage_exit(const int status)
{
	FILE *stream;

	if (status != 0)
		stream = stderr;
	else
		stream = stdout;

	fprintf(stream,
		"%s %s\n"
		"Usage:\n"
		"  %s -d -u cpu\n"
		"  %s -D\n"
		"  %s -e -u cpu\n"
		"  %s -E\n"
		"  %s -f -u cpu\n"
		"  %s -F\n"
		"  %s -o -m addr\n"
		"  %s -L [-c [cpu]] [-m addr]\n"
		"  %s -h\n\n",
		program, version, program, program, program, program
		, program, program, program, program, program);

	fprintf(stream,
		"Commands:\n"
		"Either long or short options are allowed.\n"
		"  --dcache              -d        disable the cache with options\n"
		"  --dacache             -D        disable all caches\n"
		"  --ecache              -e        enable the cache with options\n"
		"  --eacache             -E        enable all caches\n"
		"  --fcache              -f        flush the cache with options\n"
		"  --facache             -F        flush all caches\n"
		"  --mempage-online      -o        memory page online\n"
		"  --list                -L        show info with options\n"
		"  --help                -h        display this help message\n\n");

	fprintf(stream,
		"Options:\n"
		"  --cpu         -u number     cpu number\n"
		"  --cache       -c [number]   list a cache info or all\n"
		"  --mempage     -m addr       list a mempage info\n");

	exit(status);
}

int
main(int argc, char **argv)
{
	const char *optstring = "dDeEfFLu:c::hm:o";
	int c, result;
	unsigned int command = CMD_NONE;
	unsigned int options = OPT_NONE;
	struct kfm_cache_u cache;
	struct kfm_mempage_u mempage;
	char *endptr;

	memset(&cache, 0, sizeof cache);
	memset(&mempage, 0, sizeof mempage);

	program = argv[0];

	if(kfm_get_version()) {
		/* try to insmod the kfm module if kfm_get_version failed */
		if (modprobe_kfm() || kfm_get_version())
			fail(2, "%s", strerror(errno));
	}
	
	/*
	 * 	Parse options
	 */
	if ((c = getopt_long(argc, argv, optstring,
			     long_options, NULL)) == EOF)
		tryhelp_exit(-1);

	switch (c) {
	case 'd':
		set_command(&command, CMD_DCACHE);
		break;
	case 'D':
		set_command(&command, CMD_DACACHE);
		break;
	case 'e':
		set_command(&command, CMD_ECACHE);
		break;
	case 'E':
		set_command(&command, CMD_EACACHE);
		break;
	case 'f':
		set_command(&command, CMD_FCACHE);
		break;
	case 'F':
		set_command(&command, CMD_FACACHE);
		break;
	case 'L':
		set_command(&command, CMD_LIST);
		break;
	case 'o':
		set_command(&command, CMD_MEMPAGE_ONLINE);
		break;
	case 'h':
		usage_exit(0);
		break;
	default:
		tryhelp_exit(-1);
	}

	while ((c = getopt_long(argc, argv, optstring,
				long_options, NULL)) != EOF) {
		switch (c) {
		case 'u':
			set_option(&options, OPT_CPU);
			cache.cpu = atoi(optarg);
			break;
		case 'c':
			set_option(&options, OPT_CACHE);
			if(optarg)
				cache.cpu = atoi(optarg);
			else
				cache.cpu = -1;
			break;
		case 'm':
			set_option(&options, OPT_MEMPAGE);
			mempage.addr = strtoull(optarg, &endptr, 0);
			break;
		default:
			fail(2, "invalid option");
		}
	}

	if (optind < argc)
		fail(2, "unknown arguments found in command line");

	generic_opt_check(command, options);

	switch (command) {
	case CMD_DCACHE:
		result = kfm_cache_disable(&cache);
		break;
	case CMD_DACACHE:
		result = kfm_cache_disable_all();
		break;
	case CMD_ECACHE:
		result = kfm_cache_enable(&cache);
		break;
	case CMD_EACACHE:
		result = kfm_cache_enable_all();
		break;
	case CMD_FCACHE:
		result = kfm_cache_flush(&cache);
		break;
	case CMD_FACACHE:
		result = kfm_cache_flush_all();
		break;
	case CMD_MEMPAGE_ONLINE:
		result = kfm_mempage_online(&mempage);
		break;
	case CMD_LIST:
		if (options & OPT_CACHE)
			result = list_cache_info(&cache);
		else if(options & OPT_MEMPAGE)
			result = list_mempage_info(&mempage);
		break;
	}

	if (result)
		fprintf(stderr, "%s\n", strerror(errno));

	return 0;
}

static void
printf_cache_info(struct kfm_cache_u *cache)
{
	printf("%d      %s\n",
		cache->cpu, cache->status ? "disable" : "enable");
}

static int
list_cache_info(struct kfm_cache_u *cache)
{
	int ret; 
	struct kfm_cache_u c;

	printf("show cpu cache status\n");
	printf("CPU    CSTATUS\n");
	
	if (cache->cpu == -1) {  /* get all cache info */
		FILE *f = NULL;
		char *line = NULL;
		size_t linesz = 0;
		
		f = fopen("/proc/cpuinfo", "r");
		if (!f)
			fail(2, "failed to open /proc/cpuinfo, %s", strerror(errno));
		
		while (getdelim(&line, &linesz, '\n', f) > 0) {
			unsigned cpu;
			
			if (sscanf(line, "processor : %u\n", &cpu) == 1) {
				memset(&c, 0, sizeof c);
				c.cpu = cpu;
				ret = kfm_cache_get_info(&c);
				if(ret) {
					fail(2, "failed to get info, %s\n", strerror(errno));
				}

				printf_cache_info(&c);
			}
		}

		if(line)
			free(line);
		fclose(f);
	} else {
		ret = kfm_cache_get_info(cache);
		if(ret) {
			fail(2, "failed to get info, %s\n", strerror(errno));
		}

		printf_cache_info(cache);
	}

	return 0;
}

static int
list_mempage_info(struct kfm_mempage_u *mempage)
{
	int ret; 
	char status[64];
	uint64_t flags;
	
	ret = kfm_mempage_get_info(mempage);
	if(ret) {
		fail(2, "failed to get mempage info, %s\n", strerror(errno));
	}

	flags = mempage->flags;
	if	(!(flags >> KPF_NOPAGE & 1)) {
		if(flags >> KPF_HWPOISON & 1)
			strcpy(status, "offline");
		else
			strcpy(status, "online");
	} else {
		strcpy(status, "nopage");
	}

	printf("      ID                   ADDR                  FLAGS            STATUS\n");
	printf("0x%013llx     0x%016llx     0x%016llx     %s\n", 
			mempage->pfn,
			mempage->addr,
			mempage->flags,
			status);

	return 0;
}

static int 
kfm_getinfo_parse_cb(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *nlh = nlmsg_hdr(msg);
	struct nlattr *attrs[KFM_INFO_ATTR_MAX + 1];
	uint32_t ver;
	
	if (genlmsg_parse(nlh, 0, attrs, KFM_INFO_ATTR_MAX, kfm_info_nla_policy) != 0)
		return -1;

	if (!attrs[KFM_INFO_ATTR_VERSION])
		return -1;

	ver = nla_get_u32(attrs[KFM_INFO_ATTR_VERSION]);
	sprintf(version, "KFM version %d.%d.%d", NVERSION(ver));
	
	return NL_OK;
}

int
kfm_get_version(void)
{
	struct kfm_nl fm_nl;
	
	fm_nl.cmd = KFM_CMD_GET_INFO;
	fm_nl.flags = 0;
	fm_nl.fill_attr = NULL;
	fm_nl.fill_attr_data = NULL;
	fm_nl.recvmsg_cb = kfm_getinfo_parse_cb;
	fm_nl.recvmsg_cb_data = NULL;

	return kfm_nl_process_message(&fm_nl);
}

#endif /* KFM_TEST */
