#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<tchdb.h>
#include"tcutil.h"
#include<fcntl.h>
char* getstr(char[],char[]);
int main(int argc, char *argv[]){
	if(argc != 3){
		printf("usage :./create [input file] [dictname]\n ");
		exit(-1);
	}
	FILE *fd_po;
	int ecode;
	TCHDB *hdb;
	char key[BUFSIZ],value[BUFSIZ];
	hdb = tchdbnew();
	if(!tchdbopen(hdb,argv[2],HDBOWRITER|HDBOCREAT)){
		ecode = tchdbecode(hdb);
		fprintf(stderr,"open error:%s\n",tchdberrmsg(ecode));
	}
	fd_po = fopen(argv[1],"r");
	if(fd_po == NULL){
		printf("open .po file failed\n");
		return -1;
	}
	char str[BUFSIZ];
	while(!feof(fd_po)){
		fgets(str,BUFSIZ,fd_po);
		if(strncmp(str,"msgid",5) == 0)
		{	strcpy(key,getstr(str,key));
			fgets(str,BUFSIZ,fd_po);
			strcpy(value,getstr(str,value));
			if(!tchdbput2(hdb,key,value)){
				ecode = tchdbecode(hdb);
				fprintf(stderr,"put error:%s\n",tchdberrmsg(ecode));
			}
		}
		
	}
	fclose(fd_po);
	if(!tchdbclose(hdb)){
		ecode = tchdbecode(hdb);
                fprintf(stderr,"close error:%s\n",tchdberrmsg(ecode));
	}
	tchdbdel(hdb);
	return 0;
}
char*  getstr(char buf[],char des[]){
	char *p = malloc( sizeof(char)*BUFSIZ);
	p = strtok(buf,"\"");
	if(p)
		p = strtok(NULL,"\"");
	return p;
}
