/*--------------------------------------------------------------------------------------------------
-- Project     :    hftUtils
-- Filename    :    name: stubDbg.h
--                  created_by: Ruben Nieto
--                  date_created: Nov 2, 2013
--------------------------------------------------------------------------------------------------
-- File Purpose: V1.0
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

#ifndef STUBDBG_H_
#define STUBDBG_H_

/** ************************************************************************************************
* Debug levels
***************************************************************************************************/

#define NO_LEVEL                0x00000000

#define DEBUG_ERROR_LEVEL       0x00000001
#define DEBUG_WARNING_LEVEL     0x00000002
#define DEBUG_INFO_LEVEL        0x00000004
#define DEBUG_CMD_LEVEL         0x00000008

#define DEBUG_MEGATICK          0x00000010
#define DEBUG_TRADE             0x00000020
#define DEBUG_PERFORMANCE       0x00000040
#define DEBUG_FBA               0x00000080
#define DEBUG_STRATEGY          0x00000100

/** ************************************************************************************************
* stubDbg
* @details Print messages to console (use instead printf)
* @param   level Inidicates the debug level to discrimne or not message to print out
* @param   fmt Formatted message with respective arguments, to send directly to printf function
***************************************************************************************************/
extern void stubDbg (uint32 level, const char* fmt, ...);


#endif /* STUBDBG_H_ */
