/*
 * proc_evtsrc.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Jun, 17, 2011
 *      Author: Inspur OS Team
 *  
 * Description: 
 *      proc monitor evtsrc module
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <sys/wait.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <fmd.h>
#include <fmd_evtsrc.h>
#include <fmd_module.h>
#include <protocol.h>
#include <nvpair.h>
#include "proc_evtsrc.h"

static struct proc_mon* proc_list_head = NULL;
static struct proc_mon* proc_list_tail = NULL;

static int
proc_fm_event_post(struct list_head *head, 
		nvlist_t *nvl, char *fullclass, uint64_t ena, struct proc_mon *proc_node)
{
	sprintf(nvl->name, FM_CLASS);
	strcpy (nvl->value, fullclass);
	nvl->rscid = ena;
	nvl->data = malloc(sizeof(struct proc_mon));
	memcpy(nvl->data, (void *)proc_node, sizeof(struct proc_mon));
	nvlist_add_nvlist(head, nvl);

	printf("Proc module: post %s\n", fullclass);

	return 0;
}

/* 
 * parse_proc_node
 *  extract single proc info from config file
 *
 * Input
 *  cfg_flie	- config file
 *  proc_node	- 
 *
 * Return value
 *  0		- if success
 *  non-0	- if failed
 */
static int 
parse_proc_node(xmlDocPtr cfg_file, xmlNodePtr proc_node)
{
	struct proc_mon	*new_node;
	xmlChar		*key;

	new_node = (struct proc_mon*)malloc(sizeof(struct proc_mon));
	if (new_node == NULL ) {
		fprintf(stderr, "PRCO: out of memory\n");
		return -1;
	}
	memset(new_node, 0, sizeof(struct proc_mon));

	proc_node = proc_node->xmlChildrenNode;
	while (proc_node != NULL) {
		key = xmlNodeListGetString(cfg_file, proc_node->xmlChildrenNode, 1);
		if ( !xmlStrcmp(proc_node->name, "name") ) {		/* process name */
			strcpy(new_node->process_name, key);
		}else if ( !xmlStrcmp(proc_node->name, "op") ){		/* operation */
			new_node->operation = PROC_OP_RESTART;/*FIXME*/
		}else if ( !xmlStrcmp(proc_node->name, "interval") ){	/* monitor interval */
			new_node->interval = atol(key);
		}else if ( !xmlStrcmp(proc_node->name, "cmd") ){	/* command to run when dead */
			strcpy(new_node->command, key);
		}else if ( !xmlStrcmp(proc_node->name, "args") ){	/* args of command */
			strcpy(new_node->args, key);
		}

		xmlFree(key);
		proc_node = proc_node->next;
	};

	/* insert node to process list */
	if ( proc_list_head == NULL ) {
		proc_list_head = new_node;
	} else {
		proc_list_tail->next = new_node;
	}
	proc_list_tail = new_node;

	return 0;
}

/*
 * config_init
 *  initial the configuration, read from config
 *  file, decide which process to monitor and what
 *  to do when the process dead
 *
 * Input cfg_file
 *  path of cfg_file
 *
 * Output
 *  struct of configuration
 *
 * Return value
 *  0		- if init success
 *  none-0	- if init failed
 */
static int config_init(char *cfg_file)
{
	xmlDocPtr	cfg_doc;
	xmlNodePtr	proc_node;

	/* check file existence */
	if ( access(cfg_file, F_OK) ) {
		fprintf(stderr, "PROC: config file doesn't exist\n");
		return 1;
	}

	/* parse xml document */
	cfg_doc = xmlReadFile(cfg_file, "UTF-8", XML_PARSE_NOBLANKS);
	if ( cfg_doc == NULL ) {
		fprintf(stderr, "PROC: Failed to parse config file\n");
		return 1;
	}

	/* get the root node */
	proc_node = xmlDocGetRootElement(cfg_doc);
	if ( proc_node == NULL ) {
		fprintf(stderr, "PROC: Empty file\n");
		xmlFree(cfg_doc);
		return 1;
	}
	if ( xmlStrcmp(proc_node->name, (const xmlChar*)"proc_list") ) {
		fprintf(stderr, "PROC: Config file format error\n");
		xmlFree(cfg_doc);
		return 1;
	}

	/* parse node */
	proc_node = proc_node->xmlChildrenNode;
	while ( proc_node != NULL ) {
		if ( !xmlStrcmp(proc_node->name, (const xmlChar*)"proc") ) {
			parse_proc_node(cfg_doc, proc_node);
		}else {
			fprintf(stderr, "PROC: Unrecoginzed xml node: %s\n", 
					(char*)proc_node->name);
		}
		proc_node = proc_node->next;
	}

	return 0;
}

/*
 * proc_is_defunct
 *  check if a process is zombie
 *
 * Input
 *  str_pid	- pid of process, in string
 *
 * Return value
 *  0		- process is not a zombie
 *  1		- process is zombie
 *  -1		- error
 */
static int proc_is_defunct(char *str_pid)
{
	FILE	*fd;
	char	ppath[PATH_MAX];		/* path to process status file */
	char	buf[LINE_MAX];
	
	if (str_pid == NULL)
		return 0;

	/* open file which contain process state */
	sprintf(ppath, "/proc/%s/status", str_pid);
	fd = fopen(ppath, "r");
	if (fd == NULL)
		return -1;

	/* Search state. Lookup for a line begin with "State:" */
	while ( fgets(buf, LINE_MAX, fd) != NULL ) {
		if ( strstr(buf, "State:") == buf )	/* "State:" line */
			if ( strstr(buf, "Z") ) {	/* mark as zombie */
				fclose(fd);
				return 1;
			}
			else { 				/* normal process */
				fclose(fd);
				return 0;
			}
	}

	/* Normally, not reach here */
	fclose(fd);
	return -1;
}

/*
 * proc_check
 *  scan the process list, check if every process
 *  is alive. if process died, send an event to 
 *  fms, and tell fms what to do.
 * 
 *  Use 'pidof' command to check if process exist.
 *  If process not exist, pidof will return 1;
 *  If process exist, pidof will return, and we 
 *  store the output of pidof in buffer, which is
 *  a list of all pid of process who have the given
 *  name. Then we check /proc/[PID]/status to find
 *  out if the process is defunct.
 *
 * Input
 *  null
 *
 * Output
 *  fault	- list of event 
 *
 * Return val
 *  0		- if success
 *  non-0	- if failed
 */
static int proc_check(struct list_head* fault)
{
	int	ret;
	char	cmd[LINE_MAX];
	char	buf[LINE_MAX];		/* buffer of cmd output */
	int	len;
	char	*str_pid  = NULL, 	/* pointer to */
		*str_pids = NULL, 	/* pointer to command *pidof* result,
				   	   a list of pid, seperate by blank */
		*saveptr;	/* use for strtok_r */
	pid_t	pid;
	FILE	*fp;
	struct	timeval cur_ts;
	struct 	proc_mon* proc_node;
	char	fullclass[PATH_MAX];
	uint64_t ena;
	nvlist_t *nvl;

	for ( proc_node = proc_list_head; 		/* scan the process list */
			proc_node != NULL; 
			proc_node = proc_node->next ) {

		/* check for time out */
		gettimeofday(&cur_ts, NULL);
		if ( ( (cur_ts.tv_sec - proc_node->last_check.tv_sec)*1000000
			+ (cur_ts.tv_usec - proc_node->last_check.tv_usec) ) 
			< proc_node->interval ) {	
			continue;			/* doesn't need to check */
		}
		proc_node->last_check = cur_ts;		/* update time stamp */

		/* check if process is still alive */
		if ( proc_node->process_name == NULL )	/* pass NULL node */
			continue;
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "pidof -x %s", proc_node->process_name);
		if ( (fp=popen(cmd, "r")) == NULL ) {	/* execute pidof command */
			fprintf(stderr, "PROC: popen failed \n");
			continue;
		}

		/* read the pidof output into buf */
		memset(buf, 0, LINE_MAX);
		if ( fgets(buf, LINE_MAX, fp) == NULL )
			fprintf(stderr, "PROC: fgets get nothing\n");
		else {	/* eliminate LF at the end of string */
			len = strlen(buf);
			if ( buf[len-1] == '\n' )
				buf[len-1] = '\0';
		}
		
		/* ///// ereport.service.proc.stop ////// */
		ret = pclose(fp);
		if ( WEXITSTATUS(ret) ) {		/* no process found */
			memset(fullclass, 0, sizeof(fullclass));
			sprintf(fullclass, 
					"%s.service.proc.%s",
					FM_EREPORT_CLASS, 
					PROC_STAT_STOP);
			ena = 0;/* in fact, we don't need this.. */
			nvl = nvlist_alloc();
			if ( nvl == NULL ) {
				fprintf(stderr, "PROC: unable to alloc nvlist\n");
				return -1;
			}
			if ( proc_fm_event_post(fault, nvl, 
						fullclass, 
						ena,
						proc_node) != 0 ) {
				free(nvl);
				nvl = NULL;
			}
			continue;
		}

		/* ///// ereport.service.proc.zombie ////// */
		for ( str_pids=buf ; ; str_pids=NULL) {
			str_pid = strtok_r(str_pids, " ", &saveptr);
			if ( str_pid == NULL )	/* all pid check */
				break;

			/* zombie check~ */
			switch ( proc_is_defunct(str_pid) ) {
				case 0:/* Process is OK */
				/*	printf("PROC: process %s is alive\n", 
							proc_node->process_name);
				*/
					break;
				case 1:/* Is defunct.. */
					memset(fullclass, 0, sizeof(fullclass));
					sprintf(fullclass, 
							"%s.service.proc.%s",
							FM_EREPORT_CLASS, 
							PROC_STAT_ZOMBIE);
					ena = atoi(str_pid);
					nvl = nvlist_alloc();
					if ( nvl == NULL ) {
						fprintf(stderr, "PROC: unable to alloc nvlist\n");
						return -1;
					}
					if ( proc_fm_event_post(fault, nvl, 
								fullclass, 
								ena,
								proc_node) != 0 ) {
						free(nvl);
						nvl = NULL;
					}

				case -1:
				default:
					fprintf(stderr, "PROC: Defunct detect error\n");
			}
		}/* end of zombie check */
	}/* end of scan proc_list */
	return 0;
}

/*
 * proc_probe
 *  probe the proc event and send to fms
 *
 * Input
 *  emp		event source module pointer
 *
 * Return val
 *  return the nvlist which contain the event.
 *  NULL if failed
 */
struct list_head *
proc_probe(evtsrc_module_t *emp)
{
	int			ret;
	char			*cfg_file;
	struct list_head	*fault;

	fault = nvlist_head_alloc();
	if ( fault == NULL ) {
		fprintf(stderr, "PROC: alloc list_head error\n");
		return NULL;
	}

	cfg_file = strdup(PROC_CONFIG_FILE);
	if ( config_init(cfg_file) ) {
		fprintf(stderr, "PROC: unable to initialize configure\n");
		return NULL;
	}
	
	ret = proc_check(fault);
	if ( ret )
		return NULL;
	else
		return fault;
}

static evtsrc_modops_t proc_mops = {
	.evt_probe = proc_probe,
};

fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	return (fmd_module_t *)evtsrc_init(&proc_mops, path, pfmd);
}

void
fmd_fini(fmd_module_t *mp)
{
}

