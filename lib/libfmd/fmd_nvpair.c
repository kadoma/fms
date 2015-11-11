
#include <assert.h>
#include "wrap.h"
#include "fmd_nvpair.h"
#include "list.h"

nvlist_t *nvlist_alloc()
{
    nvlist_t *p_nvl = NULL;

    p_nvl = (nvlist_t *)def_calloc(1, sizeof(nvlist_t));
    p_nvl->data = NULL;
    //INIT_LIST_HEAD(&p_nvl->nvlist);

    return p_nvl;
}

/*
 * nvlist_head_alloc - Allocate head of nvlist.
 */
 
struct list_head *
nvlist_head_alloc(void)
{
    struct list_head *head = NULL;

    head = (struct list_head *)def_calloc(1, sizeof(struct list_head));
    INIT_LIST_HEAD(head);

    return head;
}

void
nvlist_free(nvlist_t *nvl)
{
    def_free(nvl);
}

void
nvlist_add_nvlist(struct list_head *head, nvlist_t *nvl)
{
    list_add(&nvl->nvlist, head);
}
