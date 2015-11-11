/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    fm_cmd.c
 * Author:      Inspur OS Team 
                	guomeisi@inspur.com
 * Date:        2015-08-04
 * Description: define the command of parsing disk.
 *
 ************************************************************/


#include <stdio.h>
#include <string.h>

char* disknamecmd(char *cmdresult){
		sprintf(cmdresult,"df -P |  awk '{print $1}' | grep /dev 1>/dev/null 2>/dev/null");
        return cmdresult;
}
char* smartworkcmd(const char *path, char *cmdresult){
		sprintf(cmdresult,"disktool -i %s | grep 'SMART support is' | cut -f 2 -d ':' 1>/dev/null 2>/dev/null", path);
        return cmdresult;
}
char* smartsupportcmd(const char *path, char *cmdresult){
		sprintf(cmdresult,"disktool -i %s | grep 'Device supports SMART and is Enabled' 1>/dev/null 2>/dev/null", path);
        return cmdresult;
}
char* smartoncmd(const char *path, char *cmdresult){
		sprintf(cmdresult,"disktool -s on %s 1>/dev/null 2>/dev/null",path);
		return cmdresult;
}
char* smarttempcmd(const char *path, char *cmdresult){
		sprintf(cmdresult,"disktool -A %s | grep 'Temperature' | cut -f 2 -d ':'| cut -f 1 -d 'C' 1>/dev/null 2>/dev/null", path);
		return cmdresult;
}
char* smarthealthycmd(const char *path,char *cmdresult){
	 sprintf(cmdresult,"disktool -H %s | grep 'overall-health' | cut -f 2 -d ':' 1>/dev/null 2>/dev/null", path);
     return cmdresult;
}
char* smartokcmd(const char *path,char *cmdresult){
	 sprintf(cmdresult,"disktool -H %s | grep 'Health Status' | cut -f 2 -d ':' 1>/dev/null 2>/dev/null", path);
     return cmdresult;
}
char* badblockscmd(const char *path, char *cmdresult){
	sprintf(cmdresult,"badblocks %s -o /tmp/badblockinfo.txt 4096 2048 1>/dev/null 2>/dev/null", path);
	return cmdresult;
}
char* badblockstxt(char *cmdresult){
	sprintf(cmdresult,"touch /tmp/badblockinfo.txt 1>/dev/null 2>/dev/null");
	return cmdresult;
}
char* getsectorscmd(const char *path, char *cmdresult){
	sprintf(cmdresult,"fdisk -l | grep %s |grep 'Disk'| awk '{print$7}' 1>/dev/null 2>/dev/null", path);
	return cmdresult;
}