/*--------------------------------------------------------------------------------------------------
-- Project     :    remoteStub
-- Filename    :    name: wserver.h
--                  created_by: carlos
--                  date_created: Sep 24, 2013
--------------------------------------------------------------------------------------------------
-- File Purpose: TODO
--
--------------------------------------------------------------------------------------------------
-- This software is protected by copyright, the design of any article
-- recorded in the software is protected by design right and the
-- information contained in the software is confidential. This software
-- may not be copied, any design may not be reproduced and the information
-- contained in the software may not be used or disclosed except with the
-- prior written permission of and in a manner permitted by the proprietors
-- Arthika Trading Solutions, S.L.
--------------------------------------------------------------------------------------------------
-- Changes log:
-- TODO
-- 
--------------------------------------------------------------------------------------------------*/


#ifndef WSERVER_H_
#define WSERVER_H_

/* Current configuration */
#define PORT 8888
//#define PORT 4323
#define AUTH_TIMEOUT 300 // in seconds


#define POSTBUFFERSIZE 2048
#define MAXNAMESIZE 20
#define MAXANSWERSIZE 8196

#define MAXKEYDATASIZE 200
#define MAXKEYS 100

#define _MAX_HIST_TRADES_TO_SHOW_ 200

#define GET 0
#define POST 1

/* Control interface for web server, from main */
extern int32 webServerStart (uint16 portNumber);
extern int32 webServerStop (void);

/* Interface functions for incoming data from strategy */

#endif /* WSERVER_H_ */
