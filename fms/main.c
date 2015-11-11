/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    main.c
 * Author:      Inspur OS Team 
                wanghuanbj@inspur.com
 * Date:        20xx-xx-xx
 * Description: the fault manager system main function
 *
 ************************************************************/

#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/resource.h>

#include "fmd_common.h"

static int stop_flag = 0;
const char *app_name = "fmd";
#define PID_FILE     "/var/run/fms.pid"

static pthread_mutex_t  m_mutex_exit;
static pthread_cond_t   m_waitexit;

#define DEBUG_MODE 0

/* {mian}{minor}{small} : {00}{00}{00}: HEX */
#define FMS_VERSION_CODE        0x020005    
#define NVERSION(version)                  \
    (version >> 16) & 0xFF,                 \
    (version >> 8) & 0xFF,                  \
    version & 0xFF

//fmd_t fmd __attribute__ ((section ("global")));

static void 
fmd_usage(void)
{
#define FMT "    %-28s # %s\n"

    fprintf(stderr, "usage: fmd [options]\n");
    fprintf(stderr, FMT, "-d", "debug log");
    fprintf(stderr, FMT, "-k", "stop daemon");
    fprintf(stderr, FMT, "-v", "display version");
#undef FMT
    exit(0);
}

static void 
print_version()
{
    printf("Version: FMS/%d.%d.%d \n", 
            NVERSION(FMS_VERSION_CODE));
    
    exit(0);
}

static pid_t 
get_running_pid()
{
    pid_t pid;
    FILE *lockfd;
    
    if ((lockfd = fopen(PID_FILE, "r")) != NULL
            &&    fscanf(lockfd, "%d", &pid) == 1 && pid > 0) {
        if (kill(pid, 0) >= 0 || errno != ESRCH) {
            fclose(lockfd);
            return(pid);
        }
    }
    
    if (lockfd != NULL) {
        fclose(lockfd);
    }

    return 0;
}

static int 
set_running_pid(pid_t run_pid)
{
    FILE *fp = NULL;
    
    if(!(fp = fopen(PID_FILE, "w+"))) {
        wr_log("fmd", WR_LOG_DEBUG, 
            "open pid file for write failed:%s", strerror(errno));
        return -1;
    }

    fprintf(fp, "%d\n", run_pid);
    fclose(fp);
    
    return 0;
}

static void 
kill_daemon(void)
{
    pid_t run_pid = 0;
    
    run_pid = get_running_pid();
    if (run_pid > 0) {
        kill(run_pid, SIGTERM);
    }

    exit(0);
}

static void
sandbox(void)
{    
    struct rlimit limit;
    int ret;
    ret = getrlimit(RLIMIT_CORE, &limit);
    if(ret == 0) {
        wr_log("fmd", WR_LOG_DEBUG,
            "getrlimit corefile size limit_cur = %d, max = %d", 
            limit.rlim_cur, limit.rlim_max);
        
        limit.rlim_max = limit.rlim_cur = RLIM_INFINITY;
          ret = setrlimit(RLIMIT_CORE, &limit);
        
        wr_log("fmd", WR_LOG_DEBUG,
              "setrlimit corefile size limit:cur = %d, return %d", 
              limit.rlim_cur, ret);
    }
    return;
    
    /* On Linux >= 2.4, you need to set the dumpable flag
       to get core dumps after you have done a setuid. */
    if (prctl(PR_SET_DUMPABLE, 2) != 0)
        wr_log("fmd", WR_LOG_DEBUG,
            "Could not set dumpable bit.  Core dumps turned off. %d %s",
            errno,
            strerror(errno));
}

static void 
signal_term(int sig)
{
    stop_flag = 1;

    wr_log("fmd", WR_LOG_DEBUG, 
            "singnal term.");
}

int
main(int argc, char **argv)
{
    int o;
    pid_t run_pid = 0;
	int log_level = WR_LOG_NORMAL;

    app_name = strrchr(argv[0], '/');
    if (!app_name)
        app_name = argv[0];
    else
        app_name += 1;

    while ((o = getopt(argc, argv, "dkv")) != -1) {
        switch (o) {
        case 'd':
			log_level = WR_LOG_DEBUG;
            break;
        case 'k':
            kill_daemon();
            break;
        case 'v':
            print_version();
            break;
        default:
            fmd_usage();
        }    
    }

    wr_log_logrotate(DEBUG_MODE);
    wr_log_init("/var/log/fms/fms.log");
    wr_log_set_loglevel(log_level);

    /* check fmd is running  */
    if ((run_pid = get_running_pid())) {
        wr_log("fmd", WR_LOG_WARNING, 
            "%s already start.", 
            app_name);
        exit(0);
    }

    if(!DEBUG_MODE)
        rv_b2daemon();

    /* open coredump */
    sandbox();    
    
    if (set_running_pid(getpid())) {
        wr_log("fmd", WR_LOG_ERROR, 
            "failed to set running pid.");
        exit(0);
    }

    signal(SIGTERM, signal_term);
    
    wr_log("fmd", WR_LOG_NORMAL, 
        "**** fmd start[%d] **** .......",
        getpid());

    /* insmod kfm.ko */
    modprobe_kfm();

    INIT_LIST_HEAD(&fmd.fmd_timer);
    INIT_LIST_HEAD(&fmd.fmd_module);
    
    INIT_LIST_HEAD(&fmd.list_case);
    INIT_LIST_HEAD(&fmd.list_repaired_case);

    fmd_queue_init(&fmd.fmd_queue);

    // esc file read and load.
    fmd_load_esc();

    // begin to loop the distribute queue
    fmd_queue_load(&fmd);

    //begin to init and start all src agent modules
    fmd_module_load(&fmd);

    // ready to work
    sleep(1);

    fmd_timer_t *ptimer = NULL;
    while(!stop_flag)
    {
        struct list_head *pos;
        list_for_each(pos, &fmd.fmd_timer){
            ptimer = list_entry(pos, fmd_timer_t, timer_list);
            ptimer->ticks ++;
            if(ptimer->ticks == ptimer->interval)
            {
                fmd_timer_fire(ptimer);
                ptimer->ticks = 0;
            }
        }
        // unit one second
        sleep(1);
    }
    
    wr_log("fmd", WR_LOG_NORMAL, 
        "**** fmd stop[%d] **** .......",
        getpid());

	unlink(PID_FILE);
    return 0;
}

void  fmd_exitproc()
{
    return;
}
