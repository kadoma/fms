/*
 * esc.c
 *
 *  Created on: Feb 10, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	ESC Main
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <syslog.h>

#include <linux/limits.h>

#include <fmd.h>
#include <fmd_esc.h>
#include <fmd_string.h>
#include <fmd_list.h>
#include <fmd_event.h>
#include <fmd_serd.h>
#include <fmd_case.h>

fmd_esc_t *pesc;

/**
 * declbody
 *
 * @param
 * @return
 */
struct esc_node *
esc_declbody(char *fullclass, char *nvlist)
{
	struct esc_node *np = (struct esc_node *)
			malloc(sizeof(struct esc_node));
	assert(np != NULL);
	memset(np, 0, sizeof(struct esc_node));

	np->fullclass = fullclass;
	np->nvlist = nvlist;

	return np;
}


/**
 * esc_build_event
 *
 * @param
 * @return
 *
 * @todo check for redefinition
 */
void
esc_build_event(struct esc_node *np)
{
	char *evtclass = np->fullclass;
	struct fmd_event_type *etype;
	char **tokens, *token;
	char *key;
	uint64_t value;
	int size = 0, ite = 0;

	etype = (struct fmd_event_type *)
			malloc(sizeof(struct fmd_event_type));
	assert(etype != NULL);
	memset(etype, 0, sizeof(struct fmd_event_type));

	/* put to symbol hashtable */
	char *sdup = strdup(evtclass);
	//hash_put(&hash_clsname, sdup, (void *)etype);

	/* fill this struct */
	tokens = split(evtclass, '.', &size);
	etype->fmd_evt_levels = size;
	for(; ite < size; ite++) {
		token = tokens[ite];
		value = hash_get(&pesc->hash_clsname, token);
		switch(ite) {
		case 0:
			etype->fmd_evt_class0 = (uint32_t)value;
			break;
		case 1:
			etype->fmd_evt_class1 = (uint32_t)value;
			break;
		case 2:
			etype->fmd_evt_class2 = (uint32_t)value;
			break;
		case 3:
			etype->fmd_evt_class3 = (uint32_t)value;
			break;
		case 4:
			etype->fmd_evt_class4 = (uint32_t)value;
			break;
		case 5:
			etype->fmd_evt_class5 = (uint32_t)value;
			break;
		}
	}

	/* add to global list */
	list_add(&etype->list, &pesc->list_evtype);

	hash_put(&pesc->hash_clsname, sdup, etype->fmd_evtid);
}


/**
 * esc_build_engine
 *
 * @param
 * @return
 *
 * @todo check for redefinition
 */
void
esc_build_engine(struct esc_node *np)
{
	char *serdclass = np->fullclass, *nvlist = np->nvlist;
	struct fmd_serd_type *serdtype;
	char **tokens, *token;
	uint64_t value;
	uint32_t tim;
	int size = 0, ite = 0;

	serdtype = (struct fmd_serd_type *)
			malloc(sizeof(struct fmd_serd_type));
	assert(serdtype != NULL);
	memset(serdtype, 0, sizeof(struct fmd_serd_type));

	/* put to symbol hashtable */
	char *sdup = strdup(serdclass);
	//hash_put(&hash_clsname, sdup, (void *)serdtype);

	/* fill this struct */
	tokens = split(serdclass, '.', &size);
	serdtype->fmd_serd_levels = size;
	for(; ite < size; ite++) {
		token = tokens[ite];
		value = hash_get(&pesc->hash_clsname, token);
		switch(ite) {
		case 0:
			serdtype->fmd_serd_class0 = (uint32_t)value;
			break;
		case 1:
			serdtype->fmd_serd_class1 = (uint32_t)value;
			break;
		case 2:
			serdtype->fmd_serd_class2 = (uint32_t)value;
			break;
		case 3:
			serdtype->fmd_serd_class3 = (uint32_t)value;
			break;
		case 4:
			serdtype->fmd_serd_class4 = (uint32_t)value;
			break;
		case 5:
			serdtype->fmd_serd_class5 = (uint32_t)value;
			break;
		}
	}

	/* N=? T=? */
	tokens = split(nvlist, ',', &size);
	/* N */
	token = tokens[0];
	token = trim(token);
	serdtype->N = atoi(strchr(token, '=') + 1);
	/* T */
	token = tokens[1];
	token = trim(token);
	tim = atoi(strchr(token, '=') + 1);
	if (strstr(token, "sec") != NULL)
		serdtype->T = tim;
	else if (strstr(token, "min") != NULL)
		serdtype->T = tim * 60;
	else if (strstr(token, "hour") != NULL)
		serdtype->T = tim * 60 * 60;
	else if (strstr(token, "day") != NULL)
		serdtype->T = tim * 60 * 60 * 24;

	/* add to global list */
	list_add(&serdtype->list, &pesc->list_serdtype);

	hash_put(&pesc->hash_clsname, sdup, serdtype->fmd_serdid);
}


/**
 * esc_build_rend
 *
 * @param
 * @return
 */
void
esc_build_rend(struct rend_node *rp)
{
	char *evtlist = rp->evtlist, *serd = rp->serd,
			*fault = rp->fault;
	struct fmd_case_type *pcase = NULL;

	pcase = (struct fmd_case_type *)malloc
			(sizeof(struct fmd_case_type));
	assert(pcase != NULL);
	memset(pcase, 0, sizeof(struct fmd_case_type));

	char **tokens, *token;
	int size = 0, ite = 0;
	tokens = split(evtlist, ',', &size);
	pcase->size = size;
	for(; ite < size; ite++) {
		token = tokens[ite];
		pcase->ereport[ite] = (uint64_t)hash_get(&pesc->hash_clsname, token);
	}
	pcase->serd = (uint64_t)hash_get(&pesc->hash_clsname, serd);
	pcase->fault = (uint64_t)hash_get(&pesc->hash_clsname, fault);

	list_add(&pcase->list, &pesc->list_casetype);
}


/**
 * esc_rendbody
 *
 * @param
 * @return
 *
 */
struct rend_node *
esc_rendbody(char *evtlist, char *serd, char *fault)
{
	struct rend_node *rp = (struct rend_node *)
			malloc(sizeof(struct rend_node));
	assert(rp != NULL);
	memset(rp, 0, sizeof(struct rend_node));

	rp->evtlist = evtlist;
	rp->serd = serd;
	rp->fault = fault;

	return rp;
}


/**
 * esc_parse_event
 *
 * @param
 * @return
 */
static struct esc_node *
esc_parse_event(char *fullclass)
{
	char *token = NULL;
	char *eclass = (char *)malloc(strlen(fullclass) + 1);
	assert(eclass != NULL);
	memset(eclass, 0, strlen(fullclass) + 1);

	strcpy(eclass, fullclass);

	while ((token = strsep(&fullclass, ".")) != NULL) {
		if (hash_get(&pesc->hash_clsname, token) == 0)	/* key not in globe hash table */
			hash_put(&pesc->hash_clsname, token, (uint64_t)token);
	}

	return esc_declbody(eclass, NULL);
}


/**
 * esc_parse_engine
 *
 * @param
 * @return
 */
static struct esc_node *
esc_parse_engine(char *engine)
{
	char *token = NULL;
	char *p = NULL;
	int len = 0;
	
	char *serd = (char *)malloc(64);
	char *nvlist = (char *)malloc(32);
	char *eclass = (char *)malloc(64);

	assert(serd != NULL);
	memset(serd, 0, 64);
	assert(nvlist != NULL);
	memset(nvlist, 0, 32);
	assert(eclass != NULL);
	memset(eclass, 0, 64);

	p = strchr(engine, ' ');
	strcpy(nvlist, p + 1);

	len = strlen(engine) - strlen(p);
	strncpy(serd, engine, len);
	strncpy(eclass, engine, len);

	while ((token = strsep(&eclass, ".")) != NULL) {
		if (hash_get(&pesc->hash_clsname, token) == 0)  /* key not in globe hash table */
			hash_put(&pesc->hash_clsname, token, (uint64_t)token);
	}

	free(engine);

	return esc_declbody(serd, p + 1);
}


/**
 * esc_parse_rend
 *
 * @param
 * @return
 */
static struct rend_node *
esc_parse_rend(char *fullrend)
{
	char *sp = NULL, *vp = NULL;
	char *rp = NULL, *fp= NULL;
	char *eclass = (char *)malloc(320);
	char *fault = (char *)malloc(64);
	char *serd = NULL;

	assert(eclass != NULL);
	memset(eclass, 0, 320);
	assert(fault != NULL);
	memset(fault, 0, 64);

	if ((sp = strchr(fullrend, '@')) != NULL) {
		serd = (char *)malloc(32);
		assert(serd != NULL);
		memset(serd, 0, 32);

		int len = strlen(fullrend) - strlen(sp);
		strncpy(eclass, fullrend, len);
		eclass = trim(eclass);

		vp = strchr(sp, ' ');
		strncpy(serd, sp + 1, strlen(sp) - strlen(vp));
		serd = trim(serd);
	} else {
		rp = strstr(fullrend, "rend ");
		strncpy(eclass, fullrend, strlen(fullrend) - strlen(rp));
		eclass = trim(eclass);

		serd = NULL;
	}

	fp = strstr(fullrend, "fault.");
	strcpy(fault, fp);

	free(fullrend);

	return esc_rendbody(eclass, serd, fault);
}


/**
 * esc_parsefile
 *
 * @param
 * @return
 */
static void
esc_parsefile(const char *fname)
{
	/* parse esc files */
	FILE *fp = NULL;
	char buf[320];
	memset(buf, 0, 320);

	if ((fp = fopen(fname, "r")) == NULL) {
		printf("FMD: failed to open file %s\n", fname);
		return;
	} else
		syslog(LOG_INFO, "Parsing file: %s\n", fname);

	while (fgets(buf, sizeof(buf), fp)) {
		/* clear '\n' */
		buf[strlen(buf) - 1] = 0;
		/* parse event */
		if (strncmp(buf, "event ", 6) == 0) {
			struct esc_node *np = NULL;
			char *eclass = (char *)malloc(64);
			assert(eclass != NULL);
			memset(eclass, 0, 64);

			strcpy(eclass, &buf[6]);
			np = esc_parse_event(eclass);
			esc_build_event(np);
			continue;
		}
		/* parse engine */
		if (strncmp(buf, "engine ", 7) == 0) {
			struct esc_node *npp = NULL;
			char *engine = (char *)malloc(64);
			assert(engine != NULL);
			memset(engine, 0, 64);

			strcpy(engine, &buf[7]);
			npp = esc_parse_engine(engine);
			esc_build_engine(npp);
			continue;
		}
		/* parse rend */
		if (strncmp(buf, "ereport.", 8) == 0) {
			struct rend_node *rp = NULL;
			char *rend = (char *)malloc(512);
			assert(rend != NULL);
			memset(rend, 0, 512);

			strcpy(rend, buf);
			rp = esc_parse_rend(rend);
			esc_build_rend(rp);
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

	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_name[0] == '.')
			continue; /* skip "." and ".." */

		p = strrchr(dp->d_name, '.');

		if (p != NULL && strcmp(p, ".esc") == 0)
			continue; /* skip .esc files */

		if (suffix != NULL && (p == NULL || strcmp(p, suffix) != 0))
			continue; /* skip files with the wrong suffix */

		(void) snprintf(path, sizeof (path), "%s/%s", dir, dp->d_name);
		(void) esc_parsefile(path);
	}

	(void) closedir(dirp);
}


/**
 * esc_stat
 *
 * @param
 * @return
 * @desc
 * 	print statistics information
 */
void
_stat_esc(void)
{
	struct list_head *pos;
	struct fmd_hash *phash;
	char *key;
	uint64_t value;

	/* hash_clsname */
	list_for_each(pos, &pesc->hash_clsname.list) {
		phash = list_entry(pos, struct fmd_hash, list);
		key = phash->key;
		value = phash->value;
		syslog(LOG_NOTICE, "%s %llx\n", (char *)key, (uint64_t)value);
	}

	/* list_evtype */
	struct fmd_event_type *pevtype;
	uint64_t evtid;
	list_for_each(pos, &pesc->list_evtype) {
		pevtype = list_entry(pos, struct fmd_event_type, list);
		evtid = pevtype->fmd_evtid;
		syslog(LOG_NOTICE, "event type: %llx\n", evtid);
	}

	/* list_serdtype */
	struct fmd_serd_type *pserd;
	uint64_t serdid;
	list_for_each(pos, &pesc->list_serdtype) {
		pserd = list_entry(pos, struct fmd_serd_type, list);
		serdid = pserd->fmd_serdid;
		syslog(LOG_NOTICE, "serd type: %llx\n", serdid);
	}

	/* list_casetype */
	struct fmd_case_type *pcase;
	uint64_t eid, fid, sid;
	int size, ite;
	list_for_each(pos, &pesc->list_casetype) {
		pcase = list_entry(pos, struct fmd_case_type, list);
		size = pcase->size;
		for(ite = 0; ite < size; ite++) {
			syslog(LOG_NOTICE, "%llx ", (uint64_t)pcase->ereport[ite]);
		}

		sid = pcase->serd;
		fid = pcase->fault;

		syslog(LOG_NOTICE, "@ %llx -> %llx\n", sid, fid);
	}
}


/**
 * escmain
 *
 * @param
 * @return
 */
int
esc_main(fmd_t *fmd)
{
	char path[PATH_MAX];

	pesc = &fmd->fmd_esc;

	/* init hashtable & list */
	INIT_HASH_HEAD(&pesc->hash_clsname);
	/* to record the size of hashtable */
	pesc->hash_clsname.value = (uint64_t)0;
	INIT_LIST_HEAD(&pesc->list_evtype);
	INIT_LIST_HEAD(&pesc->list_serdtype);
	INIT_LIST_HEAD(&pesc->list_casetype);

	memset(path, 0, PATH_MAX);
	sprintf(path, "%s/%s", BASE_DIR, "eversholt");
	esc_parsedir(path, ".cpp");

	return 0;
}
