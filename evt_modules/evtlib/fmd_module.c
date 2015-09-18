
#include <dirent.h>
#include <dlfcn.h>
#include <pthread.h>
#include <errno.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "wrap.h"
#include "fmd.h"
#include "fmd_queue.h"
#include "logging.h"
#include "fmd_module.h"

#define SO_DIR PLUGIN_DIR

/**
 * Get module name
 *
 * @return
 * 	char *
 * 	The last name
 * 	i.g /usr/lib/fm/fmd/plugins/evtsrc.disk.so
 * 	=>evtsrc.disk
 */
static char *
fmd_module_name(const char *path)
{
	char *pslash, *pdot;
	char *str;

	pslash = strrchr(path, '/');
	str = strdup(pslash + 1);
	pdot = strrchr(str, '.');
	*pdot = '\0';

	return str;
}

int
fmd_init_module(fmd_t *p_fmd, char *so_full_path)
{
    void *handle = NULL;

    char *error = NULL;
    fmd_module_t *(*fmd_module_init)(char *, fmd_t *);

    handle = dlopen(so_full_path, RTLD_LAZY);
    if(!handle) {
        error = dlerror();
        wr_log("module", WR_LOG_ERROR, "[%s] dlopen failed.[%s]", so_full_path, error);
        return -1;
    }

    fmd_module_init = dlsym(handle, "fmd_module_init");
    error = dlerror();
    if(error != NULL) {
        wr_log("module", WR_LOG_ERROR, "[%s] fmd init failed.[%s]", so_full_path, error);
        return -1;
    }

    // call every so module evtsrc_init
    fmd_module_t *p_module = (*fmd_module_init)(so_full_path, p_fmd);
    if(p_module == NULL){
        wr_log("module", WR_LOG_ERROR, "[%s] init failed.", so_full_path);
        return -1;
    }
	
    // fmd struct include all so modules one list. todo a hash map.
    list_add(&p_module->list_fmd, &p_fmd->fmd_module);
    return 0;
}

int
fmd_module_load(fmd_t *p_fmd)
{
    DIR *dirp = NULL;
    struct stat stat_buf;
    struct dirent *p_entry = NULL;

    dirp = opendir(SO_DIR);
    if(dirp == NULL){
        wr_log("module", WR_LOG_ERROR, "can't open the target dir [%s]", SO_DIR);
        return -1;
    }

    while((p_entry = readdir(dirp)) != NULL )
    {
        if(p_entry->d_name[0] == '.')
            continue;

        char so_full_path[128] = {0};
        sprintf(so_full_path, "%s/%s", SO_DIR, p_entry->d_name);

/*
        struct stat statbuf;
        lstat(so_full_path, &statbuf);
        if(!(S_ISREG(statbuf.st_mode)))
        {
            continue;
        }
*/
        char *p = strrchr(p_entry->d_name, '.');
        if((p == NULL)||(strcmp(p, ".so") != 0))
            continue;

        int ret = fmd_init_module(p_fmd, so_full_path);
        if(ret != 0)
            wr_log("module", WR_LOG_ERROR, "can't init module [%s]", so_full_path);
    }
    return 0;
}
