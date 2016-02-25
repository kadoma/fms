/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    config.c
 * Author:      Inspur OS Team
                wang.leibj@inspur.com
 * Date:        2015-08-21
 * Description: isplay or set fault manager configuration
 *
 ************************************************************/

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include "fmadm.h"
#include <fmd_adm.h>

static int opt_l = 0, opt_s = 0, opt_m = 0;
static int opt_i = 0, opt_b = 0;
static int evtsrc = 0, agent = 0;

static char arg_module[256];
static char arg_mode[256];
static char arg_interval[256];
static char mode[256];
static void
usage(void)
{
    fputs("Usage: fmadm config [-l|s] [-i <interval>] [-m <auto> | <manual> | <define>]"
          "[-b] [module]\n"
          "\t-h    help\n"
          "\t-l    list all the module configuration files\n"
          "\t-s    show the module's configuration\n"
          "\t-i    set the interval value for evtsrc module\n"
          "\t-b    set the subscribing for agent module\n\n"
          "\t-m    set the agent module fault-tolerant mode\n"
          "<interval> can be:\n"
          "\tFor example: 1\n"
          , stderr);
}
static void
usage_b(void)
{
    printf(
            "\t(enter 'quit' to quit edit. note : don't use 'ctr+c' to quit edit !)\n"
	        "\t(press Enter key to cat the next subscribe.)\n"
            "\t(enter 'add' to add a new subscribe.)\n"
            "\t(for example: add subscribe:event.cpu.unclassified_ue.)\n"
            "\t(enter 'del' to delete a subscribe.)\n\n");
}

static void
fmd_modname_print(char *modname)
{
    char buf[50];
    char c = '.';
    char *p = NULL;
    int len = 0;

    memset(buf, 0, 50);
    p = strrchr(modname, c);
    len = p - modname;

    strncpy(buf, modname, len);
    printf("%s\n", buf);
}

static int
fmd_list_mod(void)
{
    DIR *dirp;
    struct dirent *dp;
    const char *p;
    char path[] = "/usr/lib/fms/plugins/";

    if ((dirp = opendir(path)) == NULL)
        return (-1); /* failed to open directory; just skip it */

    while ((dp = readdir(dirp)) != NULL) {
        if (dp->d_name[0] == '.')
            continue; /* skip "." and ".." */

        p = dp->d_name;
        /* check name */
        assert(p != NULL);

        if ((strstr(dp->d_name, "_agent") != NULL)
         && (strstr(dp->d_name, ".conf") != NULL))
            fmd_modname_print(dp->d_name);
        if ((strstr(dp->d_name, "_src") != NULL)
         && (strstr(dp->d_name, ".conf") != NULL))
            fmd_modname_print(dp->d_name);
    }

    (void) closedir(dirp);
    return (0);
}

static int
fmd_add_agent_conf(char *mod_name)
{
    FILE *fp2 = NULL,*fp1 = NULL ;
    char dir[] = "/usr/lib/fms/plugins/";
    char filename[50];
    char buf[256];
    char buff[256];
    char buffer[PATH_MAX];
    int ready = 0,std = 1;

    memset(filename, 0, 50 * sizeof (char));
    memset(buf, 0, 256 * sizeof (char));
    memset(buffer, 0, PATH_MAX * sizeof (char));


    sprintf(filename, "%s%s.conf", dir, mod_name);
    if (strstr(mod_name, "agent") == NULL){
        usage();
        return (-1);
    }

    if ((fp1 = fopen(filename, "r+")) == NULL) {
        printf("module: %s does not exist!\n",mod_name);
        wr_log("",WR_LOG_ERROR,"fmsadm config:failed to open conf file for %s\n",mod_name);
        return (-1);
    }

    usage_b();
    while (fgets(buf, sizeof(buf), fp1)) {
        if(ready == 0){
            if(strstr(buf,"subscribe") == NULL)
                continue;

            printf("%s\n",buf);
            while (std){
                if(fgets(buff,sizeof(buff),stdin)){
                    if(strncmp(buff,"add",3) == 0 ){
                        if(check_esc(buff,mod_name) == 0){
                            if(check_sub_rep(buff,mod_name) == 0){
                                sprintf(buffer,"%s%s",buffer,buf);
                                char * tmp = strstr(buff,":");
                                sprintf(buffer,"%ssubscribe %s",buffer, tmp+1);
                                break;
                            }else{
                                printf("%s\n",buf);
                                continue;
                            }
                        }else{
                            usage_b();
                            printf("%s\n",buf);
                            continue;
                            //break;
                        }
                    }else if(strncmp(buff,"del",3) == 0 && strlen(buff) == 4){
                        break;
                    }else if(strncmp(buff,"quit",4) == 0 && strlen(buff) == 5){
                        ready = 1;
                        sprintf(buffer,"%s%s",buffer,buf);
                        break;
                    }else if (strncmp(buff,"\n",1) == 0) {
                        sprintf(buffer,"%s%s",buffer,buf);
                        break;
                    }else{
                        printf("Your enter illegal !\n");
                        usage_b();
                        printf("%s\n",buf);
                        continue;
                    }
                }else{
                    break;
                }
                continue;
            }
        }else{
            sprintf(buffer,"%s%s",buffer,buf);
            continue;
        }
    }

    if ((fp2 = fopen(filename, "w+")) == NULL) {
        wr_log("",WR_LOG_ERROR,"fmsadm config:failed to open conf file for %s\n",mod_name);
		fclose(fp1);
        return (-1);
    }

    fputs(buffer, fp2);

    fclose(fp1);
    fclose(fp2);

    return 0;
}

/*
 *check subscribe if exist in module conf
 *
 */
int check_esc(char *sub ,char *mod_name){

    FILE *fp = NULL;
    char escdir[] = "/usr/lib/fms/escdir/";

    char escfile[50];
    char escbuf[256];
    char buffer[PATH_MAX];
    int exist = 0;

    memset(escfile, 0, 50 * sizeof (char));
    memset(escbuf, 0, 256 * sizeof (char));
    memset(buffer, 0, PATH_MAX * sizeof (char));

    char *tmp =  NULL,*tmp1 = NULL;
    tmp = strdup(mod_name);
    tmp1 = strtok(tmp,"_");
    sprintf(escfile, "%s%s.esc", escdir, tmp1);

    /* open esc file */
    if ((fp = fopen(escfile, "r+")) == NULL) {
        printf("file: %s does not exist!\n",escfile);
        wr_log("",WR_LOG_ERROR,"fmsadm config:failed to open conf file for %s\n",mod_name);
        return (-1);
    }

    /*check subscribe head */
    char *subhead = strstr(sub,"subscribe:");
    if(subhead == NULL)
    {
        return (-1);
    }

    int addtmp = strlen(sub) - strlen(subhead) - 3;
    char *tmpadd = sub+3;
    while (addtmp > 0)
    {
        if (*tmpadd == ' '){
            tmpadd++;
            addtmp--;
            continue;
        }
        else
            return(-1);
    }

    if(strncmp(subhead + 10,"event",5) != 0)
    {
        printf("subscribe illegal ! subscribe header must be 'event'!\n");
        return (-1);
    }
    /* check subscribe legal*/
    char * subclass = strstr(sub,".");
    if(check_sub_legal(subclass) != 0){
        printf("subscribe illegal ! subscribe must in %s file \n",escfile);
        return (-1);
    }

    /* check subscribe if exist in esc file  */
    while (fgets(escbuf, sizeof(escbuf), fp)) {
        if(escbuf == NULL)
            continue;

        char * escclass = strstr(escbuf,".");

        if(escclass == NULL)
            continue;

        int len = strlen(escclass);
        if(strstr(sub,"*") != NULL);
            len = strlen(subclass) - 3;

        if(strncmp(subclass,escclass,len) == 0){
            exist = 1;
        }
        continue;
    }

    if(exist)
        return 0;
    else{
        printf("subscribe illegal ! subscribe must in %s file \n",escfile);
        return (-1);
    }
}

/*
 *
 *
*/
int check_sub_legal(const char *sub)
{

    if(sub == NULL)
        return (-1);
    size_t len = strlen(sub);
    while(len > 0) {
        if (*sub == ' ' ) {
            return (-1);
        }
        sub++;
        len--;
    }
        return 0;
}

/*
 *check subscribe if exist in module conf
 *
 */
int check_sub_rep(char *sub ,char *mod_name){

    FILE *fp1 = NULL;
    char dir[] = "/usr/lib/fms/plugins/";

    char modfile[50];
    char buf[256];
    char buff[256];
    char buffer[PATH_MAX];
    int exist = 0;


    memset(modfile, 0, 50 * sizeof (char));
    memset(buf, 0, 256 * sizeof (char));
    memset(buffer, 0, PATH_MAX * sizeof (char));

    sprintf(modfile, "%s%s.conf", dir, mod_name);

    /* open module file */
    if ((fp1 = fopen(modfile, "r+")) == NULL) {
        printf("\nfile: %s does not exist!\n",modfile);
        wr_log("",WR_LOG_ERROR,"fmsadm config:failed to open conf file for %s\n",mod_name);
        return (-1);
    }

    char *tmp = strstr(sub,"subscribe:");
    if(tmp == NULL)
        return (-1);

    /* check subscribe if exist in module conf */
    while (fgets(buf, sizeof(buf), fp1)) {

        char *rep = strstr(buf,"subscribe");
        if (rep == NULL)
            continue;

        int sublen = strlen(tmp+10);
        int buflen = strlen(rep+10);
        int cmplen = 0;
        if(strstr(tmp,"*"))
            sublen =  sublen -3;
        if(strstr(rep,"*"))
            buflen = buflen -3;
        if(sublen > buflen )
            cmplen = buflen;
        else
            cmplen = sublen;
        //printf("%d:%d:%d",sublen,buflen,cmplen);
        //printf("%s\n%s\n",tmp+11,rep+11);
        if(strncmp(tmp+10,rep+10,cmplen) == 0){
            printf("\n%s has already have subscribe: %s \n",mod_name,rep+10);
            return (-1);
        }
        continue;
    }
    return 0;

}



static int
fmd_add_src_conf(char *mod_name)
{
    FILE *fp2 = NULL,*fp1 = NULL;
    char dir[] = "/usr/lib/fms/plugins/";
    char filename[50];
    char buf[256];
    char buff[256];
    char buffer[PATH_MAX];
    int ready = 0;

    memset(filename, 0, 50 * sizeof (char));
    memset(buf, 0, 256 * sizeof (char));
    memset(buffer, 0, PATH_MAX * sizeof (char));

    sprintf(filename, "%s%s.conf", dir, mod_name);
    if (strstr(mod_name, "src") == NULL){
        usage();
        return (-1);
    }

    if ((fp1 = fopen(filename, "r+")) == NULL) {
        printf("\nmodule: %s does not exist!\n",mod_name);
        wr_log("",WR_LOG_ERROR,"fmsadm config:failed to open conf file for %s\n",mod_name);
        return (-1);
    }

    while (fgets(buf, sizeof(buf), fp1)) {
        if (strncmp(buf, "interval ", 9) == 0) {
            if (is_digit(arg_interval) == 0){
                sprintf(buffer, "%sinterval %s\n", buffer, arg_interval);
            }else{
                sprintf(buffer, "%s%s", buffer, buf);
                printf("\n%s is illegal , interval must be number and not 0 !\n",arg_interval);
            }
            continue;
        } else{
            sprintf(buffer, "%s%s", buffer, buf);
        }
    }

    if ((fp2 = fopen(filename, "w+")) == NULL) {
        wr_log("",WR_LOG_ERROR,"fmsadm config:failed to open conf file for %s\n",mod_name);
        fclose(fp1);
        return (-1);
    }

    fputs(buffer, fp2);

    fclose(fp1);
    fclose(fp2);

return 0;

}

int is_digit(const char *str)
{
    size_t len = strlen(str);
    int nozore = 0;
    while(len > 0) {
        if (*str < '0' || *str > '9') {
            return (-1);
        }else
        {
            if( *str != '0')
                nozore = 1;
        }
        str++;
        len--;
    }
    if(nozore == 1)
        return 0;
    else
        return (-1);
}

static int
fmd_show_conf(char *mod_name)
{
    FILE *fp = NULL,*fp1 = NULL;
    char dir[] = "/usr/lib/fms/plugins/";
    char filename[50];
    char buf[256];
    char buffer[PATH_MAX];

    memset(filename, 0, 50 * sizeof (char));
    memset(buf, 0, 256 * sizeof (char));
    memset(buffer, 0, PATH_MAX * sizeof (char));

    if (strstr(mod_name, "src") != NULL)
        evtsrc = 1;
    else if (strstr(mod_name, "agent") != NULL)
        agent = 1;

    sprintf(filename, "%s%s.conf", dir, mod_name);
    if ((fp = fopen(filename, "r+")) == NULL) {
        printf("\n%s does not exist!\n",mod_name);
        wr_log("",WR_LOG_ERROR,"fmsadm config:failed to open conf file for %s\n",mod_name);
        return (-1);
    }

    while (fgets(buffer, sizeof(buffer), fp)) {
        if ((buffer[0] == '\n') || (buffer[0] == '\0')
                || (buffer[0] == '#'))
            continue;        /* skip blank line and '#' line*/
        if (evtsrc == 1) {
            if (strncmp(buffer, "interval ", 9) == 0) {
                printf("%s\t\t%s", mod_name, buffer);
            }
            continue;
        } else if (agent == 1) {
            if (strncmp(buffer, "subscribe ", 10) == 0) {
                printf("%s\t\t%s", mod_name, buffer);
                continue;
            }
            continue;
        }
    }
    fclose(fp);
    return (0);
}

/**/
int config_module(fmd_adm_t *adm,char* module,char* mode)
{
    int ret = fmd_adm_config_module(adm,module,mode);
    return ret;
}
/*
 * fmadm config command
 */

int
cmd_config(fmd_adm_t *adm, int argc, char *argv[])
{
    int rt, c;

    memset(arg_module, 0, 256 * sizeof (char));
    memset(arg_interval, 0, 256 * sizeof (char));

    while ((c = getopt(argc, argv, "ls:i:m:bh?")) != EOF) {
        switch (c) {
        case 'l':
            opt_l++;
            break;
        case 's':
            opt_s++;
            strcpy(arg_module, optarg);
            break;
        case 'i':
            opt_i++;
            strcpy(arg_interval, optarg);
            break;
        case 'b':
            opt_b++;
            strcpy(arg_mode, optarg);
            break;
		case 'm':
            opt_m++;
            strcpy(mode,optarg);
            break;
        case 'h':
        case '?':
            usage();
            return (FMADM_EXIT_SUCCESS);
        default:
            return (FMADM_EXIT_USAGE);
        }
    }
    if (optind < argc) {
        strcpy(arg_module, argv[optind]);
        if (*arg_module == '\0') {
            wr_log("",WR_LOG_ERROR,"fmsadm config:illegal argument-- %s\n",argv[optind]);
            return (FMADM_EXIT_USAGE);
        }
    }

    if (argc == 1) {
        usage();
        return (FMADM_EXIT_ERROR);
    }

    if (((opt_l == 1) && (opt_s == 1))
      ||((opt_l == 1) && (opt_i == 1))
      ||((opt_l == 1) && (opt_b == 1))
      ||((opt_s == 1) && (opt_i == 1))
      ||((opt_s == 1) && (opt_b == 1))
      ||((opt_i == 1) && (opt_b == 1))
      ||((opt_m == 1) && (opt_i == 1))
      ||((opt_m == 1) && (opt_l == 1))
      ||((opt_m == 1) && (opt_s == 1))
      ||((opt_m == 1) && (opt_b == 1))
      ) {
        usage();
        return (FMADM_EXIT_ERROR);
    }
    if (opt_l == 1 && argc == 2) {
        printf("-----------------------------------------\n"
               "Module:\n"
               "-----------------------------------------\n");
        if ((rt = fmd_list_mod()) != 0)
            return (FMADM_EXIT_ERROR);
        return (FMADM_EXIT_SUCCESS);
    }
    else if (opt_l > 1){
        usage();
    }
    else if (opt_s == 1 && argc == 3) {
        printf("----------------------- ----------------------------"
               "-------------------\n"
               "Module                  Configuration\n"
               "----------------------- ----------------------------"
               "-------------------\n");
        if(fmd_show_conf(arg_module) != 0)
            return (FMADM_EXIT_ERROR);
    }else if(opt_s > 1) {
        usage();
    }

    else if(opt_b == 1 && argc == 3)
    {
        if(fmd_add_agent_conf(arg_module) != 0)
            return (FMADM_EXIT_ERROR);
    }else if(opt_b > 1){
        usage();
    }else if(opt_m == 1 && argc == 4){
        if(config_module(adm,arg_module,mode) != 0)
            return (FMADM_EXIT_ERROR);
    }else if(opt_i == 1 && argc == 4)
    {
        if(fmd_add_src_conf(arg_module) != 0)
            return (FMADM_EXIT_ERROR);
    }else if(opt_i > 1){
        usage();
    }else {
        usage();
    }
    return (FMADM_EXIT_SUCCESS);
}

