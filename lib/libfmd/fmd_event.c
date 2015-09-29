
#include "wrap.h"
#include "fmd_case.h"
#include "fmd_event.h"
#include "fmd_nvpair.h"
#include "list.h"
#include "fmd_api.h"
#include "fmd.h"

fmd_event_t *
fmd_create_ereport(fmd_t *pfmd, const char *eclass, char *id, nvlist_t *p_nvl)
{
    fmd_event_t *pevt = NULL;
    pevt = (fmd_event_t *)def_calloc(sizeof(fmd_event_t), 1);
    pevt->ev_create = time(NULL);
    pevt->ev_refs = 0;
    pevt->ev_class = strdup(eclass);
	pevt->dev_name = strdup(p_nvl->name);
	pevt->ev_err_id = p_nvl->dev_id;

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
fmd_create_listevent(fmd_event_t *fault, int action)
{
    fmd_event_t *pevt = NULL;
    char eclass[128] = {0};
	char *p;

    pevt = (fmd_event_t *)def_calloc(sizeof(fmd_event_t), 1);
    sprintf(eclass, "list.%s", &fault->ev_class[6]);
	
	pevt->dev_name = fault->dev_name;
	pevt->ev_err_id = fault->ev_err_id;
    pevt->ev_create = time(NULL);
    pevt->ev_refs = 0;
    pevt->ev_class = strdup(eclass);
	pevt->ev_flag = action;
	
    INIT_LIST_HEAD(&pevt->ev_list);
    return pevt;
}

fmd_event_t *
fmd_create_casefault(fmd_t *p_fmd, fmd_case_t *pcase)
{
	fmd_event_t *pevt = NULL;
	pevt = (fmd_event_t *)def_calloc(sizeof(fmd_event_t),1);
	pevt->ev_create = time(NULL);
	// bug lost it.
	pevt->ev_refs = pcase->cs_fire_times;
	//pevt->ev_data = NULL;
	INIT_LIST_HEAD(&pevt->ev_list);
	return pevt;
}


fmd_event_t *
fmd_create_eventfault(fmd_t *pfmd, fmd_case_t *pcase)
{
	fmd_event_t *pevt = NULL;

	const char *eclass = pcase->dev_name;

	pevt = (fmd_event_t *)def_calloc(sizeof(fmd_event_t), 1);

	pevt->ev_create = time(NULL);
	pevt->ev_refs = 0;
	pevt->ev_class = strdup(eclass);
	INIT_LIST_HEAD(&pevt->ev_list);

	return pevt;
}
