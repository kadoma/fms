#ifndef __FM_CMD_H
#define __FM_CMD_H

char* disknamecmd(char *cmdresult);
char* smartworkcmd(const char *path, char *cmdresult);
char* smartsupportcmd(const char *path, char *cmdresult);
char* smarttempcmd(const char *path, char *cmdresult);
char* smarthealthycmd(const char *path,char *cmdresult);
char* smartokcmd(const char *path,char *cmdresult);
char* badblockscmd(const char *path, char *cmdresult);
char* getsectorscmd(const char *path, char *cmdresult);
#endif