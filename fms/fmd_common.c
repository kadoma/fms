/************************************************************
 * Copyright (C) 2015, inspur Inc. <http://www.inspur.com>
 * FileName:    fmd_common.c
 * Author:      wanghuan
 * Date:        2015-7-15
 * Description: the fault manager system common function
 *              ./fms-2.0/fms/fmd_common.c
 *
 ************************************************************/

#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "wrap.h"
#include "logging.h"
#include "fmd_common.h"

void
fmd_timer_fire(fmd_timer_t *ptimer)
{
    pthread_mutex_lock(&ptimer->timer_lock);
    pthread_cond_signal(&ptimer->cond_signal);
    wr_log("", WR_LOG_DEBUG, "fmd time fire again ok ....");
    pthread_mutex_unlock(&ptimer->timer_lock);
}

/**
 * fmd_load_esc
 *
 * @param
 * @return
 */
void
fmd_load_esc()
{
    char path[FMS_PATH_MAX], *error;
    void *handle;
    int (*_esc_main)(fmd_t *), ret;
    void (*__stat_esc)(void);

    memset(path, 0, sizeof(path));
    sprintf(path, "%s/%s", BASE_DIR, "libesc.so");

    dlerror();
    handle = dlopen(path, RTLD_LAZY);
    if(handle == NULL) {
        syslog(LOG_ERR, "dlopen");
        exit(-1);
    }

    dlerror();
    _esc_main = dlsym(handle, "esc_main");
    if((error = dlerror()) != NULL) {
        syslog(LOG_ERR, "dlsym");
        exit(-1);
    }

    ret = (*_esc_main)(&fmd);
    if(ret < 0) {
        syslog(LOG_ERR, "_esc_main");
        exit(-1);
    }

    dlerror();
    __stat_esc = dlsym(handle, "_stat_esc");
    if((error = dlerror()) != NULL){
        syslog(LOG_ERR, "dlsym");
        exit(-1);
    }
    (*__stat_esc)();

    dlclose(handle);
}


/**
 * the file_lock funtion
 * is just for the thread check,
 * make sure only one is running.
 *
 * @name file_lock
 * @param char *
 * @return int
 *
 */
int
analy_start_para(int argc, char **argv, int *debug, int *run_mode)
{
    int ch = 0;
    while ((ch = getopt(argc, argv, "vd")) != -1) 
    {
        switch(ch) 
        {
            case 'v':
                printf("%s version: %u.%u.%u\n",argv[0], 2,0,0);
                return -1;
            case 'd':
                *debug = 1;
                break;
            default:
                usage();
                return -1;
        }
    }
    argc -= optind;
    argv += optind;
    if(argc == 1) 
    {
        if (strcasecmp(argv[0], "start")==0)
        {
            *run_mode = FMD_START;
        } 
        else if (strcasecmp(argv[0], "stop")==0) 
        {
            *run_mode = FMD_STOP;
        }
        else
        {
            usage();
            return -1;
        }
    }
    else if (argc != 0) 
    {
        usage();
        return -1;
    }
    
    return 0;
}


/**
 * the file_lock funtion
 * is just for the thread check,
 * make sure only one is running.
 *
 * @name file_lock
 * @param const char *
 * @return int
 *
 */
int
file_lock(char *app_name)
{
    char lockfile[64] = {0};
    sprintf(lockfile, ".%s.lock", app_name);

    int lfp = open(lockfile, O_WRONLY|O_CREAT, 0666);
    if(lfp < 0)
    {
        wr_log("file lock", WR_LOG_ERROR, "can't create lockfile in working directory");
        return -1;
    }

    pid_t owner_pid = mylock(lfp);
    if (owner_pid < 0)
    {
        wr_log("", WR_LOG_ERROR, "contrl error.");
        return -1;
    }
    if (owner_pid > 0)
    {
        return owner_pid;
    }
    return 0;
}


/**
 *
 * @name wait_lock_free
 * @param char *
 * @return int
 *
 */

int
wait_lock_free(char *app_name)
{
    char lockfile[64] = {0};
    sprintf(lockfile, ".%s.lock", app_name);

    int lfp = open(lockfile, O_WRONLY|O_CREAT, 0666);
    if (lfp < 0)
    {
        printf("can't create lockfile in working directory\n");
        return -1;
    }

    pid_t  owner_pid = 0;
    int i = 0;
    while(1)
    {
        owner_pid = mylock(lfp);
        if(owner_pid == 0)
        {
            printf("%ds...\n", i);
            return 0;
        }
        else
        {
            i++;
            sleep(1);
            if(i % 5 == 0)
            {
                printf("%ds...\n", i);
            }
        }
    }
    return 0;
}


pid_t
mylock(int fd)
{
    struct flock fl;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;

    if (fcntl(fd, F_SETLK, &fl) >= 0)
    {   // lock set
        return 0;   // ok
    }
    if (errno != EAGAIN)
    {   // error other than "already locked"
        return -1;  // error
    }
    if (fcntl(fd, F_GETLK, &fl) < 0)
    {   // get lock owner
        return -1;  // error getting lock
    }
    if (fl.l_type != F_UNLCK)
    {   // found lock
        return fl.l_pid;    // return lock owner
    }
    return -1;  // pro forma
}

void
rv_b2daemon()
{
    long lPid = 0;

    signal(SIGINT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    lPid = fork();

    if(lPid < 0)
    {
        fprintf(stderr,"fork error [%d] [%s]\n",errno,strerror(errno));
        exit(-1);
    }
    else if(lPid > 0)
    {
        exit(0);
    }
//  close(STDIN_FILENO);

//  close(STDOUT_FILENO);

//  close(STDERR_FILENO);

    setpgrp( );
    return;
}

int
modprobe_kfm(void)
{
    system("/sbin/insmod /lib/modules/`uname -r`/kernel/fm/kfm.ko 1>/dev/null 2>/dev/null");
    
    return 0;
}

void usage() 
{
    printf(
"usage: fmd [start]|[stop] ||[-v] || [-d].\n"
"\n"
"[-v] : print version number and exit\n"
"[-d] : run mode.run in foreground and show debug info\n\n");

    exit(1);
}

