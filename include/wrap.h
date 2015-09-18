#ifndef __WRAP_H__
#define __WRAP_H__ 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <assert.h>

#define fmd_debug \
        syslog(LOG_DEBUG, "%s %s %d\n", \
                            __FILE__, __func__, __LINE__);

static void *def_calloc(size_t size, size_t nmemb)
{
    void *p  = calloc(nmemb, size);
    if(p == NULL)
    {
        syslog(LOG_ERR,"#FMS# calloc out of memory");
        abort();
        return NULL;
    }
    return p;
}
static void def_free(void *ptr)
{
    if(ptr)
    {
        free(ptr);
        ptr = NULL;
    }
}

#endif  /* __WRAP_H__*/
