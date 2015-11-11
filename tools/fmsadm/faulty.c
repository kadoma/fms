/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    faulty.c
 * Author:      Inspur OS Team
                wang.leibj@inspur.com
 * Date:        2015-08-21
 * Description: isplay list of faulty resources
 *
 ************************************************************/

#include <string.h>
#include <errno.h>
#include <limits.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <fmd_adm.h>
#include <stdlib.h>
#include "fmadm.h"
#include <tcutil.h>
#include <tchdb.h>
#include <myconf.h>

/*
 *
 */

#include <list.h>

typedef struct status_record {
    char *severity;
    char *msgid;
    char *class;
    char *resource;
    char *asru;
    char *fru;
    char *serial;
    uint64_t uuid;
    uint64_t rscid;
    uint8_t  state;
    uint32_t count;
    uint64_t create;
    struct list_head list;
} status_record_t;

/*
typedef struct {
    char* ptr;
    int size;
    int asize;
}TCXSTR;
*/

static struct list_head status_rec_list;

static int max_display;
static int max_fault = 0;
static int opt_f = 0;

#define CASE_DB "/usr/lib/fms/db/rep_case.db"
#define CPUMEM_DB "/usr/lib/fms/db/cpumem.db"
#define DISK_DB "/usr/lib/fms/db/disk.db"
//static int opt_g = 0;

//static fmd_msg_hdl_t *fmadm_msghdl = NULL; /* handle for libfmd_msg calls */

static void
usage(void)
{
    fputs("Usage: fmadm faulty [-sfvh] [-n <num>] [-t<type>]\n"
          "\t-h    help\n"
          "\t-s    show faulty fru's\n"
          "\t-f    show faulty only \n"
          "\t-n    number of fault records to display\n"
          "\t-v    full output\n"
          "\t-t    caselist type :repaired or happenning\n"
          , stderr);
}

void
format_date(char *buf, time_t secs)
{
    struct tm *tm = NULL;
    int year = 0, month = 0, day = 0;
    int hour = 0, min = 0, sec = 0;

    time(&secs);
    tm = gmtime(&secs);
    year = tm->tm_year + 1900;
    month = tm->tm_mon + 1;
    day = tm->tm_mday;
    hour = (tm->tm_hour + 8) % 24;
    min = tm->tm_min;
    sec = tm->tm_sec;
    sprintf(buf, "%d-%02d-%02d %02d:%02d:%02d\t", year, month, day, hour, min, sec);
}

static void
print_dict_info_line(char *msgid, TCHDB *hdb, const char *linehdr)
{
    char key[32];
    sprintf(key,"%s.%s",msgid,linehdr);
    char *cp = tchdbget2(hdb,key);
    if (cp != NULL)
        (void) printf("%s: %s\n", linehdr, cp);
    (void) printf("\n");

    free(cp);
}
static void
print_dict_info(status_record_t *srp)
{

        TCHDB *hdb = tchdbnew();
    (void) printf("\n");
    if(strstr(srp->class,"cpu") != NULL){
                if(!tchdbopen(hdb,CPUMEM_DB,HDBOREADER))
                wr_log("",WR_LOG_ERROR,"%s failed to open \n",CPUMEM_DB);
        }else {
                if(!tchdbopen(hdb,DISK_DB,HDBOREADER))
                wr_log("",WR_LOG_ERROR,"%s failed to open \n",DISK_DB);
        }

    print_dict_info_line(srp->msgid,hdb, "class");
    /*if (srp->asru) {
        (void) printf("affects : %s\n",srp->asru);
    }*/
    if (srp->fru) {
        (void) printf("FRU : %s\n", srp->fru);
    }
    (void) printf("\n");
    print_dict_info_line(srp->msgid,hdb, "description");
    print_dict_info_line(srp->msgid,hdb, "response");
    print_dict_info_line(srp->msgid,hdb, "impact");
    print_dict_info_line(srp->msgid,hdb, "action");
    (void) printf("\n");
    if(!tchdbclose(hdb))
        wr_log("",WR_LOG_ERROR,"hdb failed to close \n");
    free(hdb);

}

static void
print_status_record(status_record_t *srp, int opt_t)
{
    char buf[64];
    time_t sec = 0;
    char *head;
    head = "--------------------- "
        "--------------  "
        "---------------\n"
        "TIME"
        "                     ERROR-ID"
        "         SEVERITY\n--------------------- "
        "-------------- "
        " ---------------";
    printf("%s\n", head);

    /* Get the times */
    format_date(buf, sec);

    if (opt_f) {
        if (srp->state > FAF_CASE_CREATE)
            return;
        else {
            (void) printf("%s%s\t%s\n",
                buf, srp->msgid, srp->severity);

            print_dict_info(srp);
        }
    } else {
        (void) printf("%s%s\t%s\n",
            buf, srp->msgid, srp->severity);

        print_dict_info(srp);
    }
}

static status_record_t *
status_record_alloc(void)
{
    status_record_t *srp =
        (status_record_t *)malloc(sizeof (status_record_t));

    memset(srp, 0, sizeof (status_record_t));
    srp->severity = NULL;
    srp->msgid = NULL;
    srp->class = NULL;
    srp->resource = NULL;
    srp->asru = NULL;
    srp->fru = NULL;
    srp->serial = NULL;
    srp->uuid = (uint64_t)0;
    srp->rscid = (uint64_t)0;
    srp->state = (uint8_t)0;
    srp->count = (uint32_t)0;
    srp->create = (uint64_t)0;
    INIT_LIST_HEAD(&srp->list);

    return srp;
}

static void
print_catalog(fmd_adm_t *adm)
{
    struct list_head *pos = NULL;
    status_record_t *srp = NULL;
    int max = max_display;

    if (max == 0) {
        list_for_each(pos, &status_rec_list) {
            srp = list_entry(pos, status_record_t, list);

            (void) printf("\n");
            print_status_record(srp, opt_f);
        }
    } else if (max > 0) {
        list_for_each(pos, &status_rec_list) {
            srp = list_entry(pos, status_record_t, list);
            if (max > 0) {
                (void) printf("\n");
                print_status_record(srp, opt_f);
                max--;
            } else
                break;
        }
    }
}


static void
print_fru(fmd_adm_t *adm)
{
    struct list_head *pos = NULL;
    status_record_t *srp;
    char *head;
    head = "------------------  "
           "-------------------  "
           "-----------\n"
           "DEV NAME                 FAULT CLASS"
           "        COUNTS"
           "\n------------------  "
           "-------------------  "
           "-----------";
    printf("%s\n", head);

    list_for_each(pos, &status_rec_list) {
        srp = list_entry(pos, status_record_t, list);

        (void) printf("\n");
        (void) printf("%-20s%-20s \t%d\n",srp->fru,srp->class, srp->count);
    }
}

int fmd_adm_case_dbiter(){

    TCHDB *hdbcase = tchdbnew();
    TCHDB *hdbcpumem = tchdbnew();
    TCHDB *hdbdisk = tchdbnew();

    char * key = NULL;
    char * value = NULL;

    if(!tchdbopen(hdbcase,CASE_DB,HDBOREADER)){
        wr_log("",WR_LOG_ERROR,"%s failed to open \n",CASE_DB);
        return (-1);
    }

    if(!tchdbopen(hdbcpumem,CPUMEM_DB,HDBOREADER)){
        wr_log("",WR_LOG_ERROR,"%s failed to open \n",CPUMEM_DB);
        return (-1);
    }

    if(!tchdbopen(hdbdisk,DISK_DB,HDBOREADER)){
        wr_log("",WR_LOG_ERROR,"%s failed to open \n",DISK_DB);
        return(-1);
    }

    if(!tchdbiterinit(hdbcase)){
        wr_log("",WR_LOG_ERROR,"failed iterinit \n");
        return (-1);
    }

    while((key = tchdbiternext2(hdbcase))){

        char *tmp ,*tmp1,*tmp2,*tmp3,*tmp4,*count,*class;
        status_record_t *srp = status_record_alloc();

        srp->fru = strdup(key);
        value = tchdbget2(hdbcase,key);

        tmp= strtok(value,",");
        class = (strdup(strstr(tmp,":") + 1));
        srp->class = class;

        if(tmp != NULL)
        tmp1 = strtok(NULL,",");
        srp->create = atoi(strdup(strstr(tmp1,":") + 1));

        if(tmp1 != NULL)
           tmp2 = strtok(NULL,",");

        if(tmp2 != NULL)
           tmp3 = strtok(NULL,",");
        count = strdup(strstr(tmp3,":") + 1);

        tmp4 = strtok(count,".");
        srp->count = atoi(tmp4);

        if(strstr(srp->class,"cpu") != NULL){
            srp->msgid = tchdbget2(hdbcpumem,srp->class);
            char keys[32];
            sprintf(keys,"%s.severity",srp->msgid);
            srp->severity = tchdbget2(hdbcpumem,keys);
        }else {
            srp->msgid = tchdbget2(hdbdisk,srp->class);
            char keys[32];
            sprintf(keys,"%s.severity",srp->msgid);
            srp->severity = tchdbget2(hdbdisk,keys);
        }

        list_add(&srp->list, &status_rec_list);
    }

    if(!tchdbclose(hdbcase)){
        wr_log("",WR_LOG_ERROR,"hdbcase failed to close \n");
        return (-1);
    }
    free(hdbcase);

    if(!tchdbclose(hdbcpumem)){
        wr_log("",WR_LOG_ERROR,"hdbcpumem failed to close \n");
        return (-1);
    }
    free(hdbcpumem);

    if(!tchdbclose(hdbdisk)){
        wr_log("",WR_LOG_ERROR,"hdbdisk failed to close \n");
        return (-1);
    }
    free(hdbdisk);

    return 0;
}

static int
get_cases_from_fmd(fmd_adm_t *adm,char* type)
{
    if(strncmp(type,"repaired",8) == 0)
        return fmd_adm_case_dbiter();

    struct list_head *pos = NULL;
    fmd_adm_caseinfo_t *aci = NULL;
    int rt = FMADM_EXIT_SUCCESS;
    TCHDB *hdbcpumem = tchdbnew();
    TCHDB *hdbdisk = tchdbnew();

    if(!tchdbopen(hdbcpumem,CPUMEM_DB,HDBOREADER)){
        wr_log("",WR_LOG_ERROR,"%s failed to open \n",CPUMEM_DB);
        return (-1);
    }

    if(!tchdbopen(hdbdisk,DISK_DB,HDBOREADER)){
        wr_log("",WR_LOG_ERROR,"%s failed to open \n",CPUMEM_DB);
        return (-1);
   }
     /*
     * These calls may fail with Protocol error if message payload is to big
     */
    if (fmd_adm_case_iter(adm) != 0)
        die("FMD: failed to get case list from fmd");

    list_for_each(pos, &adm->cs_list) {
        aci = list_entry(pos, fmd_adm_caseinfo_t, aci_list);
        status_record_t *srp = status_record_alloc();

        if(aci->fafc.fafc_fault == NULL)
            continue;

        srp->fru = strtok(aci->fafc.fafc_fault,":");

        if(srp->fru)
            srp->class = strtok(NULL,":");

        if(srp->class == NULL){

            srp->class = strdup(srp->fru);
            char *tmp = strtok(srp->fru,".");
            if( tmp )
                srp->fru = strtok(NULL,".");
        }

        srp->asru = aci->aci_asru;
        srp->state = aci->fafc.fafc_state;
        srp->count = aci->fafc.fafc_count;
        srp->create = aci->fafc.fafc_create;

        if(strstr(srp->class,"cpu") != NULL){
            srp->msgid = tchdbget2(hdbcpumem,srp->class);
            char key[32];
            sprintf(key,"%s.severity",srp->msgid);
            srp->severity = tchdbget2(hdbcpumem,key);
        }else {
            srp->msgid = tchdbget2(hdbdisk,srp->class);
            char key[32];
            sprintf(key,"%s.severity",srp->msgid);
            srp->severity = tchdbget2(hdbdisk,key);
        }

        list_add(&srp->list, &status_rec_list);
    }

    if(!tchdbclose(hdbcpumem)){
        wr_log("",WR_LOG_ERROR,"hdbcpumem failed to close \n");
        return (-1);
    }
    free(hdbcpumem);

    if(!tchdbclose(hdbdisk)){
        wr_log("",WR_LOG_ERROR,"hdbdisk failed to close \n");
        return (-1);
    }
    free(hdbdisk);

    return (rt);
}

/*
 * fmadm faulty command
 *
 *    -h        help
 *    -n        number of fault records to display
 *    -u        print listed uuid's only
 *    -v        full output
 *
 */

int
cmd_faulty(fmd_adm_t *adm, int argc, char *argv[])
{
    int opt_s = 0, opt_v = 0, opt_t = 0;
    int rt, c;
    //char opt_type[32];
    char *opt_type;
    while ((c = getopt(argc, argv, "svfn:t:u:h?")) != EOF) {
        switch (c) {
        case 's':
            opt_s++;
            break;
        case 'v':
            opt_v++;
            break;
        case 'f':
            opt_v++;
            opt_f++;
            break;
        case 'n':
            max_fault = atoi(optarg);
            break;
        case 't':
            opt_t++;
            opt_type = optarg;
            break;
        case 'h':
        case '?':
            usage();
            return (FMADM_EXIT_SUCCESS);
        default:
            return (FMADM_EXIT_USAGE);
        }
    }
    if (argc == 1) {
        usage();
        return (FMADM_EXIT_SUCCESS);
    }

    INIT_LIST_HEAD(&status_rec_list);

    if(opt_t){
        if((strncmp(opt_type,"repaired",8) != 0) && (strncmp(opt_type,"happenning",10)!=0)){
           printf("\t-t    caselist type :repaired or happenning\n");
        }
    }else{
        opt_type = malloc(16 * sizeof(char));
        sprintf(opt_type,"repaired");
    }
#if 0
    char    fmd_thread[8];
    FILE * fp ;
    fp = popen("ps -aux | grep fmd |wc -l","r");
    fgets(fmd_thread,sizeof(fmd_thread),fp);
    if(strncmp(fmd_thread,"3",1) != 0)
        printf("fmd does not start ,exe command :'fmd start'\n");
    pclose(fp);
#endif
    rt = get_cases_from_fmd(adm,opt_type);

    if (opt_s)
        print_fru(adm);
    max_display = max_fault;
    if (opt_v)
        print_catalog(adm);

    return (rt);
}
