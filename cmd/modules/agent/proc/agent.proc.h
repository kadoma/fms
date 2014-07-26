#ifndef __AGENT_PROC_H
#define __AGENT_PROC_H

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
