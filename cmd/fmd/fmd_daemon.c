/*
 * fmd_daemon.c
 *
 *  Created on: Mar 1, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	fmd_daemon.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * child_handler
 *
 * @param
 * @return
 */
static void
child_handler(int signum)
{
    switch(signum) {
    case SIGALRM:
    	exit(EXIT_FAILURE);
    	break;
    case SIGUSR1:
    	exit(EXIT_SUCCESS);
    	break;
    case SIGCHLD:
    	exit(EXIT_FAILURE);
    	break;
    }
}


/**
 * daemonize
 *
 * @param
 * @return
 */
void
daemonize(const char *lockfile)
{
    pid_t pid, sid, parent;
    int lfp = -1;

    /* already a daemon */
    if (getppid() == 1) return;

    /* Create the lock file as the current user */
    if (lockfile && lockfile[0]) {
        lfp = open(lockfile, O_RDWR|O_CREAT, 0640);
        if (lfp < 0) {
            syslog(LOG_ERR, "unable to create lock file %s, code=%d (%s)",
                    lockfile, errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    /* Trap signals that we expect to recieve */
    signal(SIGCHLD, child_handler);
    signal(SIGUSR1, child_handler);
    signal(SIGALRM, child_handler);

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "unable to fork daemon, code=%d (%s)",
                errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0) {

        /* Wait for confirmation from the child via SIGTERM or SIGCHLD, or
           for two seconds to elapse (SIGALRM).  pause() should not return. */
        alarm(2);
        pause();

        exit(EXIT_FAILURE);
    }

    /* At this point we are executing as the child process */
    parent = getppid();

    /* Cancel certain signals */
    signal(SIGCHLD, SIG_DFL); /* A child process dies */
    signal(SIGTSTP, SIG_IGN); /* Various TTY signals */
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGHUP, SIG_IGN); /* Ignore hangup signal */
    signal(SIGTERM, SIG_DFL); /* Die on SIGTERM */

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        syslog(LOG_ERR, "unable to create a new session, code %d (%s)",
                errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory.  This prevents the current
       directory from being locked; hence not being able to remove it. */
    if ((chdir("/")) < 0) {
        syslog(LOG_ERR, "unable to change directory to %s, code %d (%s)",
                "/", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Redirect standard files to /dev/null */
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);

    /* Tell the parent process that we are A-okay */
    kill(parent, SIGUSR1);
}
