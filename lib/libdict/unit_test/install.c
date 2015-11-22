#include<stdio.h>
#include<tchdb.h>
#include<string.h>
#include<stdlib.h>
int main(int argc,char *argv[]){
if(argc != 2){
    fprintf(stderr,"usage: %s dict_name\n",argv[0]);
    exit(-1);
}
TCHDB *hdb;
int ecode;
hdb = tchdbnew();
if(!tchdbopen(hdb,argv[1],HDBOREADER|HDBOWRITER)){
    ecode = tchdbecode(hdb);
    fprintf(stderr,"open error:%s\n",tchdberrmsg(ecode));
}
char key[16],value[16];
int i;
for(i = 0;i < 1000000;i++){
    sprintf(key,"%d",i);
    sprintf(value,"%d",i);
    tchdbput2(hdb,key,value);
}
if(!tchdbclose(hdb)){
    ecode = tchdbecode(hdb);
    fprintf(stderr,"close error:%s\n",tchdberrmsg(ecode));
}
tchdbdel(hdb);
return 0;
}

