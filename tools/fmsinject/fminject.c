/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    main.c
 * Author:      Inspur OS Team 
                guomeisi@inspur.com
 * Date:        20xx-xx-xx
 * Description: 
 *
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#define INJECT_INTERVAL 1

extern FILE *yyin;
mqd_t mqd;

/**
 * repval
 *
 * @param
 * 	N=33
 * @return
 */
int
repval(const char *str)
{
	char *p = strchr(str, '=') + 1;
	return atoi(p);
}


void
fm_inject(const char *eclass, int repeat)
{
	int count = 0, len = strlen(eclass);
	char *buff = NULL;

	/* NULL-terminated */
	buff = (char *)calloc(len + 1, sizeof(char));
	strcpy(buff, eclass);
	for(; count < repeat; count++) {
		mq_send(mqd, buff, len + 1, 0);
		sleep(INJECT_INTERVAL);
	}
}

int
main(int argc, char *argv[])
{
	if(argc < 2) {
		printf("Usage: ./fminject <filename>\n");
		exit(EXIT_FAILURE);
	}

	/* mqueue */
	mqd = mq_open("/fmd", O_RDWR | O_CREAT, 0644, NULL);
	if(mqd == -1) {
		perror("mq_open");
		exit(EXIT_FAILURE);
	}

	/* parse */
	yyin = fopen(argv[1], "r+");
	assert(yyin != NULL);
	yyparse();
	fclose(yyin);

	/* close mqueue */
	mq_close(mqd);
	return 0;
}
