/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    fm_cmd.c
 * Author:      Inspur OS Team
 * Date:        2015-08-04
 * Description: define the command of parsing disk.
 *
 ************************************************************/

#include <stdio.h>
#include <string.h>

//del
char* smartworkcmd(const char *path, char *cmdresult)
{
    sprintf(cmdresult,"disktool -i %s | grep 'SMART support is' | cut -f 2 -d ':' 1>/dev/null 2>/dev/null", path);
    return cmdresult;
}

//del
char* smartsupportcmd(const char *path, char *cmdresult)
{
    sprintf(cmdresult,"disktool -i %s | grep 'Device supports SMART and is Enabled' 1>/dev/null 2>/dev/null", path);
    return cmdresult;
}

//del
char* smartoncmd(const char *path, char *cmdresult)
{
    sprintf(cmdresult,"disktool -s on %s 1>/dev/null 2>/dev/null",path);
    return cmdresult;
}

void smarttempcmd(const char *path, char *cmd)
{
    sprintf(cmd,"disktool -A %s | grep 'Current Drive Temperature' | cut -f 2 -d ':'| cut -f 1 -d 'C' 2>/dev/null", path);
    return;
}

void smarthealthycmd(const char *path, char *cmd)
{
    sprintf(cmd,"disktool -H %s", path);
    return;
}

char* smartokcmd(const char *path,char *cmdresult)
{
    sprintf(cmdresult,"disktool -H %s | grep 'Health Status' | cut -f 2 -d ':' 1>/dev/null 2>/dev/null", path);
    return cmdresult;
}

char* badblockscmd(const char *path, char *cmdresult)
{
    sprintf(cmdresult,"badblocks %s -o /tmp/badblockinfo.txt 4096 2048 1>/dev/null 2>/dev/null", path);
    return cmdresult;
}


char* badblockstxt(char *cmdresult)
{
    sprintf(cmdresult,"touch /tmp/badblockinfo.txt 1>/dev/null 2>/dev/null");
    return cmdresult;
}

char* getsectorscmd(const char *path, char *cmdresult)
{
    sprintf(cmdresult,"fdisk -l | grep %s |grep 'Disk'| awk '{print$7}' 1>/dev/null 2>/dev/null", path);
    return cmdresult;
}
