
#include "wrap.h"
#include "fmd_case.h"
#include "fmd_event.h"
#include "fmd_nvpair.h"
#include "list.h"
#include "fmd_api.h"
#include "fmd.h"

/*
void
fmd_create_fault(fmd_event_t *pevt)
{
	char *p = NULL;
	char eclass[128] = {0};

	strcpy(eclass, pevt->ev_class);
	p = strchr(eclass, '.');
    sprintf(pevt->ev_class, "fault%s", p);
	return;
}
*/

#define REPAIRED_N   10
#define REPAIRED_T   120
fmd_event_t *
fmd_create_ereport(fmd_t *pfmd, const char *eclass, char *id, nvlist_t *p_nvl)
{
    fmd_event_t *pevt = NULL;
    pevt = (fmd_event_t *)def_calloc(sizeof(fmd_event_t), 1);

	pevt->dev_name = strdup(p_nvl->name);
	pevt->dev_id = p_nvl->dev_id;
	pevt->evt_id = p_nvl->evt_id;
	
    pevt->ev_create = time(NULL);
    pevt->ev_class = strdup(eclass);
    pevt->data = p_nvl->data;


	/* added for test by guanhj : todo every src module */
	pevt->repaired_N =REPAIRED_N;
	pevt->repaired_T = REPAIRED_T; // 2 min

    list_del(&p_nvl->nvlist);
    def_free(p_nvl);

    //wanghuan 
    INIT_LIST_HEAD(&pevt->ev_list);
    return pevt;
}

/**
 * fmd_create_list
 *
 * @param
 * @return
 */
 
fmd_event_t *
fmd_create_listevent(fmd_event_t *pevt, int action)
{
    char eclass[128] = {0};
    char *p;
    fmd_event_t *listevt = def_calloc(sizeof(fmd_event_t), 1);

    p = strchr(pevt->ev_class, '.');
    sprintf(eclass, "list%s", p);

    listevt->dev_name = strdup(pevt->dev_name);
    listevt->ev_create = time(NULL);
    listevt->ev_class = strdup(eclass);
    listevt->agent_result = action;
	listevt->dev_id = pevt->dev_id;
	listevt->evt_id = pevt->evt_id;
	listevt->repaired_N = pevt->repaired_N;
	listevt->repaired_T = pevt->repaired_T;
	listevt->ev_refs = pevt->ev_refs;

    return listevt;
}
