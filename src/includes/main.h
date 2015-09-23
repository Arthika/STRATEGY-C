/*--------------------------------------------------------------------------------------------------
-- Project     :    remoteStub
-- Filename    :    name: main.h
--                  created_by: carlos
--                  date_created: Jul 22, 2013
--------------------------------------------------------------------------------------------------
-- File Purpose: Function prototypes used by main (don't needed by user)
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
-- V1.0
-- 
--------------------------------------------------------------------------------------------------*/


#ifndef MAIN_H_
#define MAIN_H_

/** ************************************************************************************************
* hftDbgInit
* @details Initialize debug module (static data and semaphores)
***************************************************************************************************/
extern void stubDbgInit (void);

/** ************************************************************************************************
* stubDbgTerm
* @details Finish module (free resources)
***************************************************************************************************/
extern void stubDbgTerm (void);

/* Strategy thread control (used by main, don't use at user strategy functions) */
extern int32 strategyStart (void);
extern int32 strategyStop  (void);

/* Parameters management from main loop (don't needed by user) */
extern int32 checkParams (int argc, char* argv[]);
extern int32 setUserAndPassword (char *users, char *passwords);
extern int32 setIpandPortCore(char *ip, char *port);
extern int32 setDbgLevel (uint32 level);


/* exit from stub */
extern void  exitStub (void);
extern void  exitFromCoreToStrategy (idtype strategyId);

#endif /* MAIN_H_ */
