
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "wrap.h"
#include "fmd.h"
#include "fmd_nvpair.h"
#include "fmd_module.h"
#include "evt_src.h"
#include "logging.h"
#include "fmd_event.h"
#include "fmd_api.h"

int
src_module_conf(evtsrc_module_t *p_evt_mod, char *so_full_path)
{
	FILE *fp = NULL;
	char *conf_name = NULL;
	char buf[64];
	char buff[64];
	
	char *pdot = strrchr(so_full_path, '.');
	sprintf(pdot + 1, "conf");
	conf_name = so_full_path;
	
	memset(buf, 0, 64);
	memset(buff, 0, 64);

	if ((fp = fopen(conf_name, "r")) == NULL) {
		wr_log("", WR_LOG_ERROR, "FMD: failed to open conf file %s\n", conf_name);
		return (-1);
	}

	while(fgets(buf, sizeof(buf), fp)){
		if (strncmp(buf, "interval ", 9) == 0){
			strcpy(buff, &buf[9]);
			p_evt_mod->module.mod_interval = atoi(buff);

			fclose(fp);
			return 0;
		}
	}

	fclose(fp);
	wr_log("", WR_LOG_ERROR, "conf file is null. %s\n", conf_name);
	return (-1);
	
}


void *
fmd_queue_dispatch(evtsrc_module_t *evt_mod_p, fmd_event_t *pevt)
{
    fmd_t *pfmd = ((fmd_module_t *)evt_mod_p)->p_fmd;
    fmd_queue_t *p_queue = &pfmd->fmd_queue;

    ring_t *ring = &p_queue->queue_ring;
    sem_wait(&ring->ring_vaild);

    pthread_mutex_lock(&ring->ring_lock);
    ring_add(ring, pevt);
    pthread_mutex_unlock(&ring->ring_lock);

    sem_post(&ring->ring_contain);
	wr_log("", WR_LOG_DEBUG, "src add to ring, count is [%d].", ring->count);
    return NULL;

}

static void
evt_handle(evtsrc_module_t *evt_mod_p, struct list_head *head)
{
    if(evt_mod_p == NULL){
        wr_log("fmd", WR_LOG_ERROR, "event handle the evt is null.");
        return;
    }

    struct list_head *pos, *n = NULL;
    nvlist_t *nvl = NULL;
    char *eclass = NULL;
    fmd_t *p_fmd = ((fmd_module_t *)evt_mod_p)->p_fmd;

    if(list_empty(head)){
        def_free(head);
		wr_log("", WR_LOG_DEBUG, "event nvl list is null.");
        return;
    }

    list_for_each_safe(pos, n, head)
	{
        nvl = list_entry(pos, nvlist_t, nvlist);

        eclass = nvl->value;
        if(eclass[0] == '\0')
            continue;

        fmd_event_t *evt = fmd_create_ereport(p_fmd, eclass, NULL, nvl);
        if(evt == NULL){
            wr_log("fmd", WR_LOG_ERROR,
                        "[%s]src deteck event is NULL.", evt_mod_p->module.mod_name);
        }
        else{
            wr_log("",WR_LOG_DEBUG,
                      "[%s], module event is [%s]", evt_mod_p->module.mod_name, evt->ev_class);
        }
        fmd_queue_dispatch(evt_mod_p, evt);
    }
	
	def_free(head);
	return;
}

void *
do_evtsrc()
{
    struct list_head *rawevt;
    fmd_event_t *evt;
    evtsrc_module_t *evt_mod_p;
    evtsrc_modops_t *mops;

    /* Begins to callback */
    evt_mod_p = pthread_getspecific(key_module);
    mops = evt_mod_p->mops;

    /* get raw event */
    rawevt = mops->evt_probe(evt_mod_p);
    if(mops->evt_probe == NULL ){
        wr_log("", WR_LOG_ERROR, "event src so is not probe function.");
        return NULL;
    }

    if(rawevt == NULL){
        wr_log("fmd", WR_LOG_DEBUG, "no ereport event.");
        return NULL;
    }

    evt_handle(evt_mod_p, rawevt);
    return NULL;
}

static void *
evtsrc_start(void *p)
{
	evtsrc_module_t *evt_mod_p = (evtsrc_module_t *)p;
    sigset_t fullset, oldset;
    fmd_timer_t *timer = NULL;

    /* set TSD */
    pthread_setspecific(key_module, evt_mod_p);

    /* signal */
    sigfillset(&fullset);
    sigdelset(&fullset, SIGKILL);
    sigdelset(&fullset, SIGSTOP);
    pthread_sigmask(SIG_BLOCK, &fullset, &oldset);

    /* container */
    evt_mod_p = (evtsrc_module_t *)pthread_getspecific(key_module);
    timer = &evt_mod_p->timer;

	wr_log("", WR_LOG_DEBUG, "start event deteck thread [%s].", evt_mod_p->module.mod_name);
    while(1){
        pthread_mutex_lock(&timer->timer_lock);
		
		wr_log("",WR_LOG_DEBUG,"deteck thread waitting signal." );
        pthread_cond_wait(&timer->cond_signal, &timer->timer_lock);
        pthread_mutex_unlock(&timer->timer_lock);

        do_evtsrc();
    }

    return NULL;
}

static pthread_t
pthread_init(evtsrc_module_t *evt_mod_p)
{
    pthread_t pid;
    int ret;
    pthread_attr_t attr;

    ret = pthread_attr_init(&attr);
    if(ret < 0) {
        syslog(LOG_ERR, "%s\n", strerror(errno));
        return -1;
    }

    ret = pthread_create(&pid, &attr, evtsrc_start, (void *)evt_mod_p);
    if(ret < 0) {
        syslog(LOG_ERR, "pthread_create");
        exit(errno);
    }

	// wanghuan
    pthread_attr_destroy(&attr);

    evt_mod_p->timer.tid = pid;
    //((fmd_module_t *)evt_mod_p)->mod_pid = pid;

    return pid;
}

/**
 * @param
 * @return
 */

static int
timer_init(evtsrc_module_t *evt_mod_p, fmd_t *p_fmd)
{
    fmd_module_t *fmd_mod_p = &evt_mod_p->module;
    //time_t tv = evt_mod_p->mod_interval;
    fmd_timer_t *ptimer = &evt_mod_p->timer;

    ptimer->interval = evt_mod_p->module.mod_interval;
    ptimer->ticks = 0;
    pthread_cond_init(&ptimer->cond_signal, NULL);
    pthread_mutex_init(&ptimer->timer_lock, NULL);
	
    list_add(&ptimer->timer_list, &p_fmd->fmd_timer);
    return 0;
}

fmd_module_t *
evtsrc_init(evtsrc_modops_t *mod_ops_p, char* full_path, fmd_t *p_fmd)
{
    char * mod_name = strdup(full_path);//wanghuan
    evtsrc_module_t *evt_mod_p = NULL;
    evt_mod_p = (evtsrc_module_t *)def_calloc(1, sizeof(evtsrc_module_t));

    //set ops link to module struct
    evt_mod_p->mops = mod_ops_p;
	evt_mod_p->module.p_fmd = p_fmd;
	//evt_mod_p->module.mod_name = full_path;
    evt_mod_p->module.mod_name = mod_name;//wanghuan

    // read config file, path for read conf file
    if(src_module_conf(evt_mod_p, full_path) < 0){
        def_free(evt_mod_p);
        return NULL;
    }

    // init time
    timer_init(evt_mod_p, p_fmd);

    pthread_init(evt_mod_p);

    return &evt_mod_p->module;
}
