/*--------------------------------------------------------------------------------------------------
-- Project     :    remoteStub
-- Filename    :    name: util.h
--                  created_by: carlos
--                  date_created: Jul 3, 2013
--------------------------------------------------------------------------------------------------
-- File Purpose: Utilities for time management at user level
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


#ifndef UTIL_H_
#define UTIL_H_

#include "hftUtils_types.h"
#include "map.h"

/************************************************************************************/
/* Assets and securities utilities                                                  */

extern int32 getAssetString (idtype assetId, char* ptr);
extern int32 getBaseAndTerm (idtype secId, idtype* baseId, idtype* termId);
extern int32 getBaseAndTermStr (idtype secId, char* baseStr, char* termStr);

extern int32 getAssetDecimals (idtype assetId);
extern int32 getAssetDecimalsMultiplier (idtype assetId);
extern int32 getSecurityDecimals (idtype securityId);
extern int32 getSecurityDecimalsMultiplier (idtype securityId);

extern int32 setAssetDecimals (sharedMemory_t* memMap, idtype assetId, uint16 numberOfDecimals);
extern int32 setSecurityDecimals (sharedMemory_t* memMap, idtype securityId, uint16 numberOfDecimals);

/************************************************************************************/
/* Time utilities                                                                   */

/* to get time as seconds and milliseconds */
extern int32 getTimeNowLocal (timetype_t* systime);
extern int32 getTimeNowUTC   (timetype_t* systime);

/* to get time as btoh structures, time struct and second and milliseconds */
extern int32 getStructTimeNowLocal (timestruct_t* strtime, timetype_t* systime);
extern int32 getStructTimeNowUTC   (timestruct_t* strtime, timetype_t* systime);

/* to get time as string */
extern int32 getStringTimeNowLocal (char* stringtime);
extern int32 getStringTimeNowUTC   (char* stringtime);

/* to get time as string but formatted for filename (no slashes) */
extern int32 getStringFileTimeNowLocal (char* stringtime);
extern int32 getStringFileTimeNowUTC (char* stringtime);

/* to convert seconds from 1970 to struct time */
extern int32 timestampToDate (uint32 tu_time, timestruct_t* ts_stime);
extern int32 timestampToDateStr (uint32 tu_time, char* strDate);

/* to convert struct time to seconds from 1970 */
extern int32 dateTotimestamp (timestruct_t* ts_stime, uint32 *tu_time);

/* to calculate time differences */
// in the case that we now that end is greater than start
extern uint64 getTimeDif (timetype_t* systimeStart, timetype_t* systimeEnd);
// in the case that End is greater than start we will return a positive value, otherwise a negative
extern int64  getTimeDifSigned (timetype_t* systimeStart, timetype_t* systimeEnd);

extern uint64 getTimeDifToNow (timetype_t* systimeStart);

/* version control */
extern char* stubLib_getVersion (void);
extern char* stubLib_getBuild (void);
extern void  stubLib_getAllVersion (char* version, char* build);

#endif /* UTIL_H_ */
