#include<stdio.h>
#include<unistd.h>
#include<tchdb.h>
#include<string.h>
int main(){
	TCHDB *hdb;
	int ecode;
	hdb = tchdbnew();
	if(!tchdbopen(hdb,"mydict.db",HDBOREADER)){
		ecode = tchdbecode(hdb);
		fprintf(stderr,"open error:%s\n",tchdberrmsg(ecode));
	}
	char *value;
	value = tchdbget2(hdb,"GMCA-TLB-04.description");
	if(value){
		printf("%s\n",value);
		free(value);
	}else{
		ecode = tchdbecode(hdb);
		fprintf(stderr,"get error:%s\n",tchdberrmsg(ecode));
	}
	if(!tchdbclose(hdb)){
		ecode = tchdbecode(hdb);
                fprintf(stderr,"close error:%s\n",tchdberrmsg(ecode));
	}
	tchdbdel(hdb);
	return 0;
}
