/*--------------------------------------------------------------------------------------------------
-- Project     :    remoteStub
-- Filename    :    name: params.h
--                  created_by: carlos
--                  date_created: Jun 20, 2014
--------------------------------------------------------------------------------------------------
-- File Purpose: AU identification function definition
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

#ifndef PARAMS_H_
#define PARAMS_H_

#define MAX_EXECNAME_SIZE    256

#define MAX_USER_SIZE        64
#define MAX_PASSWORD_SIZE    64

#define MAX_NUMBER_OF_USERS  64

#define MAX_FILENAME_SIZE   256

#define MAX_CONFIG_FILE_PATH 256

#define CONFIG_FILE_DEFAULT_PATH "stubConfigFile"

// parameters api functions

extern int32   getExecName (char* name);
extern int32   getNumberOfUsers (void);
extern int32   getUserAndPassword (int32 userIndex, idtype* auId, char* user, char* password);
extern int32   checkUserAndPassword (char* user, char* password, int32* userIndex, idtype* auId);
extern idtype  getAccountingUnitId (int32 userIndex);
extern idtype  getAccountingUnitIdUser (char* user);
extern int32   setUserAndPassword (char *users, char *passwords);
extern int32   setIpandPortCore(char *ip, char *port);
extern int32   setUserInfo (char* user, idtype auId);

extern int32   getCoreAddr (uint32* ipaddr, uint16* port);
extern boolean getDbgLevel (uint32* level);
extern boolean getLogLevel (uint32* level);
extern uint16  getWebPort (void);
extern uint16  getMapServerPort (void);
extern boolean getPriceLog (void);
extern boolean getPriceLogOneFilePerTi (void);
extern boolean getMarginSupervision (void);
extern number  getMarginLimit (void);
extern void	   setMarginLimit (number);

extern boolean getSaveConfigFile (void);
extern boolean getConfigureFromFile (char* filename);
extern int32   getConfigFile (char* filename, byte* buffer, int32* mlen);
extern int32   setConfigFile (byte* buffer, int32 mlen);

extern int32 		getCommandStatus (void);
extern void 		setCommandStatus (int value);
extern int32 		getCommandType (void);
extern void 		setCommandType (int value);
extern boolean 		getCommandInProgress (void);
extern void 		setCommandInProgress (boolean value);
extern void 		setCommandFailures (uint32 value);
extern uint32 		getCommandFailures (void);
extern uint32 		getCommandNumberOfCancelTries (void);
extern void 		setCommandNumberOfCancelTries (uint32 value);
extern uint32 		getCommandInitialIndexOfHistoric (void);
extern void 		setCommandInitialIndexOfHistoric (uint32 value);
boolean 			getCommandRelaunched (void);
void 				setCommandRelaunched (boolean value);
extern void 		getCommandStartTime (timetype_t* value);
extern void 	  	setCommandStartTime (timetype_t* value);
extern void		 	getCommandLastCancelSentWhen (timetype_t* value);
extern void 		setCommandLastCancelSentWhen (timetype_t* value);
extern void 		getCommandLastTimeWQuotes (timetype_t* value);
extern void 		setCommandLastTimeWQuotes (timetype_t* value);

#endif /* PARAMS_H_ */
