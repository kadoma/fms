/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    fm_badblockdect.c
 * Author:      Inspur OS Team 
                	guomeisi@inspur.com
 * Date:        2015-08-04
 * Description: check if disk has badblock.
 *
 ************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "fm_badblockdect.h"
#include "fm_cmd.h"
#include "logging.h"


/* ////////////////////////////////////////////////////////
 * disk_badblocks_check
 *     check   if  disk has badblock
 *     use "badblocks tool".
 *  
 *  path: (eg:/dev/sda   /dev/sdb......)
 *
 *  return value
 *   1       if disk has badblock
 *   0       disk`s blocks are good
 *   -1      other error
 * ////////////////////////////////////////////////////////  */
int
disk_badblocks_check(const char *path)
{
    FILE *fstream = NULL;
	FILE *fstreamtxt = NULL;
	FILE *linenum = NULL;
	FILE *sectorsnum = NULL;
	FILE *bbtxt;			/* /tmp/badblockinfo.txt */
    char  buff[LINE_MAX];
    char bufline[LINE_MAX];
    char bufsectors[LINE_MAX];
    memset(buff,0,sizeof(buff));
    memset(bufline,0,sizeof(bufline));
    memset(bufsectors,0,sizeof(bufsectors));
			
    char* cmd = NULL;
    char* cmdtxt = NULL;
    char* cmdsectors = NULL;
		char cmdresult[PATH_MAX];
		char cmdsecresult[PATH_MAX];
		char cmdtxtresult[PATH_MAX];


		/* Read badblocks info from /tmp/badblockinfo.txt */
		if ((bbtxt = fopen("/tmp/badblockinfo.txt", "r")) == NULL) {
			cmdtxt = badblockstxt(cmdtxtresult);
			fstreamtxt = popen(cmdtxt,"r");
			if(fstreamtxt == NULL)
       		{
				wr_log("stderr",WR_LOG_ERROR,"execute command failed: %s",strerror(errno));
				return 0;
        	}
			pclose(fstreamtxt);
		}else{
			fclose(bbtxt);
		}
		
		
		
		cmd = badblockscmd(path, cmdresult);		     	
        fstream = popen(cmd,"r");
        if(fstream == NULL)
       	{
            wr_log("stderr",WR_LOG_ERROR,"execute command failed: %s",strerror(errno));
			return 0;
        }	
		pclose(fstream);
	
		// if disk have badblocks
		if ((bbtxt = fopen("/tmp/badblockinfo.txt", "r"))!= NULL){

			// get the count of disk`s badblocks 
			char* cmdcount = "sed -n '$=' /tmp/badblockinfo.txt";
			//get the sum of disk sectors
			
			cmdsectors = getsectorscmd(path, cmdsecresult);

        	char *resultline = "";
			char *resultsectors = "";
     
        	linenum = popen(cmdcount,"r");
			sectorsnum = popen(cmdsectors,"r");
		
        	if(linenum == NULL)
        	{
                wr_log("stderr",WR_LOG_ERROR,"execute command failed: %s",strerror(errno));
				return 0;
        	}	
			if(sectorsnum == NULL)
        	{
                wr_log("stderr",WR_LOG_ERROR,"execute command failed: %s",strerror(errno));
				return 0;
        	}

		
        	if(fgets(bufline, sizeof(bufline),linenum) != NULL)
        	{
                resultline = bufline;
			}else{
				pclose(linenum);
				pclose(sectorsnum);
				fclose(bbtxt);
				return 0;  //  there aren`t bad blocks
			}
			
			if(fgets(bufsectors, sizeof(bufsectors),sectorsnum) != NULL){
				resultsectors = bufsectors;
		
				if((atoi(resultsectors))!=0 &&((atoi(resultline))/(atoi(resultsectors)))> 0.1) { /* disk`s badblocks rate > 10%,  badblocks error */
					pclose(linenum);
					pclose(sectorsnum);
					fclose(bbtxt);
					return 1;
				}else{
					if((atoi(resultline))> 10){
						pclose(linenum);
						pclose(sectorsnum);
						fclose(bbtxt);
						return 1;
					}else{
						pclose(linenum);
						pclose(sectorsnum);
						fclose(bbtxt);
						return 0;
					}
				}				
			}else{
				if((atoi(resultline))> 10){
					pclose(linenum);
					pclose(sectorsnum);
					fclose(bbtxt);
					return 1;
				}else{
					pclose(linenum);
					pclose(sectorsnum);
					fclose(bbtxt);
					return 0;
				}
			}
		}else{
			wr_log("stderr",WR_LOG_ERROR,"open file failed: %s",strerror(errno));
			return 0;
		}
}