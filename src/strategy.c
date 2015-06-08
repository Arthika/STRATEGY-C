/*--------------------------------------------------------------------------------------------------
-- Project     :    remoteStub
-- Filename    :    name: strathread.c
--                  created_by: carlos
--                  date_created: Jul 2, 2013
--------------------------------------------------------------------------------------------------
-- File Purpose: Strategy
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "hftUtils_types.h"
#include "asset.h"
#include "security.h"
#include "map.h"
#include "megatick.h"
#include "trade.h"
#include "util.h"
#include "myUtil.h"
#include "stubDbg.h"
#include "strategy.h"
#include "monitoring.h"
#include "macro_commands.h"
#include "parameters.h"
#include "strategy_parameters.h"
#include "ti_marks.h"
#include "params.h"
#include "main.h"
#include "wserver.h"

//#define _PROFILING_DEBUG
// This is just for the initial count down messages ;-)
uint32 last_initial_delay_time_reported = 0;
// Done

// And this is for automated management of pending orders and open positions at the end of the operation period
// (including when the strategy is disconnected)
boolean end_of_operation_cancel_pending=false;
boolean end_of_operation_close_all_pending=false;
// Done

/***************************************************************************************************
* 1. USER INCLUDES: Add your includes here                                                         *
***************************************************************************************************/

/***************************************************************************************************
* 2. LOCAL DEFINES AND TYPES: Add here your defines, structs or typedefs                           *
***************************************************************************************************/

/***************************************************************************************************
* 3. GLOBAL VARIABLES: Add here declaration of your static or global variables                     *
*                      Remember to declare this variables as extern at strategy.h to get access    *
*                      from outside modules (as webserver for instance)                            *
***************************************************************************************************/

/***************************************************************************************************
* 3.1. UTILITY GLOBAL VARIABLES (defined as extern in strategy.h)                                  *
***************************************************************************************************/

char userName[256];
char execName[2048];

timestruct_t INITIAL_strtime; // Initial timestamp measured in strategyInit before any callbacks.
timetype_t   INITIAL_systime;
char         INITIAL_timeStr[SLEN];

timestruct_t CALLBACK_strtime; // Timestamp measured on strategy callback
timetype_t   CALLBACK_systime;

timestruct_t PREVIOUS_END_strtime; // Timestamp measured on (previous) strategy callback end
timetype_t   PREVIOUS_END_systime;

uint32       callback_number; // Ever increasing number of callbacks

char MTtimeStr[SLEN];



/***************************************************************************************************
* 3.2. STRATEGY GLOBAL VARIABLES (defined as extern in strategy.h)                                  *
***************************************************************************************************/

idtype STR_WHICH_SECURITY = EUR_USD;

// This is for the automated testing strategy

// #define STR_MAX_ORDERS_ALIVE 5 // Now defined as parameter
// #define STR_TOTAL_ORDERS_TO_SEND 10000 // Now defined as strategy parameter
#define STR_OUT_OF_MARKET_DIFF 500
#define STR_INTO_MARKET_DIFF 50
#define STR_PRICE_MAX_VARIABILITY 10
#define STR_PRICE_VAR 10

#define STR_NEW_ORDER_TYPE_MARKET 0
#define STR_NEW_ORDER_TYPE_IMMEDIATE_LIMIT_OUT 1
#define STR_NEW_ORDER_TYPE_IMMEDIATE_LIMIT_IN 2
#define STR_NEW_ORDER_TYPE_NON_IMMEDIATE_OUT 3
#define STR_NEW_ORDER_TYPE_NON_IMMEDIATE_IN 4
#define STR_NUMBER_NEW_ORDER_TYPES 5

#define STR_RESERVED_DATA_OFFSET 1000000

#define STR_QUANTUM 50000
#define STR_AMOUNT_RANGE_MIN 2
#define STR_AMOUNT_RANGE_MAX 6

uint32 str_order_counter=0; // This will be added to the reserved data field (plus STR_RESERVED_DATA_OFFSET)

typedef struct _orderRecord_t_ {

    idtype        tiId;            // Trading interface identifier
    idtype        security;        // Security code
    int32         quantity;        // Quantity (negative values are allowed but will change side)
    uint8         side;            // Buy or sell
    uint8         tradeType;       // Trade type
    timetype_t    lastModified;
    boolean       shouldExecuteSoon;
    uint32        replacePrice;
    boolean       cancelRequested;
    boolean       replaceRequested;

} orderRecord_t;

#define STR_MAX_RECORDS 10000
orderRecord_t str_orderRecords[STR_MAX_RECORDS];
uint32 indexOfHistoric_last_reported = 0;
// Done


/***************************************************************************************************
* 4. FUNCTION DEFINITIONS: You must add your strategy code inside next three functions             *
***************************************************************************************************/



int32 strategyInit (sharedMemory_t* memMap, flagsChanges_t* changesMap)
{
    idtype auId = 0;
    int32 auIndex = 0;

    // 0 means that this is the first user introduced by command line (main user)
    getUserAndPassword(0, &auId, userName, NULL);
    setUserAsStrategyMainUser (memMap, userName);

    char filepath[2048];
    getExecName(filepath);

    int start = 0, end = 0;
    end=strlen(filepath); // including the final NULL
    for(int i=end-1; i>=0; i--) {
        if(filepath[i]!='/')
            start = i;
        else
            break;
    }
    for(int i=start; i<=end; i++) execName[i-start]= filepath[i];
    stubDbg(DEBUG_INFO_LEVEL, "userName is %s\n", userName);
    stubDbg(DEBUG_INFO_LEVEL, "exec path is %s\n", filepath);
    stubDbg(DEBUG_INFO_LEVEL, "execName is %s\n", execName);

    /* Init webserver */
    webServerStart (getWebPort());

    /***************************************************************************/
    /* 4.1 ADD YOUR INITIALIZATION CODE HERE IF NEEDED                         */
    /* This function will be called only one time at start                     */
    /*                                                                         */

    // First the generic part (common to all strategies, if you want to use system (stub) utilities

    getStructTimeNowUTC(&INITIAL_strtime, &INITIAL_systime);
    CALLBACK_strtime = INITIAL_strtime;
    CALLBACK_systime = INITIAL_systime;
    callback_number = 0;

    sprintf(INITIAL_timeStr, "%04d/%02d/%02d-%2d:%02d:%02d UTC_%06d",
                             INITIAL_strtime.year,
                             INITIAL_strtime.month,
                             INITIAL_strtime.day,
                             INITIAL_strtime.hour,
                             INITIAL_strtime.min,
                             INITIAL_strtime.sec,
                             INITIAL_systime.usec);

    int32 result = OK;


    stubDbg (DEBUG_INFO_LEVEL, "Initializing strategy for accounting id %d (user name is %s)\n", auId, userName);

    stubDbg (DEBUG_INFO_LEVEL, "Initializing IDs ...\n");
    init_myUtils(memMap); // This is to initialize the wrapper variables to access TI and PBs with the map indexes
    stubDbg (DEBUG_INFO_LEVEL, "Done!\n");

    stubDbg (DEBUG_INFO_LEVEL, "Loading and initializing parameters ...\n");
    load_parameters(); // To initialize parameter values from the config files

    stubDbg(DEBUG_INFO_LEVEL, "Loaded parameters:\n");
    list_system_parameters();
    list_strategy_parameters();
    stubDbg (DEBUG_INFO_LEVEL, "Done!\n");

    stubDbg (DEBUG_INFO_LEVEL, "Loading and initializing ti_marks ...\n");
    load_ti_marks(memMap); // To initialize parameter values from the config files
    stubDbg (DEBUG_INFO_LEVEL, "Done!\n");

    stubDbg (DEBUG_INFO_LEVEL, "Initializing macro commands ...\n");
    init_macro_commands();
    stubDbg (DEBUG_INFO_LEVEL, "Done!\n");

    stubDbg (DEBUG_INFO_LEVEL, "Initializing monitoring ...\n");
    init_monitoring(memMap, changesMap);
    stubDbg (DEBUG_INFO_LEVEL, "Done!\n");

    stubDbg(DEBUG_INFO_LEVEL, "Now setting tick-to-order measurements to one per second\n");
    setTimeTestTrade(1);

    stubDbg(DEBUG_INFO_LEVEL, "Now setting automatic timeouts (when no real ticks are received) to 100ms\n");
    setNotifyTimeout(50);


    auIndex = memMap->strAuIndex;
    if(memMap->AccountingUnit[auIndex].numberOfAlives>0) {
        stubDbg(DEBUG_INFO_LEVEL, "There are trades alive:\n");
        int manyAlives=0;
        for(int i=MAX_TRADES_LIST_ALIVE - 1; (i>=0) && (manyAlives < memMap->AccountingUnit[auIndex].numberOfAlives); i--) {
            if(memMap->AccountingUnit[auIndex].alive[i].alive) {
                manyAlives++;
                char msg[SLEN];
                printLiveOrderReport(memMap, i, msg);
                stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
            } // if trade alive
        } // for trades alive
    } // some trades alive
    else {
        stubDbg(DEBUG_INFO_LEVEL, "No trades alive initially\n");
    } // no trades alive

    stubDbg(DEBUG_INFO_LEVEL, "Initial strategy equity is %.2f\n", memMap->AccountingUnit[auIndex].GlobalEquity_STR);
    stubDbg(DEBUG_INFO_LEVEL, "Initial EP equity is %.2f\n", memMap->EquityPool[auIndex].GlobalEquity_EP);

    // Upon strategy start, it may be a good idea to go over the pending trade list and cancel everything!!

    stubDbg(DEBUG_INFO_LEVEL, "\nDone w (generic) stub initialization\n\n");
    stubDbg(DEBUG_INFO_LEVEL, "Now to start with the specific stub initialization for this strategy\n\n");


    /***************************************************************************/
    /* SPECIFIC INITIALIZATION FOR THE STRATEGY HERE                           */
    /*                                                                         */

    srand(INITIAL_systime.usec);

    /***************************************************************************/
    /* DONE WITH SPECIFIC INITIALIZATION FOR THE STRATEGY                      */
    /*                                                                         */

    stubDbg(DEBUG_INFO_LEVEL, "\nDone w (specific) stub initialization\n\n");

    return (result);

}



int32 strategy (sharedMemory_t* memMap, flagsChanges_t* changesMap)
{
    int32 auIndex = memMap->strAuIndex;

    getStructTimeNowUTC(&CALLBACK_strtime, &CALLBACK_systime);
    callback_number++;
    char timest[NLEN];
    timestampToDateStr(memMap->megatickTime.sec, timest);
    sprintf(MTtimeStr, "%s.%06d [%lu]", timest, memMap->megatickTime.usec, memMap->megatickSequence);


    // Initial delays for strategy callbacks to start until all snapshots etc. have been received and the system is ready to go
    // This should be implemented in the stub lib later on!
        if(CALLBACK_systime.sec<INITIAL_systime.sec+INITIAL_DELAY_FOR_STRATEGY_CALLBACKS) {
            if(last_initial_delay_time_reported!=CALLBACK_systime.sec) {
                last_initial_delay_time_reported = CALLBACK_systime.sec;
                stubDbg(DEBUG_INFO_LEVEL, "Countdown for strategy to start ...%d\n", INITIAL_systime.sec+INITIAL_DELAY_FOR_STRATEGY_CALLBACKS-CALLBACK_systime.sec);
            }
            releaseMapFromStrategy();
            return OK;
        }
    // Done!

    int32 result = OK;

#ifdef _PROFILING_DEBUG
    timestruct_t TrdReport_strtime; timetype_t TrdReport_systime;
    getStructTimeNowUTC(&TrdReport_strtime, &TrdReport_systime);
#endif

    do_trade_reporting(memMap, changesMap);
    do_yellow_cards(memMap, changesMap);

#ifdef _PROFILING_DEBUG
    timestruct_t StrStart_strtime; timetype_t StrStart_systime;
    getStructTimeNowUTC(&StrStart_strtime, &StrStart_systime);
#endif

    /***************************************************************************/
    /* 4.2 WRITE HERE YOUR LOCAL VARIABLES AND ADD YOUR STRATEGY CODE HERE     */
    /* This function will be called just after receiving any data or when      */
    /* timeout is elapsed without rx data default timeout is 100 ms. but can   */
    /* be changed                                                              */
    /*                                                                         */


    if(auto_management_connected) {

        if(strategy_connected) {

            char msg[MLEN]; // This will be used for all add_message
            // 1. We send new orders if there is still room

            boolean trading_ok_now = false;
            if(EVERY_NTH_MILISECOND_TO_TRADE == 0)
                trading_ok_now = false;
            else if(((CALLBACK_systime.usec / 1000) % EVERY_NTH_MILISECOND_TO_TRADE) == 0)
                trading_ok_now = true;

            if(
                    (memMap->AccountingUnit[auIndex].numberOfAlives<STR_MAX_ORDERS_ALIVE) &&
                    (str_order_counter<STR_TOTAL_ORDERS_TO_SEND) &&
                    trading_ok_now
               ) {

                idtype thisTI=rand()%memMap->nTradingInterfaces;
    //            idtype thisPB=memMap->primeBrokerIndex[memMap->whichPrimeBroker[thisTI]];

                idtype sec=which_securities[rand()%many_securities];
                int quantity=STR_QUANTUM * (rand()%(STR_AMOUNT_RANGE_MAX-STR_AMOUNT_RANGE_MIN+1) + STR_AMOUNT_RANGE_MIN);

                tradeCommand_t tc;

                tc.tiId = memMap->tradingInterface[thisTI];
                tc.security = sec;
                tc.quantity = quantity;
                tc.side = rand()%2 ? TRADE_SIDE_BUY : TRADE_SIDE_SELL;
    //            tc.tradeType = TRADE_TYPE_MARKET; // will be set below
    //            tc.limitPrice = 0; // will be set below
                tc.maxShowQuantity = 0; // irrelevant
                tc.timeInForce = TRADE_TIMEINFORCE_INMED_OR_CANCEL; // will be set below
    //            tc.seconds = 0; // irrelevant
    //            tc.mseconds = 0; // irrelevant
    //            tc.expiration = XX; // irrelevant
                tc.unconditional = true;
                tc.checkWorstCase = false;
                tc.reservedData = STR_RESERVED_DATA_OFFSET + str_order_counter;

                boolean shouldExecuteSoon=false;

                int seed=rand()%STR_NUMBER_NEW_ORDER_TYPES;
                switch(seed) {

                    case STR_NEW_ORDER_TYPE_MARKET :
                        tc.tradeType = TRADE_TYPE_MARKET;
                        tc.timeInForce = TRADE_TIMEINFORCE_INMED_OR_CANCEL;
                        shouldExecuteSoon = true;
                        break;
                    case STR_NEW_ORDER_TYPE_IMMEDIATE_LIMIT_OUT :
                        tc.tradeType = TRADE_TYPE_LIMIT;
                        tc.timeInForce = TRADE_TIMEINFORCE_INMED_OR_CANCEL;
                        tc.limitPrice = memMap->mapBookSpInt[sec] + (tc.side == TRADE_SIDE_SELL ? STR_OUT_OF_MARKET_DIFF : - STR_OUT_OF_MARKET_DIFF);
                        shouldExecuteSoon = true;
                        break;
                    case STR_NEW_ORDER_TYPE_IMMEDIATE_LIMIT_IN :
                        tc.tradeType = TRADE_TYPE_LIMIT;
                        tc.timeInForce = TRADE_TIMEINFORCE_INMED_OR_CANCEL;
                        tc.limitPrice = memMap->mapBookSpInt[sec] + (tc.side == TRADE_SIDE_SELL ? - STR_INTO_MARKET_DIFF : STR_INTO_MARKET_DIFF);
                        shouldExecuteSoon = true;
                        break;
                    case STR_NEW_ORDER_TYPE_NON_IMMEDIATE_OUT :
                        tc.tradeType = TRADE_TYPE_LIMIT;
                        tc.timeInForce = (
                                            (memMap->whichVenue[thisTI]==VENUE_TYPE_FXALL) ||
                                            (memMap->whichVenue[thisTI]==VENUE_TYPE_HOTSPOT)
                                         ) ?
                                TRADE_TIMEINFORCE_DAY :
                                TRADE_TIMEINFORCE_GOOD_TILL_CANCEL;

                        tc.limitPrice = memMap->mapBookSpInt[sec] + (tc.side == TRADE_SIDE_SELL ? STR_OUT_OF_MARKET_DIFF : - STR_OUT_OF_MARKET_DIFF);
    //                    shouldExecuteSoon = true; // Deactivated to help Rubén
                        shouldExecuteSoon = false;
                        break;
                    case STR_NEW_ORDER_TYPE_NON_IMMEDIATE_IN :
                        tc.tradeType = TRADE_TYPE_LIMIT;
                        tc.timeInForce = (memMap->whichVenue[thisTI]==VENUE_TYPE_FXALL ?
                                TRADE_TIMEINFORCE_DAY :
                                TRADE_TIMEINFORCE_GOOD_TILL_CANCEL);
                        tc.limitPrice = memMap->mapBookSpInt[sec] + (tc.side == TRADE_SIDE_SELL ? - STR_INTO_MARKET_DIFF : STR_INTO_MARKET_DIFF);
    //                    shouldExecuteSoon = true; // Deactivated to help Rubén
                        shouldExecuteSoon = false;
                        break;
                    default :
                        add_message("??? This should not have happened!! - now stopping everything", STR_MSG_CRITICAL);
                        auto_management_connected=false;
                        strategy_connected=false;
                }

                boolean succeeded;
                if(memMap->tradingInterfaceOk[thisTI] == TI_STATUS_OK) {

                    mySendTradeToCore(memMap, userName, &tc, NULL, &succeeded);

                    if(succeeded) {

                        char timeInForceS[NLEN];
                        getTimeInForceString(tc.timeInForce, timeInForceS);
                        if(tc.tradeType==TRADE_TYPE_MARKET) {

                            stubDbg(DEBUG_INFO_LEVEL, ">>> Sent trade #%d to core thru TI #%d (%s): this was a MARKET %d %s on %s, %s (res data is %u, system mkt price is %d) - MT time is %s\n",
                                    str_order_counter,
                                    thisTI,
                                    memMap->tradingInterfaceName[thisTI],
                                    tc.quantity,
                                    (tc.side==TRADE_SIDE_BUY ? "BUY" : "SELL"),
                                    securityName[tc.security],
                                    timeInForceS,
                                    tc.reservedData,
                                    memMap->mapBookSpInt[sec],
                                    MTtimeStr
                                   );

                        } else {

                            stubDbg(DEBUG_INFO_LEVEL, ">>> Sent trade #%d to core thru TI #%d (%s): this was a LIMIT %d %s on %s, %s, limit was %d (res data is %u, system mkt price is %d) - MT time is %s\n",
                                    str_order_counter,
                                    thisTI,
                                    memMap->tradingInterfaceName[thisTI],
                                    tc.quantity,
                                    (tc.side==TRADE_SIDE_BUY ? "BUY" : "SELL"),
                                    securityName[tc.security],
                                    timeInForceS,
                                    tc.limitPrice,
                                    tc.reservedData,
                                    memMap->mapBookSpInt[sec],
                                    MTtimeStr
                                   );
                        }

                        int thisRecord = str_order_counter % STR_MAX_RECORDS;
                        str_orderRecords[thisRecord].tiId = tc.tiId;
                        str_orderRecords[thisRecord].security = tc.security;
                        str_orderRecords[thisRecord].quantity = tc.quantity;
                        str_orderRecords[thisRecord].side = tc.side;
                        str_orderRecords[thisRecord].tradeType = tc.tradeType;
                        str_orderRecords[thisRecord].lastModified = CALLBACK_systime;
                        str_orderRecords[thisRecord].shouldExecuteSoon = shouldExecuteSoon;
                        str_orderRecords[thisRecord].cancelRequested = false;
                        str_orderRecords[thisRecord].replaceRequested = false;
                        str_orderRecords[thisRecord].replacePrice = 0;

                        str_order_counter++;

                    } // succeeded
                    else {

                        add_message("!!! Call to mySendTradeToCore did not succeed!! (this should not have happened) - strategy will be disconnected\n", STR_MSG_CRITICAL);
                        strategy_connected=false;

                    } // did not succeed

                } // TI is ok


            } // number of alives within max

            // 2. We check out orders alive
            //    - We stop if an order is out of control
            //    - We modify orders alive if they can be modified

            if(memMap->AccountingUnit[auIndex].numberOfAlives > 0) {

                uint32 tradesProcessed=0;
                for(uint32 t=0; (tradesProcessed<memMap->AccountingUnit[auIndex].numberOfAlives) && (t<MAX_TRADES_LIST_ALIVE); t++) {

                    if(!memMap->AccountingUnit[auIndex].alive[t].alive) continue;

                    tradesProcessed++;
                    tradeElement_t *thisTrade=&(memMap->AccountingUnit[auIndex].alive[t]);
                    tradeCommand_t *thisTradeParams=&(thisTrade->params);
                    int32 thisRecord=(thisTradeParams->reservedData - STR_RESERVED_DATA_OFFSET) % STR_MAX_RECORDS;
                    idtype thisTI=memMap->tradingInterfaceIndex[thisTradeParams->tiId];

                    if((thisRecord<0) || (thisRecord>=STR_TOTAL_ORDERS_TO_SEND)) {
                        sprintf(msg, "??? Trade not set by this testing strategy (reserved data is %u)!!! - MT time is %s\n", thisTradeParams->reservedData, MTtimeStr);
                        add_message(msg, STR_MSG_CRITICAL);
                        strategy_connected=false;
                        break;
                    }

                    // We now check if the trade is stuck

                    if(
                            str_orderRecords[thisRecord].shouldExecuteSoon &&
                            (getTimeDifSigned(&(str_orderRecords[thisRecord].lastModified), &CALLBACK_systime) > STR_TIMEOUT_FOR_TRADE_RESOLUTION)
                       ) {

                        // Trade is stuck :(
                        sprintf(msg, "!!! Trade %s is stuck (resvd data is %u) - MT time is %s", thisTrade->ids.fixId, thisTradeParams->reservedData, MTtimeStr);
                        add_message(msg, STR_MSG_ERROR);
                        strategy_connected=false;

                    }

                    if(memMap->tradingInterfaceOk[thisTI] != TI_STATUS_OK) continue; // We can't do anything with this trade anyway :-/


                    if(
                            (str_orderRecords[thisRecord].tradeType == TRADE_TYPE_LIMIT) &&
                            !str_orderRecords[thisRecord].shouldExecuteSoon &&
                            (thisTrade->info.status == TRADE_STATE_PENDING) &&
                            !thisTrade->info.cancel.requested &&
                            !thisTrade->info.modify.requested
                       ) {

                        int32 old_price = memMap->AccountingUnit[auIndex].alive[t].params.limitPrice;
                        int32 new_price;


                        if(rand()%STR_1_IN_N_TO_CANCEL_LIMIT == 0) { // One in every STR_1_IN_N_TO_CANCEL_LIMIT times we cancel the outstanding order
                            myCancelTradeToCore(memMap, userName, thisTrade->ids.fixId, memMap->tradingInterfaceIndex[str_orderRecords[thisRecord].tiId]);
                            str_orderRecords[thisRecord].shouldExecuteSoon=true;
                            str_orderRecords[thisRecord].cancelRequested=true;
                            str_orderRecords[thisRecord].lastModified=CALLBACK_systime;
                            stubDbg(DEBUG_INFO_LEVEL, ">>> Canceling %s trade #%d on TI #%d (%s): ID is %s, this was a LIMIT %d on %s, limit was %d(res data is %u) - MT time is %s\n",
                                    (thisTradeParams->side == TRADE_SIDE_BUY ? "BUY" : "SELL"),
                                    thisRecord,
                                    thisTI,
                                    memMap->tradingInterfaceName[thisTI],
                                    thisTrade->ids.fixId,
                                    thisTradeParams->quantity,
                                    securityName[thisTradeParams->security],
                                    thisTradeParams->limitPrice,
                                    thisTradeParams->reservedData,
                                    MTtimeStr
                                   );
                        } // cancel
                        else {
                            new_price = old_price + rand()%(2*STR_PRICE_MAX_VARIABILITY) - STR_PRICE_MAX_VARIABILITY;
                            if(rand()%STR_1_IN_N_TO_REPLACE_TO_MARKET_PRICE == 0) { // One in every STR_1_IN_N_TO_REPLACE_TO_MARKET_PRICE times we get this into the market for immediate execution
                                new_price = memMap->mapBookSpInt[thisTradeParams->security] +
                                        (thisTradeParams->side == TRADE_SIDE_BUY ? STR_INTO_MARKET_DIFF : - STR_INTO_MARKET_DIFF);
                            }
    //                        if(new_price > memMap->mapBookSpInt[thisSec]) {
    //                                str_orderRecords[thisRecord].shouldExecuteSoon=true; // Deactivated to help Rubén
    //                        } // new_price > system price =>
                            myModifyTradeToCore(memMap, userName, thisTrade->ids.fixId, memMap->tradingInterfaceIndex[str_orderRecords[thisRecord].tiId], new_price, str_orderRecords[thisRecord].quantity, true);
                            str_orderRecords[thisRecord].replaceRequested=true;
                            str_orderRecords[thisRecord].replacePrice=new_price;
                            str_orderRecords[thisRecord].lastModified=CALLBACK_systime;
                            stubDbg(DEBUG_INFO_LEVEL, ">>> Modifying %s trade #%d thru TI #%d (%s): ID is %s, this was a LIMIT %d on %s, old limit was %d and new is %d (res data is %u) - MT time is %s\n",
                                    (thisTradeParams->side == TRADE_SIDE_BUY ? "BUY" : "SELL"),
                                    thisRecord,
                                    thisTI,
                                    memMap->tradingInterfaceName[thisTI],
                                    thisTrade->ids.fixId,
                                    thisTradeParams->quantity,
                                    securityName[thisTradeParams->security],
                                    thisTradeParams->limitPrice,
                                    new_price,
                                    thisTradeParams->reservedData,
                                    MTtimeStr
                                   );
                        } // if not cancel but modify


                    } // trade can be modified
                    else if(
                            (str_orderRecords[thisRecord].tradeType == TRADE_TYPE_LIMIT) &&
                            (thisTrade->info.status == TRADE_STATE_PENDING) &&
                            thisTrade->info.modify.requested &&
                            thisTrade->info.modify.rejected &&
                            !str_orderRecords[thisRecord].cancelRequested
                           ) {

                        stubDbg(DEBUG_INFO_LEVEL, ">>> Modify has been rejected, now canceling %s trade #%d on TI #%d (%s): ID is %s, this was a LIMIT %d on %s, limit was %d(res data is %u) - MT time is %s\n",
                                (thisTradeParams->side == TRADE_SIDE_BUY ? "BUY" : "SELL"),
                                thisRecord,
                                thisTI,
                                memMap->tradingInterfaceName[thisTI],
                                thisTrade->ids.fixId,
                                thisTradeParams->quantity,
                                securityName[thisTradeParams->security],
                                thisTradeParams->limitPrice,
                                thisTradeParams->reservedData,
                                MTtimeStr
                               );
                        myCancelTradeToCore(memMap, userName, thisTrade->ids.fixId, memMap->tradingInterfaceIndex[str_orderRecords[thisRecord].tiId]);
                        str_orderRecords[thisRecord].lastModified=CALLBACK_systime;
                        str_orderRecords[thisRecord].cancelRequested=true;
                        str_orderRecords[thisRecord].shouldExecuteSoon=true;

                    } // modify request was rejected
                    else if(rand()%STR_1_IN_N_TO_CANCEL_RANDOMLY == 0) { // One in every STR_1_IN_N_TO_CANCEL_RANDOMLY we cancel randomnly, whatever this trade is
                        char stS[NLEN];
                        getTradeStatusString(thisTrade->info.status, thisTrade->info.substatus, stS);
                        stubDbg(DEBUG_INFO_LEVEL, ">>> Now to randonmly send a cancel to %s trade #%d on TI #%d (%s): ID is %s, this was a LIMIT %d on %s, limit was %d, status is %s (res data is %u) - MT time is %s\n",
                                (thisTradeParams->side == TRADE_SIDE_BUY ? "BUY" : "SELL"),
                                thisRecord,
                                thisTI,
                                memMap->tradingInterfaceName[thisTI],
                                thisTrade->ids.fixId,
                                thisTradeParams->quantity,
                                securityName[thisTradeParams->security],
                                thisTradeParams->limitPrice,
                                stS,
                                thisTradeParams->reservedData,
                                MTtimeStr
                               );
                        myCancelTradeToCore(memMap, userName, thisTrade->ids.fixId, memMap->tradingInterfaceIndex[str_orderRecords[thisRecord].tiId]);
                    } // random cancel

                } // for trades alive

            } // number of alives > 0

            // 3. We check historical orders

            if(memMap->AccountingUnit[auIndex].indexOfHistoric != indexOfHistoric_last_reported) {

                for(int t = indexOfHistoric_last_reported;
                    t < memMap->AccountingUnit[auIndex].indexOfHistoric + (indexOfHistoric_last_reported > memMap->AccountingUnit[auIndex].indexOfHistoric ? MAX_TRADES_LIST_HISTORIC : 0);
                    t++) {
                    int t_idx = t % MAX_TRADES_LIST_HISTORIC;
                    tradeElement_t *thisTrade=&(memMap->AccountingUnit[auIndex].historic[t_idx]);
                    tradeCommand_t *thisTradeParams=&(thisTrade->params);
                    int32 thisRecord=(thisTradeParams->reservedData - STR_RESERVED_DATA_OFFSET) % STR_MAX_RECORDS;
                    if((thisRecord<0) || (thisRecord>=STR_TOTAL_ORDERS_TO_SEND)) {
                        sprintf(msg, "??? Trade not set by this testing strategy (reserved data is %u)!!! - MT time is %s\n", thisTradeParams->reservedData, MTtimeStr);
                        add_message(msg, STR_MSG_CRITICAL);
    //                    strategy_connected=false;
                        break;
                    }
                    if(str_orderRecords[thisRecord].tiId != thisTradeParams->tiId) {
                        sprintf(msg, "!!! Discrepancy with historical trade #%d: sent trade through TI #%d (%s), but historical trade says %d (%s) - MT time is %s",
                                     thisRecord,
                                     str_orderRecords[thisRecord].tiId,
                                     memMap->tradingInterfaceName[memMap->tradingInterfaceIndex[str_orderRecords[thisRecord].tiId]],
                                     thisTradeParams->tiId,
                                     memMap->tradingInterfaceName[memMap->tradingInterfaceIndex[thisTradeParams->tiId]],
                                     MTtimeStr
                               );
                        add_message(msg, STR_MSG_ERROR);
    //                    strategy_connected=false;
                    }
                    if(str_orderRecords[thisRecord].security != thisTradeParams->security) {
                        sprintf(msg, "!!! Discrepancy with historical trade #%d: security was %s, but historical trade says %s - MT time is %s",
                                     thisRecord,
                                     securityName[str_orderRecords[thisRecord].security],
                                     securityName[thisTradeParams->security],
                                     MTtimeStr
                               );
                        add_message(msg, STR_MSG_ERROR);
    //                    strategy_connected=false;
                    }
    /*
                    if(str_orderRecords[thisRecord].quantity != thisTradeParams->quantity) {
                        sprintf(msg, "!!! Discrepancy with historical trade #%d: quantity was %d, but historical trade says %d",
                                     thisRecord,
                                     str_orderRecords[thisRecord].quantity,
                                     thisTradeParams->quantity
                               );
                        add_message(msg, STR_MSG_ERROR);
                        strategy_connected=false;
                    }
    */
                    if(str_orderRecords[thisRecord].side != thisTradeParams->side) {
                        sprintf(msg, "!!! Discrepancy with historical trade #%d: side was %s, but historical trade says %s - MT time is %s",
                                     thisRecord,
                                     (str_orderRecords[thisRecord].side == TRADE_SIDE_BUY ? "BUY" : "SELL"),
                                     (thisTradeParams->side == TRADE_SIDE_BUY ? "BUY" : "SELL"),
                                     MTtimeStr
                               );
                        add_message(msg, STR_MSG_ERROR);
                        strategy_connected=false;
                    }
                    if(str_orderRecords[thisRecord].tradeType != thisTradeParams->tradeType) {
                        sprintf(msg, "!!! Discrepancy with historical trade #%d: trade type was %s, but historical trade says %s - MT time is %s",
                                     thisRecord,
                                     (str_orderRecords[thisRecord].tradeType == TRADE_TYPE_LIMIT ? "LIMIT" : "MARKET"),
                                     (thisTradeParams->tradeType == TRADE_TYPE_LIMIT ? "LIMIT" : "MARKET"),
                                     MTtimeStr
                               );
                        add_message(msg, STR_MSG_ERROR);
                        strategy_connected=false;
                    }
                    if(
                            (thisTrade->info.status == TRADE_STATE_REPLACED_TO_NEW) &&
                            str_orderRecords[thisRecord].replaceRequested
                       ) {
                        str_orderRecords[thisRecord].replaceRequested = false; // Replace was finished
    /*
                        if(thisTradeParams->finishedPrice != str_orderRecords[thisRecord].replacePrice) {
                            sprintf(msg, "!!! Discrepancy with historical (modified) trade #%d: new limit was %d, but historical trade says %d",
                                         thisRecord,
                                         str_orderRecords[thisRecord].replacePrice,
                                         thisTradeParams->limitPrice
                                   );
                            add_message(msg, STR_MSG_ERROR);
                            strategy_connected=false;
                        }
    */

                    }

                } // for historical trades array

                indexOfHistoric_last_reported = memMap->AccountingUnit[auIndex].indexOfHistoric;

            } // index of historic changed

            if(
                    (str_order_counter >= STR_TOTAL_ORDERS_TO_SEND) &&
                    (memMap->AccountingUnit[auIndex].numberOfAlives == 0)
              ) {
                // Done with the test!!
                add_message(">>> Test happily finished!", STR_MSG_TRADING_INFO);
                strategy_connected=false;

            }

            if(send_cancel_all_after_operation_period) end_of_operation_cancel_pending=true;
            if(send_close_all_after_operation_period) end_of_operation_close_all_pending=true;

        } // if strategy connected
        else if(end_of_operation_cancel_pending && !macro_command_in_progress) {

            if(send_cancel_all_after_operation_period) {
                add_message("--- Now canceling all pending orders, as the operation period just finished", STR_MSG_SYSTEM_INFO);
                if(memMap->AccountingUnit[auIndex].numberOfAlives>0) macro_command_cancel_all_pending_orders_in_AU(memMap);
            } else {
                add_message("--- Operation period just finished (no cancels will be sent)", STR_MSG_SYSTEM_INFO);
            }
            end_of_operation_cancel_pending=false;

        } // end of operation cancel pending
        else if(end_of_operation_close_all_pending && !macro_command_in_progress) {

            if(send_close_all_after_operation_period) {
                add_message("--- Now closing all open positions, as the operation period just finished", STR_MSG_SYSTEM_INFO);
                macro_command_close_all_posis_in_AU(memMap);
            } else {
                add_message("--- Operation period just finished (no close all will be sent)", STR_MSG_SYSTEM_INFO);
            }
            end_of_operation_close_all_pending=false;

        } // end of operation close all pending
        else {

            indexOfHistoric_last_reported = memMap->AccountingUnit[auIndex].indexOfHistoric; // so it starts well next time
            str_order_counter = 0;

        } // strategy_connected

    } // auto_management_connected


    // Done w the strategy!




#ifdef _PROFILING_DEBUG
    timestruct_t StrEnd_strtime; timetype_t StrEnd_systime;
    getStructTimeNowUTC(&StrEnd_strtime, &StrEnd_systime);
#endif

    // We then process macro commands

    if(macro_command_in_progress) process_macro_commands(memMap);

#ifdef _PROFILING_DEBUG
    timestruct_t MacEnd_strtime; timetype_t MacEnd_systime;
    getStructTimeNowUTC(&MacEnd_strtime, &MacEnd_systime);
#endif

    // Finally, we do some basic monitoring

    do_monitoring (memMap, changesMap);

#ifdef _PROFILING_DEBUG
    timestruct_t MonEnd_strtime; timetype_t MonEnd_systime;
    getStructTimeNowUTC(&MonEnd_strtime, &MonEnd_systime);
#endif


    // Done!


    /* Just after the strategy does not need the memory map any more, it can call the release memory function
     * and continue processing. Nevertheless release will be automatically called after finish this function
     * And clean changes list and map for next time that strategy will be called
     */

    releaseMapFromStrategy();


#ifdef _PROFILING_DEBUG
    timestruct_t END_strtime; timetype_t END_systime;
    getStructTimeNowUTC(&END_strtime, &END_systime);
#endif

#ifdef _PROFILING_DEBUG
    stubDbg(DEBUG_INFO_LEVEL, "Callback lasted %d us (%d + %d + %d + %d + %d + %d) [diff from prev is %d]\n",
                              getTimeDifSigned(&CALLBACK_systime, &END_systime),
                              getTimeDifSigned(&CALLBACK_systime, &TrdReport_systime),
                              getTimeDifSigned(&TrdReport_systime, &StrStart_systime),
                              getTimeDifSigned(&StrStart_systime, &StrEnd_systime),
                              getTimeDifSigned(&StrEnd_systime, &MonEnd_systime),
                              getTimeDifSigned(&MacEnd_systime, &MonEnd_systime),
                              getTimeDifSigned(&MonEnd_systime, &END_systime),
                              getTimeDifSigned(&PREVIOUS_END_systime, &CALLBACK_systime)
           );
#endif

    /*                                                                         */
    /***************************************************************************/

    getStructTimeNowUTC(&PREVIOUS_END_strtime, &PREVIOUS_END_systime);
    addCallbackDelay(&CALLBACK_systime, &PREVIOUS_END_systime);

    return (result);

}



int32 strategyFinish (sharedMemory_t* memMap, flagsChanges_t* changesMap)
{
    int32 result = OK;

    /***************************************************************************/
    /* 4.3 ADD YOUR FINALIZATION CODE HERE IF NEEDED                           */
    /* This function will be called when strategy loop has been finished       */
    /* with exit code number. Put here your code needed before exiting         */
    /*                                                                         */

    stubDbg (DEBUG_INFO_LEVEL, "Finishing strategy\n");
    stubDbg (DEBUG_INFO_LEVEL, "Now writing indicators files\n");

    saveAllIndicators(memMap);

    /* Stop web server */
    webServerStop ();

    stubDbg (DEBUG_INFO_LEVEL, "That's all folks!\n");

    /*                                                                         */
    /***************************************************************************/

    return (result);
}



