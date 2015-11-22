/****************************************************************
 * Copyrught(C) inspur inc. <http://www.inspur.com>
 * Author:
 * Date:
 * Description: the test function
 *
****************************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<tcutil.h>
#include<tchdb.h>

/*  global defs */
static const char *g_name;

/*  print the way to use the function   */
static void usage(void);

/*  create a database file  */
static int create_dict(int argc,char *argv[]);

/*  put data(key-value pair) into the database file */
static int put_dict(int argc,char* argv[]);

/*  delete a record of a database file  */
static int out_dict(int argc,char *argv[]);

/*  retrive a record in a database file */
static int get_dict(int argc,char *argv[]);

/*  list all of the database file record    */
static int list_dict(int argc,char *argv[]);

/*  perform put command */
static int proc_put(const char *path,const char *kbuf,int ksiz,const char *vbuf,
        int vsiz,int mode);

/*  perform out command */
static int proc_out(const char *path,const char *kbuf,int ksiz,int omode);

/*  perform get command */
static int proc_get(const char *path,const char *kbuf,int ksiz,int omode);

/*  perform list command    */
static int proc_list(const char *path,int omode);

/*  calculate the total number of record in a database file */
static int total_dict(int argc,char *argv[]);

int main(int argc,char *argv[]){
    g_name = argv[0];
    int rv = 0;
    if(argc < 2) usage();
    if(!strcmp(argv[1],"create")){
        rv = create_dict(argc,argv);
    }else if(!strcmp(argv[1],"put")){
        rv = put_dict(argc,argv);
    }else if(!strcmp(argv[1],"out")){
        rv = out_dict(argc,argv);
    }else if(!strcmp(argv[1],"get")){
        rv = get_dict(argc,argv);
    }else if(!strcmp(argv[1],"list")){
        rv = list_dict(argc,argv);
    }else if(!strcmp(argv[1],"total"))
        rv = total_dict(argc,argv);
    else
        usage();
    return rv;
}

static void usage(void){
    fprintf(stderr,"%s: the command line utility of the hash database API\n",g_name);
    fprintf(stderr,"\n");
    fprintf(stderr,"usage:\n");
    fprintf(stderr,"  %s  create dict_name\n",g_name);
    fprintf(stderr,"  %s  put dict_name key value\n",g_name);
    fprintf(stderr,"  %s  out dict_name key\n",g_name);
    fprintf(stderr,"  %s  get dict_name key\n",g_name);
    fprintf(stderr,"  %s  list dict_name\n",g_name);
    fprintf(stderr,"  %s  total dict_name",g_name);
    fprintf(stderr,"\n");
    exit(1);
}
static int create_dict(int argc,char *argv[]){
    char *path = argv[2];
    if(!path)   usage();
    int ecode;
    TCHDB *hdb = tchdbnew();
    if(!tchdbopen(hdb,path,HDBOWRITER|HDBOCREAT|HDBOTRUNC)){
        ecode = tchdbecode(hdb);
        fprintf(stderr,"%s: %s: %d: %s\n",g_name,path,ecode,tchdberrmsg(ecode));
        tchdbdel(hdb);
        return 1;
    }
    bool err = false;
    if(!tchdbclose(hdb)){
        ecode = tchdbecode(hdb);
        fprintf(stderr,"%s: %s: %d: %s\n",g_name,path,ecode,tchdberrmsg(ecode));
        err = true;
    }
    tchdbdel(hdb);
    return err ? 1 : 0;
}
static int put_dict(int argc,char *argv[]){
    char *path = NULL;
    char *key = NULL;
    char *value = NULL;
    int omode = 0;
    int i;
    for(i=2;i<argc;i++){
        if(!path)
            path = argv[i];
        else if(!key)
            key = argv[i];
        else if(!value)
            value = argv[i];
        else
            usage();
    }
    if(!path || !key || !value)
        usage();
    int ksiz,vsiz;
    char *kbuf,*vbuf;
    ksiz = strlen(key);
    vsiz = strlen(value);
    kbuf = tcmalloc(ksiz + 1);
    memcpy(kbuf,key,ksiz);
    kbuf[ksiz] = '\0';
    vbuf = tcmalloc(vsiz + 1);
    memcpy(vbuf,value,vsiz);
    vbuf[vsiz] = '\0';
    int rv = proc_put(path,kbuf,ksiz,vbuf,vsiz,omode);
    free(kbuf);
    free(vbuf);
    return rv;
}
static int proc_put(const char *path,const char *kbuf,int ksiz,const char *vbuf,
        int vsiz,int omode){
    TCHDB *hdb = tchdbnew();
    int ecode;
    if(!tchdbopen(hdb,path,HDBOWRITER|omode)){
        ecode = tchdbecode(hdb);
        fprintf(stderr,"%s: %s: %d: %s\n",g_name,path,ecode,tchdberrmsg(ecode));
        tchdbdel(hdb);
        return 1;
    }
    bool err = false;
    if(!tchdbput(hdb,kbuf,ksiz,vbuf,vsiz)){
        ecode = tchdbecode(hdb);
        fprintf(stderr,"%s: %s: %d: %s\n",g_name,path,ecode,tchdberrmsg(ecode));
        err = true;
    }
    if(!tchdbclose(hdb)){
        if(!err){
            ecode = tchdbecode(hdb);
            fprintf(stderr,"%s: %s: %d: %s\n",g_name,path,ecode,tchdberrmsg(ecode));
        }
        err = true;
    }
    tchdbdel(hdb);
    return err ? 1 : 0;
}
static int out_dict(int argc,char *argv[]){
    char *path = NULL;
    char *key = NULL;
    int omode = 0;
    int i;
    for(i = 2;i < argc;i++){
        if(!path)
            path = argv[i];
        else if(!key)
            key = argv[i];
        else
            usage();
    }
    if(!path||!key)
        usage();
    int ksiz = strlen(key);
    char *kbuf = tcmalloc(ksiz + 1);
    memcpy(kbuf,key,ksiz);
    kbuf[ksiz] = '\0';
    int rv= proc_out(path,kbuf,ksiz,omode);
    free(kbuf);
    return rv;
}
static int proc_out(const char *path,const char *kbuf,int ksiz,int omode){
    TCHDB *hdb = tchdbnew();
    int ecode;
    if(!tchdbopen(hdb,path,HDBOWRITER|omode)){
        ecode = tchdbecode(hdb);
        fprintf(stderr,"%s: %s: %d: %s\n",g_name,path,ecode,tchdberrmsg(ecode));
        tchdbdel(hdb);
        return 1;
    }
    bool err = false;
    if(!tchdbout(hdb,kbuf,ksiz)){
        ecode = tchdbecode(hdb);
        fprintf(stderr,"%s: %s: %d: %s\n",g_name,path,ecode,tchdberrmsg(ecode));
        err = true;
    }
    if(!tchdbclose(hdb)){
        if(!err){
            ecode = tchdbecode(hdb);
            fprintf(stderr,"%s: %s: %d: %s\n",g_name,path,ecode,tchdberrmsg(ecode));
        }
        err = true;
    }
    tchdbdel(hdb);
    return err ? 1 : 0;
}
static int get_dict(int argc,char *argv[]){
    char *path = NULL;
    char *key = NULL;
    int omode = 0;
    int i;
    for(i = 2;i < argc;i++){
        if(!path)
            path = argv[i];
        else if(!key)
            key = argv[i];
        else
            usage();
    }
    if(!path||!key)
        usage();
    int ksiz = strlen(key);
    char *kbuf = tcmalloc(ksiz + 1);
    memcpy(kbuf,key,ksiz);
    kbuf[ksiz] = '\0';
    int rv= proc_get(path,kbuf,ksiz,omode);
    return rv;
}
static int proc_get(const char *path,const char *kbuf,int ksiz,int omode){
    TCHDB *hdb = tchdbnew();
    int ecode;
    if(!tchdbopen(hdb,path,HDBOWRITER|HDBOREADER|omode)){
        ecode = tchdbecode(hdb);
        fprintf(stderr,"%s: %s: %d: %s\n",g_name,path,ecode,tchdberrmsg(ecode));
        tchdbdel(hdb);
        return 1;
    }
    bool err = false;
    int vsiz;
    char *vbuf = tchdbget(hdb,kbuf,ksiz,&vsiz);
    if(vbuf){
        printf("%s\n",vbuf);
        free(vbuf);
    }else{
        ecode = tchdbecode(hdb);
        fprintf(stderr,"%s: %s: %d: %s\n",g_name,path,ecode,tchdberrmsg(ecode));
        err = true;
    }
    if(!tchdbclose(hdb)){
        if(!err){
            ecode = tchdbecode(hdb);
            fprintf(stderr,"%s: %s: %d: %s\n",g_name,path,ecode,tchdberrmsg(ecode));
        }
        err = true;
    }
    tchdbdel(hdb);
    return err ? 1 : 0;
}
static int list_dict(int argc,char *argv[]){
    char *path = NULL;
    int omode = 0;
    int i;
    for(i = 2;i< argc;i++){
        if(!path)
            path = argv[i];
        else
            usage();
    }
    if(!path)
        usage();
    int rv = proc_list(path,omode);
    return rv;
}
static int proc_list(const char *path,int omode){
    TCHDB *hdb = tchdbnew();
    int ecode;
    if(!tchdbopen(hdb,path,HDBOWRITER|HDBOREADER|omode)){
        ecode = tchdbecode(hdb);
        fprintf(stderr,"%s: %s: %d: %s\n",g_name,path,ecode,tchdberrmsg(ecode));
        tchdbdel(hdb);
        return 1;
    }
    bool err = false;
    if(!tchdbiterinit(hdb)){
        ecode = tchdbecode(hdb);
        fprintf(stderr,"%s: %s: %d: %s\n",g_name,path,ecode,tchdberrmsg(ecode));
        err = true;
    }
    char *key,*value;
    while((key = tchdbiternext2(hdb))!=NULL){
        value = tchdbget2(hdb,key);
        if(value){
            printf("%s:\t%s\n",key,value);
            free(value);
        }
    }
    if(!tchdbclose(hdb)){
        if(!err){
            ecode = tchdbecode(hdb);
            fprintf(stderr,"%s: %s: %d: %s\n",g_name,path,ecode,tchdberrmsg(ecode));
        }
        err = true;
    }
    tchdbdel(hdb);
    return err ? 1 : 0;
}
static int total_dict(int argc,char *argv[]){
    char *path = NULL;
    int i;
    for(i = 2;i< argc;i++){
        if(!path)
            path = argv[i];
        else
            usage();
    }
    if(!path)
        usage();
    long count;
    TCHDB *hdb = tchdbnew();
    int ecode;
    if(!tchdbopen(hdb,path,HDBOREADER)){
        ecode = tchdbecode(hdb);
        fprintf(stderr,"%s: %s: %d: %s\n",g_name,path,ecode,tchdberrmsg(ecode));
        tchdbdel(hdb);
        return 1;
    }
    printf("%s total have %lld data\n",path,tchdbrnum(hdb));
    return 1;
}
