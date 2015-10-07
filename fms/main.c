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

#include "fmd_common.h"

static int stop_flag = 0;
char *app_name = "fmd";

static pthread_mutex_t  m_mutex_exit;
static pthread_cond_t   m_waitexit;

//fmd_t fmd __attribute__ ((section ("global")));

int
main(int argc, char **argv)
{
	int debug_mode = 0;
    int run_mode = FMD_START;

    int ret = analy_start_para(argc, argv, &debug_mode, &run_mode);
	if(ret != 0)
		return 0;

    if(run_mode == FMD_START && debug_mode == 0)
        rv_b2daemon();
	else{
		wr_log_set_loglevel(WR_LOG_DEBUG);
		wr_log_logrotate(0);
		//wr_log_logrotate(debug_mode);
	}
    
    ret = file_lock(app_name);
	//not exist
    if(ret == 0 && run_mode == FMD_STOP)
    {
        return 0;
    }
    if(ret > 0 && run_mode == FMD_STOP)
    {
        wr_log("", WR_LOG_DEBUG, "%s begin terminate ...", app_name);
        kill(ret, SIGTERM);
        wait_lock_free(app_name);
        wr_log("", WR_LOG_DEBUG, "%s terminated!", app_name);
        return 0;
    }
    if(ret > 0 || ret < 0)
    {
        wr_log("", WR_LOG_ERROR, "%s is running, cann't start again!", app_name);
        return 0;
    }

	//default log level 
	wr_log_logrotate(debug_mode);
	int log_level = WR_LOG_DEBUG;
	wr_log_init("./fms.log");
	wr_log_set_loglevel(log_level);

	wr_log("fmd", WR_LOG_DEBUG, "**** fmd start **** .......");

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
    while(1)
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
		if(stop_flag)
			break;
	}

	//sleep(11111);
	
    pthread_mutex_init(&m_mutex_exit, NULL);
    pthread_cond_init(&m_waitexit, NULL);
    
    struct sigaction sa;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = fmd_exitproc;
    
    sigaction(SIGTERM, &sa, (struct sigaction *)0);
    sigaction(SIGINT, &sa, (struct sigaction *)0);
        
    pthread_mutex_lock(&m_mutex_exit);
    pthread_cond_wait(&m_waitexit, &m_mutex_exit);
    pthread_mutex_unlock(&m_mutex_exit);
	
	wr_log("fmd", WR_LOG_DEBUG, "**** fmd stop **** .......");
    return 0;
}

void  fmd_exitproc()
{
	stop_flag = 1;
	
    wr_log("", WR_LOG_NORMAL, "Stop deteck threads.....");
	//stop_deteck_threads();
    wr_log("", WR_LOG_NORMAL, "Stop deteck threads  finished\n");


    wr_log("", WR_LOG_NORMAL, "Stop deteck threads.....");
	//stop_process_threads();
    wr_log("", WR_LOG_NORMAL, "Stop deteck threads  finished\n");
	
    wr_log("", WR_LOG_NORMAL, "Stop deteck threads.....");
	//stop_deteck_threads();
    wr_log("", WR_LOG_NORMAL, "Stop deteck threads  finished\n");

    pthread_cond_signal(&m_waitexit);
}
