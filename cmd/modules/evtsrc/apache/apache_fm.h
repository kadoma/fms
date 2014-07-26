/*
 * apache_fm.h
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 *      apache_fm.h 
 */

#ifndef _APACHE_FM_H
#define _APACHE_FM_H

enum {
	FM_OK = 0,
	NETWORK_UNREACHABLE = 1,
	INVALID_ERR = 2,
	SERVER_ERR = 3,
	CLIENT_ERR = 4,
	SOCKET_TIMEOUT = 5
};

#endif  /* _APACHE_FM_H */
