/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    fms_hotplug.c
 * Author:      Inspur OS Team
                wang.leibj@inspur.com
 * Date:        2015-10-29
 * Description: clear caselist
 *
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <mqueue.h>

int clear_caselist(char* dev){

    mqd_t mqd;
    struct mq_attr attr;
    void *recvbuf = NULL;
    int len = strlen(dev);

    mqd = mq_open("/hotplug", O_RDWR | O_CREAT, 0644, NULL);
    if (mqd == -1) {
        perror("hotplug mq_open");
        return (-1);
    }

    int ret = 0;
    ret = mq_send(mqd, dev, len * sizeof (char), 0);
    if (ret < 0) {
        perror("Clnt message posted");
        perror("hotplug faulty:");
        return (-1);
    }
    /* Determine max. msg size; allocate buffer to receive msg */
    if (mq_getattr(mqd, &attr) == -1) {
        perror("hotplug mq_getattr");
        return (-1);
    }


    recvbuf = malloc(attr.mq_msgsize);
    if (recvbuf == NULL) {
        perror("hotplug malloc");
        return (-1);
    }

    memset(recvbuf, 0, attr.mq_msgsize);

    ret = mq_receive(mqd, recvbuf, attr.mq_msgsize, NULL);
    if (ret < 0) {
        perror("hotplug msgrcv");
        return (-1);
    }

    mq_close(mqd);
printf("recvbuf = %s \n",recvbuf);
    if(strstr(recvbuf,"success") != NULL)
        return 0;
    else
        return (-1);
}

void usage()
{
    printf("./fms_hotplug [device]\n");
}

int main(int argc,char* argv[]){

    if(argc != 2)
    {
        usage();
        return (-1);
    }

    int ret = clear_caselist(argv[1]);

    return ret;
}
