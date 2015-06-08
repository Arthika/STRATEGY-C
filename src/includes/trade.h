/*--------------------------------------------------------------------------------------------------
-- Project     :    remoteStub
-- Filename    :    name: trade.h
--                  created_by: carlos
--                  date_created: Jun 26, 2013
--------------------------------------------------------------------------------------------------
-- File Purpose: Trade functions definition
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


#ifndef TRADE_H_
#define TRADE_H_

#include "hftUtils_types.h"
#include "megatick.h"

/* ERROR CODES returned */

// Trades side
#define TRADE_SIDE_BUY                     1
#define TRADE_SIDE_SELL                    2

// Trade types
#define TRADE_TYPE_MARKET                  1
#define TRADE_TYPE_LIMIT                   2
#define TRADE_TYPE_ICEBERG                 3

// Time in force Valid values (not all type trades support all time in force values):
#define TRADE_TIMEINFORCE_DAY              0   // 0 = Day
#define TRADE_TIMEINFORCE_GOOD_TILL_CANCEL 1   // 1 = Good Till Cancel (GTC)
#define TRADE_TIMEINFORCE_INMED_OR_CANCEL  3   // 3 = Immediate or Cancel (OC)
#define TRADE_TIMEINFORCE_FILL_OR_KILL     4   // 4 = Fill or Kill (FOK)
#define TRADE_TIMEINFORCE_DATE             6   // 6 = Good Till Date
#define TRADE_TIMEINFORCE_GOOD_FOR_SECS    7   // 7 = Good For Seconds
#define TRADE_TIMEINFORCE_GOOD_FOR_MSECS   8   // 8 = Good For Milliseconds

// Trades states
#define TRADE_STATE_INVALID                0
#define TRADE_STATE_IN_FLUX                1
#define TRADE_STATE_PENDING                2
#define TRADE_STATE_INDETERMINED           3
#define TRADE_STATE_EXECUTED               4
#define TRADE_STATE_CANCELED               5
#define TRADE_STATE_REJECTED               6
#define TRADE_STATE_ERROR_SEND             7
#define TRADE_STATE_REPLACED_TO_NEW        8
#define TRADE_STATE_REPLACED_TO_CANCEL     9
#define TRADE_STATE_CANCELED_BY_USER       10

/* trade status struct */

typedef struct _tradeCommand_t_
{
    /* ordered information */
    idtype        tiId;            // Trading interface identifier
    idtype        security;        // Security code
    int32         quantity;        // Quantity (negative values are allowed but will change side)
    uint8         side;            // Buy or sell
    uint8         tradeType;       // Trade type
    uint32        limitPrice;      // Limit price for trade in the case of limit type
    uint32        maxShowQuantity; // Maximum amount shown on an iceberg order
    uint8         timeInForce;     // Time in force type (ToDo: TBD)
    uint32        seconds;         // Number of seconds: mandatory in the case of good for seconds
    uint32        mseconds;        // Number of mseconds: mandatory in the case of good for milliseconds
    timestruct_t  expiration;      // Mandatory time and/or date for good till date
    boolean       unconditional;   // flag to send unconditional trade (even if no margin reserve is possible)
    boolean       checkWorstCase;  // flag to send indication to core that checks worst case when margin reserve (only real in other case)

    /* returned information when trade is finished */
    uint32        finishedPrice;    // Average price when finished
    int32         finishedQuantity; // Quantity filled when finish
    number        commission;       // return number with commission number when executed
    idtype        currencyCommis;   // currency for the commission

    /* User data: this content will be returned unchanged to user when finished */
    uint32        priceAtStart;     // Automatically filled with trading interface price when trade is send
    uint32        reservedData;     // Reserved content for user (returned unchanged as is)
}
tradeCommand_t;


/* Exported functions */

extern int32 RequestTradeListToCore (idtype auId);

/* Time in force utils */

// get if value of order type and time in force is  OK (OK=0) for a venue type
extern int32 TimeInForceValidFromType (idtype venueType, uint8 timeInForce, uint8 orderType);


#endif /* TRADE_H_ */
