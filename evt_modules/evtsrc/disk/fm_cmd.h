#ifndef __FM_CMD_H
#define __FM_CMD_H

char* disknamecmd();
char* smartworkcmd(const char *path, char *cmdresult);
char* smartsupportcmd(const char *path, char *cmdresult);
char* smartoncmd(const char *path, char *cmdresult);

void smarttempcmd(const char *path, char *cmdresult);

char* smarthealthycmd(const char *path,char *cmdresult);
char* smartokcmd(const char *path,char *cmdresult);
char* badblockscmd(const char *path, char *cmdresult);
char* badblockstxt(char *cmdresult);
char* getsectorscmd(const char *path, char *cmdresult);
#endif