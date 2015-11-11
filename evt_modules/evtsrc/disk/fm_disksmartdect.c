/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    fm_disksmartdect.c
 * Author:      Inspur OS Team 
                	guomeisi@inspur.com
 * Date:        2015-08-04
 * Description: check if the smart tool is supported.
 *
 ************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "fm_disksmartdect.h"
#include "fm_cmd.h"
#include "logging.h"

/* ////////////////////////////////////////////////////////
 * DoesSmartWork
 *     check   if  smart is support
 *     use "smartctl -i".
 *
 *  return value
 *   1       if SMART is support and Enable
 *   0       if SMART is Unavailable and device lacks SMART capability
 *  -1       if SMART is support and Disabled, please use command ( -s on ) to enable 
 * ////////////////////////////////////////////////////////  */
int
DoesSmartWork(const char *path) {
        FILE *fstream = NULL;
        char buff[LINE_MAX];
        memset(buff,0,sizeof(buff));
        char* cmd = NULL;
		char cmdresult[PATH_MAX];

		/*   get info of  "SMART support is:"   */
		cmd = smartworkcmd(path, cmdresult);

        char* result = "";
        fstream = popen(cmd,"r");

        if(fstream == NULL)
        {
				wr_log("stderr",WR_LOG_ERROR,"execute command failed: %s",strerror(errno));
				return 0;
        }

        char testchar;
		if(fgets(buff, sizeof(buff),fstream) != NULL)
        {
                result = buff;
                while(*result == ' ')
                {
                        result++;
                }
                testchar = *result;
                if (testchar == 'U'){   //"Unavailable"
						wr_log("",WR_LOG_WARNING,"device lacks SMART capability.\n");
						pclose(fstream);
						return 0;
				}else if(testchar == 'A'){   //"Available"
                        if(fgets(buff, sizeof(buff),fstream) != NULL){
                                result = buff;
								while(*result == ' ')
                				{
                        			result++;
                				}
                                testchar = *result;
                                if(testchar == 'E'){    //"Enabled
                                		pclose(fstream);
										return 1;
                                        
                                }else{          //"Disabled""£¬ SMART support is: Available£¬but  disabled
										wr_log("",WR_LOG_WARNING,"SMART Disabled. Use option -s with argument 'on' to enable it.\n");
										pclose(fstream);
										return -1;
                                }
                        }
                }
        }else{
                pclose(fstream);

				/*  Device supports SMART and is Enabled  */
				cmd = smartsupportcmd(path, cmdresult);
				fstream = popen(cmd,"r");
				if(fstream == NULL)
       			{
               		wr_log("stderr",WR_LOG_ERROR,"execute command failed: %s",strerror(errno));
					return 0;
        		}
				if(fgets(buff, sizeof(buff),fstream) != NULL){ //Device supports SMART and is Enabled
					pclose(fstream);
					return 1;
				}						
		}
		//Device does not support SMART	
		wr_log("",WR_LOG_WARNING,"Device does not support SMART.\n");
        pclose(fstream);
        return 0;
}

/* ////////////////////////////////////////////////////////
 * disk_healthy_check
 *     check   if  disk is healthy 
 *     use "smartctl", make sure  smartctl is enable.
 *
 *  return value
 *   0       if disk is healthy
 *   1       if disk is unhealthy
 * 
 * ////////////////////////////////////////////////////////*/
int disk_unhealthy_check(const char *path) {
		
        FILE *fstream = NULL;
        char buff[LINE_MAX];
        memset(buff,0,sizeof(buff));
		char* cmd = NULL;
		char cmdresult[PATH_MAX];

		/*   get info of "SMART overall-health self-assessment test result:PASSED"  */
		cmd = smarthealthycmd(path, cmdresult);
		
		char* result = ""; 			
		fstream = popen(cmd,"r");
        if(fstream == NULL)
       	{  
			wr_log("stderr",WR_LOG_ERROR,"execute command failed: %s",strerror(errno));
			return 0;
        }
		char testchar;
        if(fgets(buff, sizeof(buff),fstream) != NULL)
        {
            result = buff;
            while(*result == ' ')
            {
                result++;
            }
            testchar = *result;
           	if(testchar == 'P' ) /* testchar == "PASSED" */
            {
                pclose(fstream);
                return 0;
            }      
        }else{  
			pclose(fstream);
			
			/*   SMART Health Status: OK   */
			cmd = smartokcmd(path, cmdresult);
			fstream = popen(cmd,"r");
			if(fstream == NULL)
       		{
               	wr_log("stderr",WR_LOG_ERROR,"execute command failed: %s",strerror(errno));
				return 0;
        	}
        	if(fgets(buff, sizeof(buff),fstream) != NULL)
        	{
            	result = buff;
             	while(*result == ' ')
                {
                        result++;
                }
            	testchar = *result;
            	if(testchar == 'O' ) /* testchar == OK */
            	{
                	pclose(fstream);
                	return 0;
            	}
        	}		
		}
        pclose(fstream);
		return 1;
	
}

/* ////////////////////////////////////////////////////////
 * disk_temperature_check
 *     check   if  disk`s temperature is over-temperature 
 *     use "smartctl", make sure  smartctl is enable.
 *
 *  return value
 *   0       if disk`s temperature is normal
 *   1       if disk`s temperature is over-temperature
 * ////////////////////////////////////////////////////////*/
int disk_temperature_check(const char *path){
        FILE *fstream = NULL;
        char buff[LINE_MAX];
        memset(buff,0,sizeof(buff));
		char* cmd = NULL;
		char cmdresult[PATH_MAX];
		cmd = smarttempcmd(path, cmdresult);
		
		/* get disk`s temperature info */
		//Current Drive Temperature:
        //Drive Trip Temperature: 
        
		char* result = ""; 			
        fstream = popen(cmd,"r");
        if(fstream == NULL)
       	{
			wr_log("stderr",WR_LOG_ERROR,"execute command failed: %s",strerror(errno));
			return 0;
        }	
		
        while(fgets(buff, sizeof(buff),fstream) != NULL)
        {
            result = buff;
            if(atoi(result) > TEMPERATURE_LIMIT ) /* if disk temperature>70C, disk is over-temperature error */
            {
            		pclose(fstream);
					return 1;
            }               
        }
        pclose(fstream);
		return 0;
        	
}