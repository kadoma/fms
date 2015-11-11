 /************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    cpu.c
 * Author:      Inspur OS Team
                wang.leibj@inspur.com
 * Date:        2015-08-11
 * Description: get cpu infomation function
 *
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <assert.h>
#include <string.h>

#include <sys/types.h>
#include <sys/fcntl.h>
#include <linux/limits.h>
#include <logging.h>
#include <fmd_errno.h>
#include <fmd_topo.h>

/**
 * @param
 * @return
 *     FMD_SUCCESS
 *     NODE_ID_INVALID
 *     OPENDIR_FAILED
 *
 * @desc:
 *     Get Single CPU Info
 */
static topo_cpu_t *
fmd_topo_read_cpu(const char *dir, int nodeid, int processorid)
{
    char path[PATH_MAX];
    int fd;
    char buf;
    topo_cpu_t *pcpu = NULL;

    /* malloc */
    pcpu = (topo_cpu_t *)malloc(sizeof(topo_cpu_t));
    assert(pcpu != NULL);
    memset(pcpu, 0, sizeof(topo_cpu_t));

    pcpu->cpu_system = 0;
    pcpu->cpu_chassis = nodeid;
    pcpu->cpu_board = 0;

    /* physical_package_id */
    snprintf(path, sizeof(path), "%s/%s", dir,
            "topology/physical_package_id");
    fd = open(path, O_RDONLY);
    if(fd < 0) {
        wr_log("",WR_LOG_ERROR,"OPEN %s failed ",path);
        return NULL;
    }
    if(read(fd, &buf, sizeof(buf)) < 0) {
		close(fd);
        wr_log("",WR_LOG_ERROR,"read %s failed ",path);
        return NULL;
    }
    close(fd);
    pcpu->cpu_socket = buf - '0';

    /* core id */
    snprintf(path, sizeof(path), "%s/%s", dir,
            "topology/core_id");
    fd = open(path, O_RDONLY);
    if(fd < 0) {
        wr_log("",WR_LOG_ERROR,"OPEN %s failed ",path);
        return NULL;
    }
    if(read(fd, &buf, sizeof(buf)) < 0) {
        wr_log("",WR_LOG_ERROR,"read %s failed ",path);
		close(fd);
        return NULL;
    }
    close(fd);
    pcpu->cpu_core = buf - '0';

    /* thread id */
    pcpu->cpu_thread = 0;

    /* topoclass */
    pcpu->cpu_topoclass = TOPO_PROCESSOR;

    /* processor id */
    pcpu->processor = processorid;

    return pcpu;
}


/**
 * @param
 * @return
 *     FMD_SUCCESS
 *     NODE_ID_INVALID
 *     OPENDIR_FAILED
 *
 * @desc:
 *     Add Single CPU Info TO LIST
 */
void
fmd_topo_walk_cpu(const char *dir, int nodeid, fmd_topo_t *ptopo)
{
    char path[PATH_MAX];
    //int len;
    struct dirent *dp;
    const char *p;
    char ch;
    DIR *dirp;
    topo_cpu_t *pcpu;
    if ((dirp = opendir(dir)) == NULL)
        return; /* failed to open directory; just skip it */

    while ((dp = readdir(dirp)) != NULL) {
        if (dp->d_name[0] == '.')
            continue; /* skip "." and ".." */

        p = dp->d_name;

        /* check name */
        assert(p != NULL);
        if((strncmp(p, "cpu", 3) != 0)) {
            continue;
        }
        ch = p[3];
        if(ch < '0' || ch > '9') {
            continue;
        }
        ch = p[4];
        if(ch && (ch < '0' || ch > '9')) {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s", dir, p);
        pcpu = fmd_topo_read_cpu(path, nodeid, atoi(p + 3));
        if(pcpu == NULL) {
            wr_log("",WR_LOG_ERROR,"CPU %d Maybe OFFLINE!  ",atoi(p+3));
            continue;
        }
        list_add(&pcpu->list, &ptopo->list_cpu);
    }

    (void) closedir(dirp);
}


/**
 * @param
 * @return
 *
 * @desc:
 *     if these two cpus are thread-level siblings.
 */
int
cpu_sibling(topo_cpu_t *pcpu, topo_cpu_t *ppcpu)
{
    return ((pcpu->cpu_chassis == ppcpu->cpu_chassis) &&
            (pcpu->cpu_socket == ppcpu->cpu_socket) &&
            (pcpu->cpu_core == ppcpu->cpu_core));
}


/**
 * @param
 * @return
 *     FMD_SUCCESS
 *     NODE_ID_INVALID
 *     OPENDIR_FAILED
 *
 * @desc:
 *     Get CPU TOPO
 */
int
fmd_topo_cpu(const char *dir, const char *prefix, fmd_topo_t *ptopo)
{
    char path[PATH_MAX];
    int n;
    struct dirent *dp;
    const char *p;
    char ch;
    DIR *dirp;

    if ((dirp = opendir(dir)) == NULL)
        return OPENDIR_FAILED; /* failed to open directory; just skip it */

    while ((dp = readdir(dirp)) != NULL) {
        if (dp->d_name[0] == '.')
            continue; /* skip "." and ".." */

        p = dp->d_name;
        if (prefix != NULL && (p == NULL ||
                strncmp(p, prefix, n = strlen(prefix)) != 0))
            continue; /* skip files with the wrong suffix */

        /* node[0-8] style */
        ch = p[n];
        if(ch < '0' || ch > '7' || p[n + 1]) { /* node id invalid */
			(void) closedir(dirp);
            return NODE_ID_INVALD;
        }
        (void) snprintf(path, sizeof (path), "%s/%s", dir, dp->d_name);
        (void) fmd_topo_walk_cpu(path, atoi(&ch), ptopo);
    }

    (void) closedir(dirp);
    return FMD_SUCCESS;
}
