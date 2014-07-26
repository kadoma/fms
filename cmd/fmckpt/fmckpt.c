/*
 * fmckpt.c
 *
 *  Created on: Nov 1, 2010
 *	Author: Inspur OS Team
 *
 *  Description:
 *	fmckpt.c
 *
 */

#include <sys/stat.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>

#include "fmckpt.h"

#define	FMCKPT_EXIT_SUCCESS	0
#define	FMCKPT_EXIT_FATAL	1
#define	FMCKPT_EXIT_USAGE	2
#define	FMCKPT_EXIT_ERROR	3

static char arg_file[256];
static char arg_time[256];

static int
usage(FILE *fp) 
{
        (void) fprintf(fp, "Usage: fmckpt [-lc] [-r <checkpoint>] [-T <time>]\n");

        (void) fprintf(fp,
	    "\t-l  display the checkpoint list\n"
            "\t-c  rollback to the last checkpoint status\n"
            "\t-r  rollback to the specific checkpoint status\n"
            "\t-T  set the checkpoint interval\n\n"
	    "<time> can be:\n"
	    "\tsec\n"
	    "\tmin\n"
	    "\thour\n"
	    "\tday\n"
	    "\tFor example: 1sec\n\n");

        return (FMCKPT_EXIT_USAGE);
}

static int
fmckpt_copy_file(char *src, char *dst)
{
        int from_fd, to_fd;
        int bytes_read, bytes_write;
        char buffer[3];
        char *ptr;

        /* open the raw file */
        /* open file readonly, return -1 if error; else fd. */
        if ((from_fd = open(src, O_RDONLY)) == -1) {
                fprintf(stderr, "Open %s Error:%s\n", src, strerror(errno));
                return (-1);
        }

        (void) unlink(dst);
        /* create the target file */
        if ((to_fd = open(dst, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
                fprintf(stderr,"Open %s Error:%s\n", dst, strerror(errno));
                return (-1);
        }

        /* a classic copy file code as follow */
        while (bytes_read = read(from_fd, buffer, 3)) {
                /* a fatal error */
                if ((bytes_read == -1) && (errno != EINTR))
                        break;
                else if (bytes_read > 0) {
                        ptr = buffer;
                        while (bytes_write = write(to_fd,ptr,bytes_read)) {
                                /* a fatal error */
                                if ((bytes_write == -1) && (errno != EINTR))
                                        break;
                                /* read bytes have been written */
                                else if (bytes_write == bytes_read)
                                        break;
                                /* only write a part, continue to write */
                                else if (bytes_write > 0) {
                                        ptr += bytes_write;
                                        bytes_read -= bytes_write;
                                }
                        }
                        /* fatal error when writting */
                        if (bytes_write == -1)
                                break;
                }
        }
        close(from_fd);
        close(to_fd);

        return 0;
}

int
fmckpt_ckpt_rename(char *src)
{
        char dir[PATH_MAX], dst[PATH_MAX];

        (void) sprintf(dir, "/usr/lib/fm/ckpt/now");
        (void) sprintf(dst, "%s/ckpt.cpt", dir);

        if (access(dir, 0) != 0) { 
                printf("fmckpt: checkpoint file %s rename failed.\n", src);
                return (-1);
        }    

        if (fmckpt_copy_file(src, dst) != 0 && errno != ENOENT) {
                printf("fmckpt: failed to rename %s\n", src);
                return (-1);
        }    

        return 0;
}

void
fmckpt_run(fmckpt_type_t type)
{
	char buffer[PATH_MAX];
	memset(buffer, 0, 256 * sizeof (char));

	switch (type) {
		case FMCKPT_NOW:	/* -c */
			printf("fmckpt: Ready to rollback to the last checkpoint.\n");
			system("service fmd stop");
			system("service fmd start");
			break;
		case FMCKPT_SPEC:	/* -r checkpoint */
                        printf("fmckpt: Ready to rollback to the specific checkpoint: %s\n", arg_file);
			sprintf(buffer, "/usr/lib/fm/ckpt/%s", arg_file);
			fmckpt_ckpt_rename(buffer);
			system("service fmd stop");
			system("service fmd start");
                        break;
		default:
			break;
	}
}

void
fmckpt_list(void)
{
	char path[] = "/usr/lib/fm/ckpt";
	DIR *dirp;
        struct dirent *dp; 
        const char *p;

        if ((dirp = opendir(path)) == NULL)
                return; /* failed to open directory; just skip it */

	printf("----------------------------"
	       "----------------------------\n"
	       "checkpoint list:\n"
	       "----------------------------"
	       "----------------------------\n");
        while ((dp = readdir(dirp)) != NULL) {
                if ((dp->d_name[0] == '.')
		 || (dp->d_name[0] == 'n')
		 || (dp->d_name[0] == 'c'))
                        continue; /* skip ".", ".." , "now" and "ckpt.conf" */

                p = dp->d_name;

                /* check name */
                assert(p != NULL);

		printf("%s\n", p);
        }

        (void) closedir(dirp);
}

static int
fmckpt_set(void)
{
	char buf[256];
	char buffer[PATH_MAX];
	char filename[] = "/usr/lib/fm/ckpt/ckpt.conf";
	FILE *fp1 = NULL, *fp2 = NULL;

	memset(buf, 0, 256 * sizeof (char));
	memset(buffer, 0, PATH_MAX * sizeof (char));

	if ((fp1 = fopen(filename, "r+")) == NULL) {
		printf("fmckpt: failed to open conf file ckpt.conf\n");
		return (-1);
	}

	while (fgets(buf, sizeof(buf), fp1)) {
		if (strncmp(buf, "interval ", 9) == 0) {
			sprintf(buffer, "%sinterval %s\n", buffer, arg_time);
			continue;
		} else
			sprintf(buffer, "%s%s", buffer, buf);
	}

	if ((fp2 = fopen(filename, "w+")) == NULL) {
		printf("fmckpt: failed to open conf file ckpt.conf\n");
		return (-1);
	}

	fputs(buffer, fp2);

	fclose(fp1);
	fclose(fp2);

	return 0;
}

int
main(int argc, char *argv[])
{
	int opt_l = 0, opt_c = 0, opt_r = 0, opt_T = 0;
	int c;

	memset(arg_file, 0, 256 * sizeof (char));
	memset(arg_time, 0, 256 * sizeof (char));

	while ((c =
	    getopt(argc, argv, "lcr:T:h?")) != EOF) {
		switch (c) {
			case 'l':
				opt_l++;
				break;
			case 'c':
				opt_c++;
				break;
			case 'r':
				opt_r++;
				strcpy(arg_file, optarg);
				break;
			case 'T':
				opt_T++;
				strcpy(arg_time, optarg);
				break;
			case 'h':
			case '?':
				return (usage(stderr));
			default:
				break;
		}
	}

	if (argc == 1)
		return (usage(stderr));

	if (((opt_c == 1) && (opt_r == 1))
	  ||((opt_c == 1) && (opt_T == 1))
	  ||((opt_r == 1) && (opt_T == 1)))
		return (usage(stderr));

	if (opt_l)
		fmckpt_list();
	else if (opt_c)
		fmckpt_run(FMCKPT_NOW);
	else if (opt_r)
		fmckpt_run(FMCKPT_SPEC);
	else if (opt_T)
		fmckpt_set();
	else
		return (usage(stderr));

	return (FMCKPT_EXIT_SUCCESS);
}

