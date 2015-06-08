/*--------------------------------------------------------------------------------------------------
-- Project     :    remoteStub
-- Filename    :    name: mapMacros.h
--                  created_by: carlos
--                  date_created: Nov 22, 2013
--------------------------------------------------------------------------------------------------
-- File Purpose: Memory map makros (memory map easier)
--
--------------------------------------------------------------------------------------------------
-- This software is protected by copyright, the design of any article
-- recorded in the software is protected by design right and the
-- information contained in the software is confidential. This software
-- may not be copied, any design may not be reproduced and the information
-- contained in the software may not be used or disclosed except with the
-- prior written permission of and in a manner permitted by the proprietors
-- Arthika Trading Solutions, S.L.
--------------------------------------------------------------------------------------------------*/

#ifndef MAPMACROS_H_
#define MAPMACROS_H_

#define best_Ask_Price(mapPointer, security)  (mapPointer->mapTiTbAsk[mapPointer->mapSortingAsk[security][0].tiIndex][security].price)
#define best_Bid_Price(mapPointer, security)  (mapPointer->mapTiTbbib[mapPointer->mapSortingBid[security][0].tiIndex][security].price)

#define topofbook_Ask_Price(mapPointer, tradingIfIndex, security)  (mapPointer->mapTiTbAsk[tradingIfIndex][security].price)
#define topofbook_Bid_Price(mapPointer, tradingIfIndex, security)  (mapPointer->mapTiTbBid[tradingIfIndex][security].price)


#endif /* MAPMACROS_H_ */
