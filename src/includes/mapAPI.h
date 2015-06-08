/*--------------------------------------------------------------------------------------------------
-- Project     :    remoteStubGUI
-- Filename    :    name: mapApi.h
--                  created_by: carlos
--                  date_created: Mar 31, 2015
--------------------------------------------------------------------------------------------------
-- File Purpose: Api header file
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

#ifndef MAPAPI_H_
#define MAPAPI_H_

/***************************************************************************************************
 * CONFIGURATION                                                                                   *
 ***************************************************************************************************/

#include "map.h"
#include "hftUtils_types.h"

extern uint64 getMapConfig_megatickSequence (sharedMemory_t* map);
extern int32  getMapConfig_megatickInfo (sharedMemory_t* map, uint64* megatickSequence, timetype_t* megatickTime);
extern int32  getMapConfig_bookInfo (sharedMemory_t* map, idtype* bookId, uint32* bookAddress,uint16* bookPort);
extern idtype getMapConfig_numberOfAccountingUnits (sharedMemory_t* map);
extern idtype getMapConfig_numberOfEquityPools (sharedMemory_t* map);
extern idtype getMapConfig_numberOfPrimeBroker (sharedMemory_t* map);
extern idtype getMapConfig_numberOfTradingInterfaces (sharedMemory_t* map);
extern idtype getMapConfig_primeBrokerId (sharedMemory_t* map, int32 pbIndex);
extern int32  getMapConfig_primeBrokerIndex (sharedMemory_t* map, int32 pbId);
extern int32  getMapConfig_primeBrokerNameFromId (sharedMemory_t* map, idtype pbId, char* name);
extern int32  getMapConfig_primeBrokerNameFromIndex (sharedMemory_t* map, int32 pbIndex, char* name);
extern idtype getMapConfig_tradingInterfaceId (sharedMemory_t* map, int32 tiIndex);
extern int32  getMapConfig_tradingInterfaceIndex (sharedMemory_t* map, int32 tiId);
extern int32  getMapConfig_tradingInterfaceNameFromId (sharedMemory_t* map, idtype tiId, char* name);
extern int32  getMapConfig_tradingInterfaceNameFromIndex (sharedMemory_t* map, int32 tiIndex, char* name);
extern idtype getMapConfig_primeBrokerForTradingInterfaceIndex (sharedMemory_t* map, int32 tiIndex);
extern idtype getMapConfig_primeBrokerForTradingInterfaceId (sharedMemory_t* map, int32 tiId);
extern idtype getMapConfig_venueForTradingInterfaceIndex (sharedMemory_t* map, int32 tiIndex);
extern idtype getMapConfig_venueForTradingInterfaceId (sharedMemory_t* map, int32 tiId);
extern idtype getMapConfig_venueTypeForTradingInterfaceIndex (sharedMemory_t* map, int32 tiIndex);
extern idtype getMapConfig_venueTypeForTradingInterfaceId (sharedMemory_t* map, int32 tiId);

/***************************************************************************************************
 * PRICES                                                                                          *
 ***************************************************************************************************/

extern int32  getMapPrice_topOfBookSortedAsk (sharedMemory_t* map, idtype security, uint32* priceList, uint32* liquidityList, idtype* tiIdList, int32* tiIndexList);
extern int32  getMapPrice_topOfBookSortedBid (sharedMemory_t* map, idtype security, uint32* priceList, uint32* liquidityList, idtype* tiIdList, int32* tiIndexList);
extern int32  getMapPrice_fbdSortedBid (sharedMemory_t* map, idtype security, idtype tiId, uint32* priceList, uint32* liquidityList, idtype* quoteList);
extern int32  getMapPrice_fbdSortedAsk (sharedMemory_t* map, idtype security, idtype tiId, uint32* priceList, uint32* liquidityList, idtype* quoteList);

#endif /* MAPAPI_H_ */
