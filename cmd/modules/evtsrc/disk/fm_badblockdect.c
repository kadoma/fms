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
 *     check   if  disk have badblocks
 *     use "badblocks tool".
 *  
 *  path: (eg:/dev/sda   /dev/sdb......)
 *
 *  return value
 *   1       if disk have badblocks
 *   0       disk`s blocks are good
 *   -1      other error
 * ////////////////////////////////////////////////////////  */
int
disk_badblocks_check(const char *path){
        FILE *fstream = NULL;
		FILE *linenum = NULL;
		FILE *sectorsnum = NULL;
		FILE *bbtxt;			/* /tmp/badblockinfo.txt */
        char buff[LINE_MAX];
		char bufline[LINE_MAX];
		char bufsectors[LINE_MAX];
        memset(buff,0,sizeof(buff));
		memset(bufline,0,sizeof(bufline));
		memset(bufsectors,0,sizeof(bufsectors));
			
		char* cmd = NULL;
		char* cmdsectors = NULL;
		char cmdresult[PATH_MAX];
		cmd = badblockscmd(path, cmdresult);		
        	
        fstream = popen(cmd,"r");
        if(fstream == NULL)
       	{
            fprintf(stderr,"execute command failed: %s",strerror(errno));
        }	
		pclose(fstream);


		/* Read badblocks info from /tmp/badblockinfo.txt */
		if ((bbtxt = fopen("/tmp/badblockinfo.txt", "r")) == NULL) {
			perror("/tmp/badblockinfo.txt");
			//syslog(LOG_USER|LOG_INFO, "open /tmp/badblockinfo.txt error\n");
			wr_log("badblock", WR_LOG_ERROR, "open /tmp/badblockinfo.txt error.");
			return -1;
		}

	
		// if disk have badblocks
		if (fgets(buff, LINE_MAX, bbtxt) != NULL){

			// get the count of disk`s badblocks 
			char* cmdcount = "sed -n '$=' /tmp/badblockinfo.txt";
			//get the sum of disk sectors
			
			cmdsectors = getsectorscmd(path, cmdresult);

        	char *resultline = "";
			char *resultsectors = "";
     
        	linenum = popen(cmdcount,"r");
			sectorsnum = popen(cmdsectors,"r");
		
        	if(linenum == NULL || sectorsnum == NULL)
        	{
                fprintf(stderr,"execute command failed: %s",strerror(errno));
        	}	
		
        	if(fgets(bufline, sizeof(bufline),linenum) != NULL)
        	{
                resultline = bufline;
			}
			if(fgets(bufsectors, sizeof(bufsectors),sectorsnum) != NULL)
        	{
                resultsectors = bufsectors;
			}

			if(((atoi(resultline))/(atoi(resultsectors)))> 0.1) { /* disk`s badblocks rate > 10%,  badblocks error */
				pclose(bbtxt);
				return 1;
			}

		}
		pclose(bbtxt);
		return 0;
}