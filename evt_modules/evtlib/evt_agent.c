
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include "wrap.h"
#include "fmd_module.h"
#include "fmd_event.h"
#include "evt_agent.h"
#include "fmd.h"
#include "logging.h"
#include "fmd_ring.h"

#define NAME_LEN 128

static void
conf_add_sub(agent_module_t *p_agent_mod, char *eclass)
{
	struct subitem *p = (struct subitem *)def_calloc(sizeof(struct subitem), 1);

	p->si_eclass = strdup(eclass);
	
	list_add(&p->si_list, &p_agent_mod->module.list_eclass);
}

int
agent_module_conf(agent_module_t *p_agent_mod, char *so_full_path)
{
	FILE *fp = NULL;
	char buf[NAME_LEN] = {0};
	char *conf_name = NULL;
	char *eclass = (char *)def_calloc(NAME_LEN, 1);

	char *pdot = strrchr(so_full_path, '.');
	sprintf(pdot + 1, "conf");
	conf_name = so_full_path;

	INIT_LIST_HEAD(&p_agent_mod->module.list_eclass);
	INIT_LIST_HEAD(&p_agent_mod->module.list_queue);
	INIT_LIST_HEAD(&p_agent_mod->module.list_fmd);

	if ((fp = fopen(conf_name, "r")) == NULL){
		wr_log("agent", WR_LOG_ERROR, "agent conf file %s is error.", conf_name);
		return (-1);
	}

	while(fgets(buf, sizeof(buf), fp))
	{
		if (strncmp(buf, "subscribe ", 10) == 0) {
			buf[strlen(buf) - 1] = 0;	/* clear '\n' */
			strcpy(eclass, &buf[10]);
			conf_add_sub(p_agent_mod, eclass);
			continue;
		}
	}

	def_free(eclass);
	fclose(fp);
	return 0;
}

int
agent_ring_put(fmd_t *p_fmd, fmd_event_t *pevt)
{
	fmd_queue_t *p_queue = &p_fmd->fmd_queue;
	list_repaired *plist_repaired = &p_queue->queue_repaired_list;
	struct list_head *prepaired_head = &plist_repaired->list_repaired_head;

    pthread_mutex_lock(&plist_repaired->evt_repaired_lock);
	
    struct list_head *pos;

    list_for_each(pos, prepaired_head)
    {
		fmd_event_t *p_event = list_entry(pos, fmd_event_t, ev_repaired_list);
		if((p_event->dev_name == pevt->dev_name) &&(p_event->dev_id == pevt->dev_id)
			 && (p_event->evt_id == pevt->evt_id) && (p_event->agent_result == pevt->agent_result))
		{
			p_event->ev_create = pevt->ev_create;
			p_event->ev_refs = pevt->ev_refs;
			def_free(pevt->dev_name);
	        def_free(pevt->ev_class);
	        def_free(pevt->data);
	        def_free(pevt);
			wr_log("agent", WR_LOG_DEBUG, " **** list case event change." );
			
			return 0;
		}

	}
	list_add_tail(&pevt->ev_repaired_list, &plist_repaired->list_repaired_head);
    
    pthread_mutex_unlock(&plist_repaired->evt_repaired_lock);
    sem_post(&plist_repaired->evt_repaired_sem);

	return 0;
}

void *
do_agent(ring_t *ring)
{
	fmd_event_t *evt, *eresult;
	
	agent_module_t *p_agent_mod = pthread_getspecific(key_module);
	agent_modops_t *mops = p_agent_mod->mops;
	
	fmd_t *pfmd = ((fmd_module_t *)p_agent_mod)->p_fmd;

	/* consumer all events in the ring buffer */
	while(ring_stat(ring) != 0)
	{
		sem_wait(&ring->ring_contain);
		pthread_mutex_lock(&ring->ring_lock);
		
		evt = (fmd_event_t *)ring_del(ring);
		if(evt == NULL)
			wr_log("", WR_LOG_ERROR, "ring is null, get item is failed.");
		
		pthread_mutex_unlock(&ring->ring_lock);
		sem_post(&ring->ring_vaild);
		if(evt == NULL){
			wr_log("agent do ring", WR_LOG_ERROR, "agent do ring is null.");
			exit(-1);
		}

		wr_log("", WR_LOG_DEBUG, "agent get event and print.[%s]", evt->ev_class);		
		eresult = mops->evt_handle(pfmd, evt);
		if(eresult == NULL){
			wr_log("handle agent", WR_LOG_DEBUG, "event process serd/fault and log event.");
			return 0;
		}
		//put to master queue again
		wr_log("", WR_LOG_NORMAL, "event list put to ring.[%s]", eresult->ev_class);
		agent_ring_put(pfmd, eresult);
	}
    sleep(1);
	return 0;
}

void *start_agent(void *p)
{
	int ret;
	sigset_t fullset, oldset;
//	agent_module_t *emp;
	agent_modops_t *mops;

	agent_module_t *p_agent_mod = (agent_module_t *)p;
	ring_t *ring = ((agent_module_t *)p_agent_mod)->ring;

	/* set TSD */
	pthread_setspecific(key_module, p_agent_mod);

	/* signal */
	sigfillset(&fullset);
	sigdelset(&fullset, SIGKILL);
	sigdelset(&fullset, SIGSTOP);
	pthread_sigmask(SIG_BLOCK, &fullset, &oldset);

	/* container */
	//emp = (agent_module_t *)pthread_getspecific(key_module);

	/* Thread starts working... */
	wr_log("", WR_LOG_DEBUG, "[%s] agent module thread start...", p_agent_mod->module.mod_name);
	while(1) {
		do_agent(ring);
	}
	return NULL;
}

static void
pthread_init(agent_module_t *p_agent_mod)
{
	pthread_t pid;
	int ret;
	pthread_attr_t attr;

	ret = pthread_attr_init(&attr);
	if(ret < 0) {
		wr_log("agent", WR_LOG_ERROR, "agent thread is error calloc .");
		exit(errno);
	}

    ret = pthread_create(&pid, &attr, start_agent, (void *)p_agent_mod);
    if(ret < 0) {
        wr_log("agent", WR_LOG_ERROR, "agent thread is error calloc .");
        exit(errno);
	}

	pthread_attr_destroy(&attr);

	((fmd_module_t *)p_agent_mod)->mod_pid = pid;

	return;

}

fmd_module_t *
agent_init(agent_modops_t *agent_ops, char *so_path, fmd_t *p_fmd)
{
    char * mod_name = strdup(so_path);//wanghuan 
	fmd_module_t *p_fmd_module = NULL;
	ring_t *ring;
	struct list_head *pos;
	fmd_queue_t *p_queue = &p_fmd->fmd_queue;

	agent_module_t *p_agent_mod = (agent_module_t *)def_calloc(sizeof(agent_module_t), 1);
	if(p_agent_mod == NULL){
		wr_log("trace agent", WR_LOG_ERROR, "agent init is error calloc .");
		return NULL;
	}

	p_fmd_module = (fmd_module_t *)p_agent_mod;

	/* setup mops */
	p_agent_mod->mops = agent_ops;
	//p_agent_mod->module.mod_name = so_path;
    p_agent_mod->module.mod_name = mod_name;//wanghuan
	/* setup ring */
	ring = (ring_t *)def_calloc(sizeof(ring_t), 1);

	ring_init(ring);
	p_agent_mod->ring = ring;

	/* setup fmd */
	((fmd_module_t *)p_agent_mod)->p_fmd = p_fmd;

	/* read config file */
	if(agent_module_conf(p_agent_mod, so_path) < 0){
		def_free(ring);
		def_free(p_agent_mod);
		return NULL;
	}

	//subscribng events  new and head. 
	list_add(&p_fmd_module->list_queue, &p_queue->list_agent);

	pthread_init(p_agent_mod);

	return (fmd_module_t *)p_agent_mod;
}
