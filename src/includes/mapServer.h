/*--------------------------------------------------------------------------------------------------
-- Project     :    remoteStubGUI
-- Filename    :    name: mapServer.h
--                  created_by: carlos
--                  date_created: Mar 4, 2015
--------------------------------------------------------------------------------------------------
-- File Purpose: UDP server for map (partial)
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
-- 
--------------------------------------------------------------------------------------------------*/

#ifndef MAPSERVER_H_
#define MAPSERVER_H_

#include "hftUtils_types.h"

#define MAX_MAP_SERVER_MSG_BUFFER_SIZE 65536

/***********************************************/
/* commands that can be requested */
/***********************************************/

// get configuration parameters from map
#define MAPSERVER_CODE_MASK              0xFF00

// For auId for a given user & password, use MAPSERVER_TRADES_ALIVE

#define MAPSERVER_GET_CONFIG_MASK        0x0100
#define MAPSERVER_GET_NUMBER_OF_PB       0x0101
#define MAPSERVER_GET_PB_ID_FROM_INDEX   0x0102
#define MAPSERVER_GET_PB_INDEX_FROM_ID   0x0103
#define MAPSERVER_GET_PB_NAME_FROM_ID    0x0104
#define MAPSERVER_GET_PB_NAME_FROM_INDEX 0x0105
#define MAPSERVER_GET_NUMBER_OF_TI       0x0106
#define MAPSERVER_GET_TI_ID_FROM_INDEX   0x0107
#define MAPSERVER_GET_TI_INDEX_FROM_ID   0x0108
#define MAPSERVER_GET_TI_NAME_FROM_ID    0x0109
#define MAPSERVER_GET_TI_NAME_FROM_INDEX 0x0110
#define MAPSERVER_GET_PB_FROM_TI_INDEX   0x0111
#define MAPSERVER_GET_PB_FROM_TI_ID		 0x0112
#define MAPSERVER_GET_DECIMAL_POSITIONS_FOR_SECURITY 0x0113

// get all types of prices
#define MAPSERVER_GET_PRICES_MASK        0x0200

#define MAPSERVER_GET_PRICES_FIX_TOB_BID 0x0201
#define MAPSERVER_GET_PRICES_FIX_TOB_ASK 0x0202
#define MAPSERVER_GET_PRICES_FIX_FBA_BID 0x0203
#define MAPSERVER_GET_PRICES_FIX_FBA_ASK 0x0204
#define MAPSERVER_GET_PRICES_FIX_FBD_BID 0x0205
#define MAPSERVER_GET_PRICES_FIX_FBD_ASK 0x0206

#define MAPSERVER_GET_PRICES_BOOK_ASSET  0x0210
#define MAPSERVER_GET_PRICES_BOOK_SECURITY      0x0211

// get exposures
#define MAPSERVER_GET_EXPOSURES_MASK     0x0300
#define MAPSERVER_GET_ASSET_EXPOSURE     0x0301
#define MAPSERVER_GET_SECURITY_EXPOSURE  0x0302
#define MAPSERVER_GET_TOTAL_ASSET_EXPOSURE 	 	0x0303
#define MAPSERVER_GET_GLOBAL_ASSET_EXPOSURE 	0x0304
#define MAPSERVER_GET_GLOBAL_SECURITY_EXPOSURE 	0x0305
#define MAPSERVER_GET_GLOBAL_TOTAL_ASSET_EXPOSURE 	 0x0306

// get accounting from any accounting unit
#define MAPSERVER_GET_ACCOUNTING_MASK    0x0400
#define MAPSERVER_GET_EQUITY		     0x0401
#define MAPSERVER_GET_USED_MARGIN	     0x0402
#define MAPSERVER_GET_RESERVED_MARGIN    0x0403
#define MAPSERVER_GET_GLOBAL_EQUITY	     0x0404

#define MAPSERVER_GET_TRADES_MASK        0x0500
#define MAPSERVER_SEND_TRADE 			 0x0501
#define MAPSERVER_MODIFY_TRADE 			 0x0502
#define MAPSERVER_CANCEL_TRADE 			 0x0503
#define MAPSERVER_TRADES_ALIVE 			 0x0504 // Returns the auId for a given user & password
#define MAPSERVER_TRADE_ALIVE_INFO		 0x0505
#define MAPSERVER_TRADE_ALIVE_ID		 0x0506
#define MAPSERVER_TRADE_HISTORIC_INFO	 0x0507

/***********************************************/


// header for incoming messages
typedef struct _mapServer_header_t_
{
    idtype msgCode;
    idtype bookId;
    uint32 megatickSeq;
    int32  param1;
    int32  param2;
    int32  param3;
    int32  param4;
    int32  payloadSize;
}
mapServer_header_t;

/* exported functions */

extern int32 mapServerInit(uint16 portSvr);
extern int32 mapServerExit(void);

#endif /* MAPSERVER_H_ */
