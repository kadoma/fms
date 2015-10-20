
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <syslog.h>
#include <linux/limits.h>

#include "wrap.h"
#include "list.h"
#include "fmd_case.h"
#include "fmd_event.h"
#include "fmd_esc.h"
#include "fmd_hash.h"
#include "fmd.h"
#include "logging.h"

#define PATH_LEN 256

fmd_esc_t *pesc = NULL;

void
esc_build_event(char *eclass)
{
	fmd_event_type_t *etype = NULL;
	etype = (struct fmd_event_type *)def_calloc(sizeof(struct fmd_event_type), 1);
	strncpy(etype->eclass, eclass, strlen(eclass));
	list_add(&etype->list, &pesc->list_event);
}

void
esc_build_engine(char *eclass)
{
	fmd_serd_type_t *serdtype = NULL;
	serdtype = (fmd_serd_type_t *)def_calloc(sizeof(fmd_serd_type_t), 1);

	serdtype->N = atoi(strchr(eclass, '=') + 1);
	serdtype->T = atoi(strrchr(eclass, '=') + 1) * 60 * 60;

	char *p = strstr(eclass, "{");
	// -1
	strncpy(serdtype->eclass, eclass, p - eclass -1);
	
	list_add(&serdtype->list, &pesc->list_serd);
}

void
esc_build_fault(char *eclass)
{
	fmd_fault_type_t *etype = NULL;
	etype = (fmd_fault_type_t *)def_calloc(sizeof(fmd_fault_type_t), 1);
	strncpy(etype->eclass, eclass, strlen(eclass));
	
	list_add(&etype->list, &pesc->list_fault);
}

static void
esc_parsefile(const char *fname)
{
	if(fname == NULL)
	{
		wr_log("", WR_LOG_ERROR, "esc file name is null.");
		return;
	}
	
	FILE *fp = NULL;
	char buf[128] = {0};

	if((fp = fopen(fname, "r")) == NULL){
		wr_log("FMD",WR_LOG_ERROR, "failed to open file %s\n", fname);
		return;
	}else
		wr_log("parsefile",WR_LOG_NORMAL, "Parsing file: %s\n", fname);

	while(fgets(buf, sizeof(buf), fp))
	{
		buf[strlen(buf) - 1] = 0;
		if (strncmp(buf, "event ", 6) == 0){
			esc_build_event(&buf[6]);
			continue;
		}

		if (strncmp(buf, "engine ", 7) == 0){
			char *engine = (char *)def_calloc(64, 1);
			strcpy(engine, &buf[7]);
			esc_build_engine(engine);
			continue;
		}

		if (strncmp(buf, "fault ", 6) == 0){
			char *eclass = (char *)def_calloc(64, 1);
			strcpy(eclass, &buf[6]);
			esc_build_fault(eclass);
			continue;
		}
		
	}

	fclose(fp);
}

/**
 * esc_parsedir
 *
 * @param
 * @return
 */
static void
esc_parsedir(const char *dir, const char *suffix)
{
	char path[PATH_MAX];
	struct dirent *dp;
	const char *p;
	DIR *dirp;

	if ((dirp = opendir(dir)) == NULL)
		return; /* failed to open directory; just skip it */

	while ((dp = readdir(dirp)) != NULL)
	{
		if (dp->d_name[0] == '.')
			continue; /* skip "." and ".." */

		p = strrchr(dp->d_name, '.');

		if (suffix != NULL && (p == NULL || strcmp(p, suffix) != 0))
			continue; /* skip files with the wrong suffix */

		if (p != NULL && strcmp(p, ".esc") == 0)
		{
			(void) snprintf(path, sizeof (path), "%s/%s", dir, dp->d_name);
			(void) esc_parsefile(path);
		}
	}

	(void) closedir(dirp);
}

void
_stat_esc(void)
{
	struct list_head *pos;
	struct fmd_serd_type *pserd = NULL;
	struct fmd_fault_type *pfault = NULL;
	struct fmd_event_type *pevent = NULL;

	wr_log("", WR_LOG_DEBUG, "print esc file serd .\n");
	list_for_each(pos, &pesc->list_serd) {
		pserd = list_entry(pos, struct fmd_serd_type, list);
		wr_log("", WR_LOG_DEBUG ,"serd eclass is %s", pserd->eclass);
	}

	wr_log("", WR_LOG_DEBUG, "print esc file fault. \n");
	list_for_each(pos, &pesc->list_fault) {
		pfault = list_entry(pos, struct fmd_fault_type, list);
		wr_log("", WR_LOG_DEBUG, "fault eclass is %s", pfault->eclass);
	}

	wr_log("", WR_LOG_DEBUG, "print esc file event. \n");
	list_for_each(pos, &pesc->list_event) {
		pevent = list_entry(pos, struct fmd_event_type, list);
		wr_log("", WR_LOG_DEBUG, "event eclass is %s", pevent->eclass);
	}
	
}

int
esc_main(fmd_t *fmd)
{
	char path[PATH_LEN] = {0};

	pesc = &fmd->fmd_esc;

	/* init hashtable & list */
	INIT_HASH_HEAD(&pesc->hash_clsname);
	
	/* to record the size of hashtable */
	pesc->hash_clsname.value = (uint64_t)0;
	INIT_LIST_HEAD(&pesc->list_event);
	INIT_LIST_HEAD(&pesc->list_fault);
	INIT_LIST_HEAD(&pesc->list_serd);

	sprintf(path, "%s/%s", BASE_DIR, "escdir");
	esc_parsedir(path, ".esc");

	return 0;
}
