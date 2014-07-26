#ifndef __PROC_EVTSRC_H
#define __PROC_EVTSRT_H

#include <time.h>

#define PROC_CONFIG_FILE	"/usr/lib/fm/fmd/plugins/evtsrc.proc.monitor.xml"

#define PROC_STAT_STOP		"stop"
#define PROC_STAT_ZOMBIE	"zombie"

struct proc_mon {
	char	process_name[PATH_MAX];		/* name of process */
#define PROC_OP_NOTHING		0		/* nothing to do when dead */
#define	PROC_OP_RESTART		1		/* restart when dead */
	uint8_t	operation;			/* thing to do when process dead */
	long	interval;			/* monitor interval, in microsecond(us) */
	char	command[PATH_MAX];		/* command used to restart process */
	char	args[PATH_MAX];			/* args of command */

	struct 	timeval	last_check;		/* last check time */
	struct	proc_mon *next;			/* pointer to next node */
};

#endif
