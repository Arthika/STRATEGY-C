/*--------------------------------------------------------------------------------------------------
-- Project     :    remoteStub
-- Filename    :    name: changeContext.h
--                  created_by: carlos
--                  date_created: Mar 4, 2014
--------------------------------------------------------------------------------------------------
-- File Purpose: Superuser resources to add or change context
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

#ifndef CHANGECONTEXT_H_
#define CHANGECONTEXT_H_

/** ************************************************************************************************
* contextCashAdd
* @details Add cash to M1 (relative mode, this will be added or subtracted from current amount)
* @param   pbId Prime Broker identifier to assign the cash
* @param   auId Accounting unit identifier to assign this cash (0 means that is this one)
* @param   asset Affected asset identifier
* @param   amount Amount of cash (positive or negative)
* return   OK if successful or ERROR in other case
***************************************************************************************************/
extern int32 contextCashAdd (idtype pbId, idtype auId, idtype asset, number amount);

/** ************************************************************************************************
* contextCashClean
* @details Clean cash at M1 (set 0 to current amount)
* @param   pbId Prime Broker identifier to assign the cash
* @param   auId Accounting unit identifier to assign this cash (0 means that is this one)
* @param   asset Affected asset identifier
* return   OK if successful or ERROR in other case
***************************************************************************************************/
extern int32 contextCashClean (idtype pbId, idtype auId, idtype asset);

/** ************************************************************************************************
* contextCashChange
* @details Change cash value at M1 (absolute mode, first delete the current amount and set this new one)
* @param   pbId Prime Broker identifier to assign the cash
* @param   auId Accounting unit identifier to assign this cash (0 means that is this one)
* @param   asset Affected asset identifier
* @param   amount Amount of cash (positive or negative)
* return   OK if successful or ERROR in other case
***************************************************************************************************/
extern int32 contextCashChange (idtype pbId, idtype auId, idtype asset, number amount);

/** ************************************************************************************************
* contextTradeAdd
* @details Add and historic trade to be investigated due to was alive last time
* @param   auId Accounting unit identifier to asociate trade
* @param   tradeId Identifier of the trade (to be looked for at context list in core)
* return   OK if successful or ERROR in other case
***************************************************************************************************/
extern int32 contextTradeAdd (idtype auId, char* tradeId);

/** ************************************************************************************************
* contextTradeResolve
* @details Clean trade from alive list, due to was in investigate mode or was resolved externally
* @param   auId Accounting unit identifier to assign this cash (0 means that is this one)
* @param   tradeCommand Trade content
* @param   tradeId Identifier to look for trade
* return   OK if successful or ERROR in other case
***************************************************************************************************/
extern int32 contextTradeResolve (idtype auId, tradeCommand_t* tradeCommand, char* tradeId);

/** ************************************************************************************************
* contextTradeAccount
* @details Add exposure to exposures list M2 only for accounting purposes (trade will not be sent)
*          Useful when a new trade was made using prime broker GUI and must be accounted at core
* @param   auId Accounting unit identifier to assign this cash (0 means that is this one)
* @param   tradeCommand Trade content
* @param   tradeId In the case that an identifier need to be used when logged
* return   OK if successful or ERROR in other case
***************************************************************************************************/
extern int32 contextTradeAccount (idtype auId, tradeCommand_t* tradeCommand, char* tradeId);


#endif /* CHANGECONTEXT_H_ */
