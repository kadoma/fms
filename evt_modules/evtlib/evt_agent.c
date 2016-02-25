
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
#include "atomic_64.h"

#define MAXLINE				1024
#define WHITESPACE			" \t\n\r\f"
#define DELIMS				",: \t\n\r\f"
#define EOS					'\0'
#define COMMENTCHAR			'#'
#define ARRAY_SIZE(arr) 	(sizeof(arr) / sizeof((arr)[0]))

#define NAME_LEN 128

static int set_evt_handle_mode(void *mconf, const char *option);

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

struct directive {
	const char *name;
	int (*set_func) (void *, const char *);
} g_directives[] = {
	{ AGENT_KEY_EVT_HANDLE_MODE, set_evt_handle_mode}
	/* add other directive */
};

static int
set_evt_handle_mode(void *mconf, const char *option)
{
	agent_modconfs_t *conf = (agent_modconfs_t *)mconf;
	int i;
	struct {
		char *s;
		int v;
	} mode[] = {
		{"manual", EVENT_HANDLE_MODE_MANUAL},
		{"auto", EVENT_HANDLE_MODE_AUTO},
		{"define", EVENT_HANDLE_MODE_DEFINE},
		{"\0", -1}
	};

	for (i = 0; mode[i].s[0] != '\0'; i++) {
		if (strcmp(mode[i].s, option) == 0) {
			conf->evt_handle_mode = mode[i].v;
			return 0;
		}
	}

	wr_log("agent", WR_LOG_ERROR, 
		"Illegal event handle mode [%s]", option);

	return -1;
}

static struct directive*
get_directive(const char *name)
{
	int	j;

	for (j = 0; j < ARRAY_SIZE(g_directives); ++j) {
		if (strcmp(name, g_directives[j].name) == 0) {
			return &g_directives[j];
		}
	}
	
	return NULL;
}

int 
parse_config(agent_modconfs_t *mconf, const char *cfgfile)
{
	FILE *f;
	char buf[MAXLINE];
	char *cp;
	char directive[MAXLINE];
	struct directive *dp;
	int dirlength;
	char option[MAXLINE];
	int optionlength;
	int errcount = 0;
	
	if ((f = fopen(cfgfile, "r")) == NULL) {
		if (errno == ENOENT)	/* file does not exist, default value */
			return 0;
		
		wr_log("agent", WR_LOG_ERROR, "agent conf file %s is error.", cfgfile);
		return (-1);
	}
	
	while (fgets(buf, MAXLINE, f) != NULL) {
		char *bp = buf;
				
		/* Skip over white space */
		bp += strspn(bp, WHITESPACE);

		/* Comments on the line */
		if ((cp = strchr(bp, COMMENTCHAR)) != NULL)  {
			*cp = EOS;
		}

		/* Ignore blank (and comment) lines */
		if (*bp == EOS) {
			continue;
		}
		
		/* Now we expect a directive name */
		dirlength = strcspn(bp, WHITESPACE);
		strncpy(directive, bp, dirlength);
		directive[dirlength] = EOS;
		dp = get_directive(directive);
		if (dp == NULL) {
			wr_log("agent", WR_LOG_WARNING, 
				"Illegal directive [%s] in %s", directive, cfgfile);
			++errcount;
			continue;
		}

		bp += dirlength;

		/* Skip over Delimiters */
		bp += strspn(bp, DELIMS);
		
		if (*bp != EOS) {
			optionlength = strcspn(bp, DELIMS);
			strncpy(option, bp, optionlength);
			option[optionlength] = EOS;
			
			if (dp->set_func(mconf, option)) {
				errcount++;
			}
		}
	}
	fclose(f);
	
	return(errcount ? -1 : 0);
}

/* read configuration file xxx_agent_mod.conf */
static int
agent_module_conf2(agent_module_t *p_agent_mod, const char *so_full_path)
{
	char conf_name[MAXLINE];
	char *pdot = NULL;
	FILE *fp = NULL;
	int len = 0;
	agent_modconfs_t *mconfs_p = &p_agent_mod->mconfs;

	/* default value */
	mconfs_p->evt_handle_mode = EVENT_HANDLE_MODE_AUTO;
	
	/* configuration file path */
	pdot = strrchr(so_full_path, '.');
	len = pdot - so_full_path;
	strncpy(conf_name, so_full_path, len);
	sprintf(conf_name + len, "_mod.conf");

	wr_log("agent", WR_LOG_DEBUG, "parse config file [%s]", conf_name);

	/* parse config */
	if (parse_config(mconfs_p, conf_name)) {
		return -1;
	}

	return 0;
}

static int
update_conf(agent_modconfs_t *mconf, const char *cfgfile, 
	char *direct, char *value)
{
	FILE *f, *ftmp = NULL;
	int fd;
	char tmp_file[MAXLINE];
	char buf[MAXLINE];
	char *cp;
	char directive[MAXLINE];
	struct directive *dp;
	int dirlength;
	int direct_update = 0;

	/* Illegal directive */
	dp = get_directive(direct);
	if (dp == NULL) {
		wr_log("agent", WR_LOG_ERROR, 
			"Illegal directive [%s]", directive);
		return -1;
	}
	
	/* open conf file */
	if ((f = fopen(cfgfile, "r")) == NULL) {
		if (errno != ENOENT) {
			wr_log("agent", WR_LOG_ERROR, "agent conf file %s is error.", cfgfile);
			return (-1);
		} else { /* file does not exist, create */
			if ((f = fopen(cfgfile, "w+")) == NULL)
				return -1;
		}
	}

	/* create temporary file */
	sprintf(tmp_file, "%sXXXXXX", cfgfile);
	fd = mkstemp(tmp_file);
	if (fd == -1) {
		fclose(f);
		return -1;
	}
	if ((ftmp = fdopen(fd, "w")) == NULL)
		goto out;

	/* update conf file */
	while (fgets(buf, MAXLINE, f) != NULL) {
		char *bp = buf;
				
		/* Skip over white space */
		bp += strspn(bp, WHITESPACE);

		/* Comments line or blank line */
		if (*bp == COMMENTCHAR || *bp == EOS) {
			fprintf(ftmp, "%s", buf);
			continue;
		}
				
		/* Now we expect a directive name */
		dirlength = strcspn(bp, WHITESPACE);
		strncpy(directive, bp, dirlength);
		directive[dirlength] = EOS;

		if (strcmp(directive, direct) == 0) {
			if (dp->set_func(mconf, value))
				goto out;
			fprintf(ftmp, "%s %s\n", direct, value);
			direct_update++;
			continue;
		}

		fprintf(ftmp, "%s", buf);
	}

	/* directive does not exist int the conf file */
	if (!direct_update) {
		if (dp->set_func(mconf, value))
			goto out;
		fprintf(ftmp, "%s %s\n", direct, value);
	}
	
	fclose(f);
	fclose(ftmp);

	if (rename(tmp_file, cfgfile))
		goto out;
	
	return 0;

out:
	fclose(f);
	if (ftmp)
		fclose(ftmp);
	else 
		close(fd);
	unlink(tmp_file);
	return -1;
}

int
agent_module_update_conf(fmd_t *fmd, char *mod_name, 
	char *conf_path, char *direct, char *value)
{
	char conf_name[MAXLINE];
	fmd_module_t *modp;
	agent_module_t *p_agent_mod;
		
	/* invalid parameter */
	if (mod_name == NULL || direct == NULL || value == NULL)
		return -1;
	
	/* is agent module */
	if (strstr(mod_name, "agent") == NULL) {
		wr_log("agent", WR_LOG_ERROR, 
			"module[%s] is not a agent module.", mod_name);
		return -1;
	}

	/* find module */
	modp = fmd_get_module_by_name(fmd, mod_name);
	if (modp == NULL) {
		wr_log("agent", WR_LOG_ERROR, 
			"module[%s] does not exist.", mod_name);
		return -1;
	}
	p_agent_mod = (agent_module_t *)modp;
	
	/* get configuration file path */
	if (conf_path) {
		sprintf(conf_name, "%s/%s_mod.conf", conf_path, mod_name);
	} else {
		char *so_full_path = modp->mod_name;
		char *pdot = NULL;
		int len;

		pdot = strrchr(so_full_path, '.');
		len = pdot - so_full_path;
		strncpy(conf_name, so_full_path, len);
		sprintf(conf_name + len, "_mod.conf");
	}

	/* update conf */
	if (update_conf(&p_agent_mod->mconfs, conf_name, direct, value)) {
		return -1;
	}

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
	struct list_head *listn;

    list_for_each_safe(pos, listn, prepaired_head)
    {
		fmd_event_t *p_event = list_entry(pos, fmd_event_t, ev_repaired_list);
		if((strncmp(p_event->dev_name, pevt->dev_name, strlen(pevt->dev_name)) == 0) &&(p_event->dev_id == pevt->dev_id)
			 && (p_event->evt_id == pevt->evt_id) && (p_event->agent_result == pevt->agent_result))
		{
			p_event->ev_create = pevt->ev_create;
			p_event->ev_refs = pevt->ev_refs;
			p_event->ev_count++;
			def_free(pevt->dev_name);
	        def_free(pevt->ev_class);
			// for 8 cpus test wanghuan
	        //(pevt->data);
	        def_free(pevt);
			wr_log("agent", WR_LOG_DEBUG, " **** list case event change." );
			
			pthread_mutex_unlock(&plist_repaired->evt_repaired_lock);
			return 0;
		}

	}
	pevt->ev_count = 1;
	list_add_tail(&pevt->ev_repaired_list, &plist_repaired->list_repaired_head);
    
    pthread_mutex_unlock(&plist_repaired->evt_repaired_lock);

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
        
		evt->handle_mode = p_agent_mod->mconfs.evt_handle_mode;
		eresult = mops->evt_handle(pfmd, evt);
        
        fmd_case_t *pp_case = (fmd_case_t *)evt->p_case;
        if(evt->ev_flag != AGENT_TODO)
            atomic_dec(&pp_case->m_event_using);
        
		if(eresult == NULL){

			wr_log("handle agent", WR_LOG_DEBUG, "event process serd/fault and log event.");
			return 0;
		}
		//put to master queue again
		wr_log("", WR_LOG_DEBUG, "event list put to ring.[%s], result is [%d]", 
											eresult->ev_class, eresult->agent_result);
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
	if (agent_module_conf2(p_agent_mod, so_path)) {
		def_free(ring);
		def_free(p_agent_mod);
		return NULL;
	}
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
