#include<stdio.h>
#include<unistd.h>
#include<tchdb.h>
#include<string.h>
int main(int argc,char *argv[]){
    if(argc != 2){
        printf("usage:./read_dict [db_name]\n");
        exit(1);
    }
	TCHDB *hdb;
	int ecode;
	hdb = tchdbnew();
	if(!tchdbopen(hdb,argv[1],HDBOREADER)){
		ecode = tchdbecode(hdb);
		fprintf(stderr,"open error:%s\n",tchdberrmsg(ecode));
	}
	char *value,*key;
	value = tchdbget2(hdb,"ereport.disk.unhealthy");
	if(value){
		printf("%s\n",value);
		free(value);
	}else{
		ecode = tchdbecode(hdb);
		fprintf(stderr,"get error:%s\n",tchdberrmsg(ecode));
	}
    tchdbiterinit(hdb);
    while((key=tchdbiternext2(hdb)) != NULL){
        value = tchdbget2(hdb,key);
        if(value){
            printf("%s:%s\n",key,value);
            free(value);
        }
    }
	if(!tchdbclose(hdb)){
		ecode = tchdbecode(hdb);
                fprintf(stderr,"close error:%s\n",tchdberrmsg(ecode));
	}
	tchdbdel(hdb);
	return 0;
}
