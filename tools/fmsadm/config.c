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

static int opt_l = 0, opt_s = 0;
static int opt_i = 0, opt_b = 0;
static int evtsrc = 0, agent = 0;

static char arg_module[256];
static char arg_interval[256];

static void
usage(void)
{
    fputs("Usage: fmadm config [-l|s] [-i <interval>] "
          "[-b] [module]\n"
          "\t-h    help\n"
          "\t-l    list all the modules\n"
          "\t-s    show the module's configuration\n"
          "\t-i    set the interval value for evtsrc module\n"
          "\t-b    set the subscribing for agent module\n\n"
          "<interval> can be:\n"
          "\tms\n"
          "\tsecond\n"
          "\tminute\n"
          "\thour\n"
          "\tday\n"
          "\tFor example: 1second\n\n"
          , stderr);
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

/*
 * clear old configuration, then add new's.
 */
static int
fmd_add_conf(char *filename, char *mod_name)
{
    char buf[256];
    char buff[256];
    char buffer[PATH_MAX];
    FILE *fp1 = NULL, *fp2 = NULL;
    int ready = 0;

    memset(buf, 0, 256 * sizeof (char));
    memset(buff, 0, 256 * sizeof (char));
    memset(buffer, 0, PATH_MAX * sizeof (char));

    if ((fp1 = fopen(filename, "r+")) == NULL) {
        wr_log("",WR_LOG_ERROR,"fmsadm config:failed to open conf file for %s\n",mod_name);
        return (-1);
    }
    if (evtsrc) {
        while (fgets(buf, sizeof(buf), fp1)) {
            if (strncmp(buf, "interval ", 9) == 0) {
                sprintf(buffer, "%sinterval %s\n", buffer, arg_interval);
                continue;
            } else
                sprintf(buffer, "%s%s", buffer, buf);
        }
    } else if (agent) {
        while (fgets(buf, sizeof(buf), fp1)) {
            printf(
                   "\t(press 'add' to add new subscribe)\n"
                   "\tfor example:add fault.cpu.intel.cache\n"
                   "\t(press 'del' to delete a line)\n\n");
            printf("this line is : %s \n",buf);
            while (fgets(buff,sizeof(buff),stdin)){
                if(strstr(buff,"add") != NULL){
                    sprintf(buffer,"%s%s",buffer,buf);
                    sprintf(buffer,"%ssubscribe %s",buffer, buff+4);
                    break;
                }else if(strstr(buff,"del") != NULL){
                    break;
                }else {
                    sprintf(buffer,"%s%s",buffer,buf);
                    break;
                }
            }
            continue;
        }
    }

    if ((fp2 = fopen(filename, "w+")) == NULL) {
        wr_log("",WR_LOG_ERROR,"fmsadm config:failed to open conf file for %s\n",mod_name);
        return (-1);
    }

    fputs(buffer, fp2);

    fclose(fp1);
    fclose(fp2);

    return 0;
}

static int
fmd_open_conf(char *mod_name)
{
    FILE *fp = NULL;
    char dir[] = "/usr/lib/fms/plugins/";
    char filename[50];
    char buf[256];
    char buffer[PATH_MAX];

    memset(filename, 0, 50 * sizeof (char));
    memset(buf, 0, 256 * sizeof (char));
    memset(buffer, 0, PATH_MAX * sizeof (char));

    sprintf(filename, "%s%s.conf", dir, mod_name);
    if (strstr(mod_name, "src") != NULL)
        evtsrc = 1;
    else if (strstr(mod_name, "agent") != NULL)
        agent = 1;
    if ((fp = fopen(filename, "r+")) == NULL) {
        wr_log("",WR_LOG_ERROR,"fmsadm config:failed to open conf file for %s\n",mod_name);
        return (-1);
    }

    if (opt_s) {
        while (fgets(buffer, sizeof(buffer), fp)) {
            if ((buffer[0] == '\n') || (buffer[0] == '\0')
             || (buffer[0] == '#'))
                continue;        /* skip blank line and '#' line*/
            if (evtsrc == 1) {
                if (strncmp(buffer, "interval ", 9) == 0) {
                    printf("%s\t\t%s", mod_name, buffer);
                    goto out;
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
    } else if (opt_b || opt_i) {
        if (fmd_add_conf(filename, mod_name) != 0)
            return (-1);
    }
out:
    fclose(fp);
    return (0);
}

/*
 * fmadm config command
 *
 *      -h        help
 *    -l        list all the modules
 *      -s        show the module's configuration
 *      -i        set the interval value for evtsrc module
 *      -b        set the subscribing for agent module
 */

int
cmd_config(fmd_adm_t *adm, int argc, char *argv[])
{
    int rt, c;

    memset(arg_module, 0, 256 * sizeof (char));
    memset(arg_interval, 0, 256 * sizeof (char));

    while ((c = getopt(argc, argv, "ls:i:bh?")) != EOF) {
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
      ||((opt_i == 1) && (opt_b == 1))) {
        usage();
        return (FMADM_EXIT_ERROR);
    }
    if (opt_l) {
        printf("-----------------------------------------\n"
               "Module:\n"
               "-----------------------------------------\n");
        if ((rt = fmd_list_mod()) != 0)
            return (FMADM_EXIT_ERROR);
        return (FMADM_EXIT_SUCCESS);
    }
    if (opt_s) {
        printf("----------------------- ----------------------------"
               "-------------------\n"
               "Module                  Configuration\n"
               "----------------------- ----------------------------"
               "-------------------\n");
    }

    if ((rt = fmd_open_conf(arg_module)) != 0)
        return (FMADM_EXIT_ERROR);

    return (FMADM_EXIT_SUCCESS);
}

