#include <stdio.h>
#include <string.h>

char* disknamecmd(char *cmdresult){
		sprintf(cmdresult,"df -P |  awk '{print $1}' | grep /dev");
        return cmdresult;
}
char* smartworkcmd(const char *path, char *cmdresult){
		sprintf(cmdresult,"smartctl -i %s | grep 'SMART support is' | cut -f 2 -d ':'", path);
        return cmdresult;
}
char* smartsupportcmd(const char *path, char *cmdresult){
		sprintf(cmdresult,"smartctl -i %s | grep 'Device supports SMART and is Enabled'", path);
        return cmdresult;
}
char* smarttempcmd(const char *path, char *cmdresult){
		sprintf(cmdresult,"smartctl -A %s | grep 'Temperature' | cut -f 2 -d ':'| cut -f 1 -d 'C'", path);
		return cmdresult;
}
char* smarthealthycmd(const char *path,char *cmdresult){
	 sprintf(cmdresult,"smartctl -H %s | grep 'overall-health' | cut -f 2 -d ':'", path);
     return cmdresult;
}
char* smartokcmd(const char *path,char *cmdresult){
	 sprintf(cmdresult,"smartctl -H %s | grep 'Health Status' | cut -f 2 -d ':'", path);
     return cmdresult;
}
char* badblockscmd(const char *path, char *cmdresult){
	sprintf(cmdresult,"badblocks %s -o /tmp/badblockinfo.txt", path);
	return cmdresult;
}

char* getsectorscmd(const char *path, char *cmdresult){
	sprintf(cmdresult,"fdisk -l | grep %s |grep Disk| awk '{print$7}'", path);
	return cmdresult;
}