/*--------------------------------------------------------------------------------------------------
-- Project     :    remoteStubJF
-- Filename    :    name: strategy.h
--                  created_by: carlos
--                  date_created: Nov 5, 2013
--------------------------------------------------------------------------------------------------
-- File Purpose: Extern variables defined at strategy.c
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

#ifndef STRATEGY_H_
#define STRATEGY_H_

// Debug checks:
//#define _SHOW_DEBUG_PRICES
#define _CHECK_SORTING
#define _SECURITY_TO_CHECK EUR_USD
#define _CHECK_FULL_BOOK_AGGREGATION
#define _CHECK_TRADES_ALIVE

/***************************************************************************************************
* 3.1. UTILITY GLOBAL VARIABLES EXTERN:  Add here extern declarations of your global variables      *
***************************************************************************************************/

extern char userName[256];
extern char execName[2048];

#define PASSWORD "Ceporruelo"
#define PASSWORD_ON false

extern timestruct_t INITIAL_strtime; // Initial timestamp measured in strategyInit before any callbacks.
extern timetype_t   INITIAL_systime;
extern char         INITIAL_timeStr[SLEN];

extern timestruct_t CALLBACK_strtime; // Timestamp measured on strategy callback
extern timetype_t   CALLBACK_systime;

extern timestruct_t PREVIOUS_END_strtime; // Timestamp measured on (previous) strategy callback end
extern timetype_t   PREVIOUS_END_systime;

extern uint32       callback_number; // Ever increasing number of callbacks
#define INITIAL_DELAY_FOR_STRATEGY_CALLBACKS 5 // Number of seconds to directly return from callbacks before system stabilizes

extern char MTtimeStr[SLEN];


/***************************************************************************************************
* 3.2. STRATEGY GLOBAL VARIABLES EXTERN:  Add here extern declarations of your global variables    *
***************************************************************************************************/
extern idtype STR_WHICH_SECURITY;


#endif /* STRATEGY_H_ */
