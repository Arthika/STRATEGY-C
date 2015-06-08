/*
 * macro_commands.c
 *
 *  Created on: May 21, 2014
 *      Author: julio
 */

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
#include "ti_marks.h"
#include "changeContext.h"
#include "math.h"
#include "params.h"

/*********************************************************************************/
/* API AND EXTERNAL VARIABLES                                                    */
/*********************************************************************************/

boolean macro_command_in_progress = false;
int macro_command_type = MACRO_COMMAND_NO_COMMAND;
int macro_command_status = MACRO_COMMAND_STATUS_NO_STATUS;
char macro_command_output_msg[MLEN] = "";

quoteTradingHistory_t quotesTradingHistoryAsk[MAX_NUMBER_TI][NUM_SECURITIES][MAX_QUOTES];
quoteTradingHistory_t quotesTradingHistoryBid[MAX_NUMBER_TI][NUM_SECURITIES][MAX_QUOTES];


/*********************************************************************************/
/* INTERNAL (PRIVATE) VARIABLES TO macro_commands.c                              */
/*********************************************************************************/

int macro_command_failures = 0;

timetype_t    __macro_command_start_time;
uint32        __macro_command_initial_index_of_historic;
int           __macro_command_markedQuoteIndexes[MAX_NUMBER_TI][NUM_SECURITIES][MAX_QUOTES];
int           __macro_command_number_of_marked_quotes[MAX_NUMBER_TI][NUM_SECURITIES];
int           __macro_command_number_of_cancel_tries;
timetype_t    __macro_command_last_cancel_sent_when;
int32         __macro_command_finalIntendedOffset;
idtype        __macro_command_hedging_security;
idtype        __macro_command_PB_to_wipe;
boolean       __macro_command_nothing_left_to_wipe;
timetype_t    __macro_command_last_time_w_quotes;
boolean       __macro_command_relaunched = false;
boolean       __macro_command_use_market_orders = false;
int32         __macro_command_additional_leeway_to_limit = 0;




/*********************************************************************************/
/* INTERNAL PROCEDURES                                                           */
/*********************************************************************************/



#define __MACRO_COMMAND_COMPARISON_PRECISION 1.0
boolean are_equal(number n1, number n2) {
    return (fabs(n1 - n2) <= __MACRO_COMMAND_COMPARISON_PRECISION);
}



boolean send_best_immediate_order_PB_TI(sharedMemory_t* memMap, idtype security, int32 amount,
                                        boolean onePB, idtype PBifNotAll, boolean oneTI, idtype TIifNotAll, uint32 codeReserved,
                                        boolean considerYellowCards, boolean considerAvailableMargins,
                                        boolean useMarketOrders, int32 additionalLeewayToLimit,
                                        boolean considerOnlyGoodForHedging, int32 maxChunkAmount) {

    int32 auIndex = memMap->strAuIndex;

    stubDbg(DEBUG_INFO_LEVEL, ">>> Called smart routing function: %d %s %s (max chunk is %d)\n - MT time is %s\n",
                              abs(amount), securityName[security], (amount >= 0 ? "BUY" : "SELL"),
                              maxChunkAmount, MTtimeStr);

    int32 amountToSend[MAX_NUMBER_TI] = {0};
    uint32 limitToSend[MAX_NUMBER_TI];

    boolean isBuy = (amount >= 0);
    amount = abs(amount);
    boolean somethingToSend = false;

    // look for the asociated user for this accounting unit index
    int32 k = getNumberOfUsers();
    idtype id = 0;
    char  user[MAX_USER_SIZE]={0};

    for (int32 j=0; j < k; j++)
    {
        getUserAndPassword (j, &id, user, NULL);
        if (memMap->AccountingUnit[auIndex].auId == id)
        {
            // user found
            break;
        }
    }


    if(oneTI) {

        __macro_command_number_of_marked_quotes[TIifNotAll][security]=0;

    } else {

        for(idtype tiIndex=0; tiIndex < memMap->nTradingInterfaces; tiIndex++) {

            if(!onePB || (PBifNotAll == memMap->primeBrokerIndex[memMap->whichPrimeBroker[tiIndex]])) {

                __macro_command_number_of_marked_quotes[tiIndex][security]=0;

            }

        }

    }

    int32 applicableMaximum;
    if(maxChunkAmount >= minimum_order_size) {
        if(maxChunkAmount < maximum_order_size) {
            applicableMaximum = maxChunkAmount;
        } else {
            applicableMaximum = maximum_order_size;
        }
    } else {
        applicableMaximum = maximum_order_size;
    }

    static sortingFB_t OthersQuotesBid[MAX_QUOTES*MAX_NUMBER_TI];
    static int NumberOthersQuotesBid;
    static sortingFB_t OwnQuotesBid[MAX_TRADES_LIST_ALIVE];
    static int NumberOwnQuotesBid;
    static sortingFBbyTI_t OthersQuotesByTIBid[MAX_NUMBER_TI][MAX_QUOTES];
    static int NumberOthersQuotesbyTIBid[MAX_NUMBER_TI];
    static sortingFBbyTI_t OwnQuotesByTIBid[MAX_NUMBER_TI][MAX_TRADES_LIST_ALIVE];
    static int NumberOwnQuotesbyTIBid[MAX_NUMBER_TI];

    removeOwnOrdersFromQuoteList(memMap,
                                 security,
                                 true, // Bid
                                 OthersQuotesBid,
                                 &NumberOthersQuotesBid,
                                 OwnQuotesBid,
                                 &NumberOwnQuotesBid,
                                 OthersQuotesByTIBid,
                                 NumberOthersQuotesbyTIBid,
                                 OwnQuotesByTIBid,
                                 NumberOwnQuotesbyTIBid
                                 );


    static sortingFB_t OthersQuotesAsk[MAX_QUOTES*MAX_NUMBER_TI];
    static int NumberOthersQuotesAsk;
    static sortingFB_t OwnQuotesAsk[MAX_TRADES_LIST_ALIVE];
    static int NumberOwnQuotesAsk;
    static sortingFBbyTI_t OthersQuotesByTIAsk[MAX_NUMBER_TI][MAX_QUOTES];
    static int NumberOthersQuotesbyTIAsk[MAX_NUMBER_TI];
    static sortingFBbyTI_t OwnQuotesByTIAsk[MAX_NUMBER_TI][MAX_TRADES_LIST_ALIVE];
    static int NumberOwnQuotesbyTIAsk[MAX_NUMBER_TI];

    removeOwnOrdersFromQuoteList(memMap,
                                 security,
                                 false, // Ask
                                 OthersQuotesAsk,
                                 &NumberOthersQuotesAsk,
                                 OwnQuotesAsk,
                                 &NumberOwnQuotesAsk,
                                 OthersQuotesByTIAsk,
                                 NumberOthersQuotesbyTIAsk,
                                 OwnQuotesByTIAsk,
                                 NumberOwnQuotesbyTIAsk
                                 );

    for(int q=0; (amount >= minimum_order_size) &&
                 (q < (isBuy ? NumberOthersQuotesAsk : NumberOthersQuotesBid));
                 q++) {

        sortingFB_t *thisSortingElement = (isBuy ?
                                                 &(OthersQuotesAsk[q]) :
                                                 &(OthersQuotesBid[q]));
        uint32 thisPrice=thisSortingElement->price;
        idtype thisTIindex=thisSortingElement->tiIndex;
        idtype thisQuoteIndex=thisSortingElement->quoteIndex;
        mtbookElement_t *thisQuote = (isBuy ?
                                            &(memMap->mapFBQuotesAsk[thisTIindex][security][thisQuoteIndex]) :
                                            &(memMap->mapFBQuotesBid[thisTIindex][security][thisQuoteIndex]));
        uint32 thisAmount = amount;
        quoteTradingHistory_t *thisQuoteTradingHistory = (isBuy ?
                                                                &(quotesTradingHistoryAsk[thisTIindex][security][thisQuoteIndex]) :
                                                                &(quotesTradingHistoryBid[thisTIindex][security][thisQuoteIndex]));

        if(
                (oneTI && (TIifNotAll != thisTIindex)) ||
                (onePB && (PBifNotAll != memMap->primeBrokerIndex[memMap->whichPrimeBroker[thisTIindex]]))
           )
            continue;

        if(thisAmount + amountToSend[thisTIindex] > applicableMaximum)
            thisAmount = applicableMaximum - amountToSend[thisTIindex];
        if(thisAmount > thisQuote->liquidity) thisAmount = thisQuote->liquidity;

        boolean enoughMargin = enoughMarginToTrade(memMap, user, thisTIindex, thisAmount + amountToSend[thisTIindex], security, (isBuy ? TRADE_SIDE_BUY : TRADE_SIDE_SELL));

        if(
                (memMap->tradingInterfaceOk[thisTIindex]!=TI_STATUS_OK) ||
                (
                        ignoreTIswDelayedPrices && pricesDelayed[thisTIindex]
                ) ||
                (
                        ignoreTIswNoTicks && noTicks[thisTIindex]
                ) ||
                (
                        considerYellowCards &&
                        (
                                tiOperation[thisTIindex].yellowCard ||
                                tiOperation[thisTIindex].redCard
                        )
                ) ||
                (thisQuote->price<=0) || // This should never happen, but you know
                (
                        considerAvailableMargins &&
                        !enoughMargin
                ) ||
                (thisAmount<minimum_order_size) ||
                (
                        considerOnlyGoodForHedging &&
                        !goodForHedging[thisTIindex]
                )
          ) {
  /*
            stubDbg(DEBUG_INFO_LEVEL,
                    "Skipping quote #%d of TI #%d (%s): TI is %s (%s and %s), %s yellow card, %s red card, price is %d, %s enough margin, thisAmount is %d\n",
                    q,
                    thisTIindex,
                    memMap->tradingInterfaceName[thisTIindex],
                    (memMap->tradingInterfaceOk[thisTIindex] == TI_STATUS_NOK ? "NOT OK" :
                            (memMap->tradingInterfaceOk[thisTIindex] == TI_STATUS_OK_NO_PRICE ? "OK but no prices" : "OK" )
                    ),
                    (pricesDelayed[thisTIindex] ?
                            "prices delayed" : "prices NOT delayed"
                    ),
                    (noTicks[thisTIindex] ?
                            "NO ticks" : "ticks OK"
                    ),
                    (tiPerformanceInfo[thisTIindex].yellowCard ? "with a" : "no"),
                    (tiPerformanceInfo[thisTIindex].redCard ? "with a" : "no"),
                    thisQuote->price,
                    (enoughMargin ? "with" : "without"),
                    thisAmount
                   );
*/
            continue; // This quote is too small or this TI is not useful
        }


        if(
                !useMarketOrders &&
                (
                        (thisQuote->timestamp.sec==thisQuoteTradingHistory->currentQuoteTimestamp.sec) &&
                        (thisQuote->timestamp.usec==thisQuoteTradingHistory->currentQuoteTimestamp.usec) &&
                        thisQuoteTradingHistory->currentQuoteFailed
                )
           ) {

            stubDbg(DEBUG_INFO_LEVEL,
                    "Skipping quote #%d of TI #%d (%s) since this quote already failed (timestamp is %d:%d)\n",
                    q,
                    thisTIindex,
                    memMap->tradingInterfaceName[thisTIindex],
                    thisQuote->timestamp.sec,
                    thisQuote->timestamp.usec
                   );

            continue; // This quote is not tradeable
        }


        // if we got to this point, then we can try to trade this quote
        amountToSend[thisTIindex]+=thisAmount;
        limitToSend[thisTIindex]=thisPrice;
        if(!useMarketOrders) {
            thisQuoteTradingHistory->currentQuoteTimestamp=thisQuote->timestamp; // Here we mark the quote to later see if it failed
            thisQuoteTradingHistory->currentQuoteFailed=false; // We reset the failure mark
            __macro_command_markedQuoteIndexes[thisTIindex][security][__macro_command_number_of_marked_quotes[thisTIindex][security]]=thisQuoteIndex;
            __macro_command_number_of_marked_quotes[thisTIindex][security]++;
        }
        amount-=thisAmount;
        somethingToSend=true;

    } // loop (quotes)



    if(!somethingToSend) {
//        char amS[NLEN];
//        renderWithDots(amS,amount,true);
//        stubDbg(DEBUG_INFO_LEVEL, "No tradeable quotes available to send immediate trade (%s) on %s\n", amS, securityName[security]);

        sprintf(macro_command_output_msg, "No tradeable quotes"); // __macro_command_last_time_w_quotes is not updated in this case!
        return false;

    } else {

        for(int tiIndex=0; tiIndex<memMap->nTradingInterfaces; tiIndex++) {

            if(amountToSend[tiIndex] > 0) {

                boolean succeeded;
                uint32 tempTradeId;
                tradeCommand_t tc;
                tc.tiId=memMap->tradingInterface[tiIndex];
                tc.security=security;
                tc.quantity=amountToSend[tiIndex];
                tc.side=(isBuy ? TRADE_SIDE_BUY : TRADE_SIDE_SELL);
                if(useMarketOrders)
                    tc.tradeType=TRADE_TYPE_MARKET;
                else
                    tc.tradeType=TRADE_TYPE_LIMIT;
                tc.limitPrice=limitToSend[tiIndex] + (useMarketOrders ? 0 : (isBuy ? additionalLeewayToLimit : -additionalLeewayToLimit));
                tc.timeInForce = TRADE_TIMEINFORCE_INMED_OR_CANCEL;
//                tc.unconditional=false;
                if(!considerAvailableMargins)
                    tc.unconditional=true;
                else
                    tc.unconditional=true; // Changed until margin reserves work well
                tc.checkWorstCase=false;
                tc.reservedData=codeReserved;
                mySendTradeToCore(memMap, user, &tc, &tempTradeId, &succeeded);
                if(succeeded) {
                    char amS[NLEN];
                    renderWithDots(amS,amountToSend[tiIndex], false);
                    stubDbg(DEBUG_INFO_LEVEL, ">>> (From smart routing:) Sent immediate LIMIT order: %s %s @%d on %s through TI #%d (%s)\n",
                            (isBuy ? "BUY" : "SELL"), amS, limitToSend[tiIndex], securityName[security], tiIndex, memMap->tradingInterfaceName[tiIndex]);
                } else {
                    // This should actually not happen, as this trade has been simulated before with the same parameters
                    // So if this happens, this is a major malfunction problem and we should stop everything
                    MalfunctionAlarm=true;
                    sprintf(MalfunctionAlarmText, "Successfully simulated trade failed in mySendTradeToCore!!");
                    add_message("??? Successfully simulated trade failed in mySendTradeToCore - now disconnecting auto trading", STR_MSG_CRITICAL);
                    sprintf(macro_command_output_msg, "Malfunction - successfully simulated trade was rejected by core");
                    strategy_connected=false;
                    macro_command_in_progress=false;
                    macro_command_cancel_all_pending_orders_in_AU(memMap);
                    return false;

                } // succeeded or not

            } // if amountToSend != 0

        } // for tiIndex

    } // somethingToSend

    __macro_command_last_time_w_quotes=CALLBACK_systime;
    stubDbg(DEBUG_INFO_LEVEL, "--- Trades just were sent through smart routing function (send_best_immediate_order_PB_TI).\n"
                              "Current ToB prices were as follows:\n");
    print_prices(memMap, security, isBuy, !isBuy);

    return true;
}



void sendCancelsToAllPendingOrders(sharedMemory_t* memMap, int32 auIndex) {

    boolean someCancelSent=false;
    uint32 tradesProcessed=0;

    // look for the asociated user for this accounting unit index
    int32 k = getNumberOfUsers();
    idtype id = 0;
    char  user[MAX_USER_SIZE]={0};

    for (int32 j=0; j < k; j++)
    {
        getUserAndPassword (j, &id, user, NULL);
        if (memMap->AccountingUnit[auIndex].auId == id)
        {
            // user found
            break;
        }
    }

    for(uint32 t=0; (t<MAX_TRADES_LIST_ALIVE) && (tradesProcessed<memMap->AccountingUnit[auIndex].numberOfAlives); t++) {

        if(true == memMap->AccountingUnit[auIndex].alive[t].alive) tradesProcessed++;
        else continue;

        idtype tiIndex;
        tiIndex=memMap->tradingInterfaceIndex[memMap->AccountingUnit[auIndex].alive[t].params.tiId];

        boolean canSendCancel =
                (my_getTimeDifSigned(&__macro_command_last_cancel_sent_when, &CALLBACK_systime) >= MACRO_COMMAND_MAX_WAIT_TIME_FOR_CANCELS) ||
                (!memMap->AccountingUnit[auIndex].alive[t].info.cancel.requested && !memMap->AccountingUnit[auIndex].alive[t].info.modify.requested);

        if(
                (TRADE_TYPE_MARKET != memMap->AccountingUnit[auIndex].alive[t].params.tradeType) &&
                (TRADE_TIMEINFORCE_FILL_OR_KILL != memMap->AccountingUnit[auIndex].alive[t].params.timeInForce) &&
                (TRADE_TIMEINFORCE_INMED_OR_CANCEL != memMap->AccountingUnit[auIndex].alive[t].params.timeInForce) &&
                (TRADE_STATE_PENDING == memMap->AccountingUnit[auIndex].alive[t].info.status) &&
                canSendCancel &&
//                !memMap->alive[t].info.modify.requested &&
                (memMap->tradingInterfaceOk[tiIndex]!=TI_STATUS_NOK) // We can't do anything anyway
           ) {

            char status[NLEN];
            getTradeStatusString(memMap->AccountingUnit[auIndex].alive[t].info.status, memMap->AccountingUnit[auIndex].alive[t].info.substatus, status);
            stubDbg (DEBUG_INFO_LEVEL, ">>> Sending cancel to core for trade ID %s (in TI #%d - %s), "
                                       "status is %s, cancel%s requested (%srejected), modify%s requested (%srejected) - MTtime is %s\n",
                                       memMap->AccountingUnit[auIndex].alive[t].ids.fixId, tiIndex, memMap->tradingInterfaceName[tiIndex],
                                       status,
                                       (memMap->AccountingUnit[auIndex].alive[t].info.cancel.requested ? "" : " NOT"),
                                       (memMap->AccountingUnit[auIndex].alive[t].info.cancel.rejected ? "" : "NOT "),
                                       (memMap->AccountingUnit[auIndex].alive[t].info.modify.requested ? "" : " NOT"),
                                       (memMap->AccountingUnit[auIndex].alive[t].info.modify.rejected ? "" : "NOT "),
                                       MTtimeStr
                                       );
            int32 result = myCancelTradeToCore(memMap, user, memMap->AccountingUnit[auIndex].alive[t].ids.fixId, tiIndex);
            someCancelSent = true;
            stubDbg (DEBUG_INFO_LEVEL, "--- Cancel sent, result = %d\n\n", result);

        } // if pending order can be sent

//        else {
//            stubDbg (DEBUG_INFO_LEVEL, ">> Trade ID %s in TI #%d (%s) cannot be canceled, so we are leaving it as it is\n",
//                    memMap->alive[t].ids.fixId, tiIndex, memMap->tradingInterfaceName[tiIndex]);
//        }

    } // loop (trades alive)

    if(someCancelSent) {
        __macro_command_number_of_cancel_tries++;
        __macro_command_last_cancel_sent_when = CALLBACK_systime;
    }

}



boolean do_wipe_PB_from_AU(sharedMemory_t* memMap, idtype pbIndex, int32 auIndex) {

    boolean anyPendingOrderToResolve=false;
    boolean anyPosiToClose=false;

    uint32 tradesProcessed=0;
    for(uint32 t=0; (tradesProcessed<memMap->AccountingUnit[auIndex].numberOfAlives) && (t<MAX_TRADES_LIST_ALIVE); t++) {

        if(!memMap->AccountingUnit[auIndex].alive[t].alive) continue;
        tradesProcessed++;

        idtype thisTIindex=memMap->tradingInterfaceIndex[memMap->AccountingUnit[auIndex].alive[t].params.tiId];
        idtype thisPB=memMap->primeBrokerIndex[memMap->whichPrimeBroker[thisTIindex]];

        if(thisPB==pbIndex) {

            anyPendingOrderToResolve=true;
            tradeCommand_t tc;
            tc.finishedQuantity=0;
            tc.finishedPrice=0;

            if(contextTradeResolve(memMap->AccountingUnit[auIndex].auId, &tc, memMap->AccountingUnit[auIndex].alive[t].ids.fixId)!=OK) {

                sprintf(macro_command_output_msg, "Error resolving pending order %s", memMap->AccountingUnit[auIndex].alive[t].ids.fixId);
                return false;

            } // if contextTradeResolve failed

        } // if this trade is in the selected PB

    } // for trades alive


    if(!anyPendingOrderToResolve) {

        idtype pbID=memMap->primeBroker[pbIndex];

        for(int s=0; s<NUM_SECURITIES; s++) {

            if(memMap->AccountingUnit[auIndex].SecurityExposureInt[pbIndex][s].amount != 0) {

                int tiForThisPB=-1;
                for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {

                    if(memMap->whichPrimeBroker[ti]==pbID) {
                        tiForThisPB=ti; // we found it
                        break;
                    }

                }

                if((tiForThisPB<0) || (tiForThisPB>=memMap->nTradingInterfaces)) {

                    sprintf(macro_command_output_msg, "No TI for PB %s", memMap->primeBrokerName[pbIndex]);
                    return false;

                }

                tradeCommand_t tc;
                char tID[SLEN];
                tc.tiId = memMap->tradingInterface[tiForThisPB];
                tc.security = s;
                tc.quantity = abs(memMap->AccountingUnit[auIndex].SecurityExposureInt[pbIndex][s].amount);
                tc.side = memMap->AccountingUnit[auIndex].SecurityExposureInt[pbIndex][s].amount > 0 ? TRADE_SIDE_SELL : TRADE_SIDE_BUY;
                tc.tradeType = TRADE_TYPE_MARKET; // irrelevant
//                        tc.limitPrice = 0; // irrelevant
//                        tc.maxShowQuantity = 0; // irrelevant
                tc.timeInForce = TRADE_TIMEINFORCE_INMED_OR_CANCEL; // irrelevant
//                        tc.seconds = 0; // irrelevant
//                        tc.mseconds = 0; // irrelevant
//                        tc.expiration = XX; // irrelevant
                tc.unconditional = true; // irrelevant
                tc.checkWorstCase = false; // irrelevant
                tc.reservedData = 0; // irrelevant

                tc.finishedPrice = memMap->AccountingUnit[auIndex].SecurityExposureInt[pbIndex][s].price;
                tc.finishedQuantity = tc.quantity;

                anyPosiToClose=true;
                if(contextTradeAccount(memMap->AccountingUnit[auIndex].auId, &tc, tID)!=OK) {

                    sprintf(macro_command_output_msg, "Error closing position in %s", securityName[s]);
                    return false;

                } // if contextTradeAccount failed

            } // if security exposure != 0

        } // for securities

        if(!anyPosiToClose) {

            for(int a=0; a<NUM_ASSETS; a++) {

                if(contextCashClean(pbID, memMap->AccountingUnit[auIndex].auId, a)!=OK) {

                    sprintf(macro_command_output_msg, "Error cleaning cash in %s", assetName[a]);
                    return false;

                } // if contextCashClean failed

            } // for assets

            sprintf(macro_command_output_msg, "Success");
            __macro_command_nothing_left_to_wipe = true;
            return true;

        } // if not any pending posis to close

    } // if not any pending orders to resolve

    __macro_command_nothing_left_to_wipe = false;
    return true;

}







/*********************************************************************************/
/* API FUNCTIONS IMPLEMENTATION                                                  */
/*********************************************************************************/

boolean macro_command_hedge_security_in_AU(sharedMemory_t* memMap, idtype security, int32 finalIntendedOffset, boolean useMarketOrders, int32 additionalLeewayToLimit)
{
    int32 auIndex = memMap->strAuIndex;

    if(macro_command_in_progress && !__macro_command_relaunched) {
        add_message("!!! Macro command hedge_security_in_AU called when another macro command was already running", STR_MSG_ERROR);
        return false;
    }

    char msg[SLEN];
    int32 quantityToHedge=finalIntendedOffset-(int32)memMap->AccountingUnit[auIndex].GblSecurityExposureAUInt[security].amount;
    if(!__macro_command_relaunched) {
        macro_command_type = MACRO_COMMAND_HEDGE_SECURITY_AU;
        macro_command_in_progress = true;
        __macro_command_start_time=CALLBACK_systime;
        __macro_command_last_time_w_quotes=CALLBACK_systime;
        __macro_command_finalIntendedOffset=finalIntendedOffset;
        __macro_command_hedging_security=security;
        __macro_command_initial_index_of_historic=memMap->AccountingUnit[auIndex].indexOfHistoric;
        __macro_command_use_market_orders=useMarketOrders;
        __macro_command_additional_leeway_to_limit=additionalLeewayToLimit;

        sprintf(msg, "@@@ Macro command hedge_security_in_AU called on %s: current security exposure is %d, finalIntendedOffset is %d, therefore trading amount is %d",
                securityName[security], (int32) memMap->AccountingUnit[auIndex].GblSecurityExposureAUInt[security].amount, finalIntendedOffset, quantityToHedge);
//        add_message(msg, STR_MSG_TRADING_INFO);
        stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);

    }


    if(abs(quantityToHedge)<minimum_order_size) {

        sprintf(macro_command_output_msg, "No hedging is necessary (current imbalance is %d, vs a minimum of %d)", quantityToHedge, minimum_order_size);
        sprintf(msg, "@@@ Macro command hedge_security_in_AU finished: %s", macro_command_output_msg);
//        add_message(msg,STR_MSG_ERROR);
        stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
        macro_command_status = MACRO_COMMAND_STATUS_SUCCEEDED;
        macro_command_in_progress = false;
//        macro_command_failures = 0; // Commented, as we did nothing really
        __macro_command_relaunched = false;

    } else {

        boolean anyTradeSent=
                send_best_immediate_order_PB_TI(memMap, security, quantityToHedge,
                                                false, 0, false, 0, (useMarketOrders ? MACRO_COMMAND_MARKET_HEDGING_RESERVED_DATA_ID : MACRO_COMMAND_LIMIT_HEDGING_RESERVED_DATA_ID),
                                                true, true,
                                                useMarketOrders, additionalLeewayToLimit,
                                                true, maximum_hedge_size);

        if(!anyTradeSent) {

            if(!__macro_command_relaunched) {

                stubDbg(DEBUG_INFO_LEVEL, "@@@ No tradeable quotes - now waiting for a while\n");

            } else {

                stubDbg(DEBUG_INFO_LEVEL, "@@@ No tradeable quotes (again, we keep waiting)\n");

            }

            if(my_getTimeDifSigned(&__macro_command_last_time_w_quotes, &CALLBACK_systime) > MACRO_COMMAND_MAX_WAIT_TIME_FOR_QUOTES) {

                sprintf(macro_command_output_msg, "No tradeable quotes"); // we overwrite macro_command_output_msg (previously set by send_best_immediate_order_PB_TI)
                sprintf(msg, "!!! Macro command hedge_security_in_AU failed: %s", macro_command_output_msg);
                add_message(msg,STR_MSG_ERROR);
//                stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
                macro_command_status = MACRO_COMMAND_STATUS_FAILED;
                macro_command_failures++;
                macro_command_in_progress=false;
                __macro_command_relaunched=false;
            }

        }

    }

    return true;

}



boolean macro_command_hedge_all_securities_in_AU(sharedMemory_t* memMap)
{
    int32 auIndex = memMap->strAuIndex;

    if(macro_command_in_progress && !__macro_command_relaunched) {
        add_message("!!! Macro command hedge_all_securities_in_AU called when another macro command was already running", STR_MSG_ERROR);
        return false;
    }

    char msg[SLEN];

    if(!__macro_command_relaunched) {
        macro_command_type = MACRO_COMMAND_HEDGE_ALL_SECURITIES_AU;
        macro_command_in_progress = true;
        __macro_command_start_time=CALLBACK_systime;
        __macro_command_last_time_w_quotes=CALLBACK_systime;
        __macro_command_initial_index_of_historic=memMap->AccountingUnit[auIndex].indexOfHistoric;

        sprintf(msg, "@@@ Macro command hedge_all_securities_in_AU called");
//        add_message(msg, STR_MSG_TRADING_INFO);
        stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);

    }

    boolean anythingToHedge=false;
    boolean anyTradeSent=false;

    for(int security=1; security<NUM_SECURITIES; security++) {

        int32 currentExp=memMap->AccountingUnit[auIndex].GblSecurityExposureAUInt[security].amount;

        if(abs(currentExp)>=minimum_order_size) {

            anythingToHedge=true;
            if(send_best_immediate_order_PB_TI(memMap, security, -currentExp,
                                               false, 0, false, 0, MACRO_COMMAND_MARKET_HEDGING_RESERVED_DATA_ID,
                                               true, true,
                                               true, 0,
                                               true,maximum_hedge_size))
                anyTradeSent=true;

        } // currentExp >= minimum order size

    } // Loop: securities

    if(!anythingToHedge) {

        sprintf(macro_command_output_msg, "No hedging is necessary");
        sprintf(msg, "@@@ Macro command hedge_all_securities_in_AU finished: %s", macro_command_output_msg);
//        add_message(msg,STR_MSG_ERROR);
        stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
        macro_command_status = MACRO_COMMAND_STATUS_SUCCEEDED;
        macro_command_in_progress = false;
//        macro_command_failures = 0; // We did nothing really
        __macro_command_relaunched = false;

    } // nothing to hedge

    if(!anyTradeSent) {

        if(!__macro_command_relaunched) {

            stubDbg(DEBUG_INFO_LEVEL, "@@@ No tradeable quotes - now waiting for a while\n");

        } else {

            stubDbg(DEBUG_INFO_LEVEL, "@@@ No tradeable quotes (again, we keep waiting)\n");

        }

        if(my_getTimeDifSigned(&__macro_command_last_time_w_quotes, &CALLBACK_systime) > MACRO_COMMAND_MAX_WAIT_TIME_FOR_QUOTES) {

            sprintf(macro_command_output_msg, "No tradeable quotes"); // we overwrite macro_command_output_msg (previously set by send_best_immediate_order_PB_TI)
            sprintf(msg, "!!! Macro command hedge_all_securities_in_AU failed: %s", macro_command_output_msg);
            add_message(msg,STR_MSG_ERROR);
//            stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
            macro_command_status = MACRO_COMMAND_STATUS_FAILED;
            macro_command_failures++;
            macro_command_in_progress=false;
            __macro_command_relaunched=false;
        }

    }

    return true;

}


boolean macro_command_hedge_security_in_EP(sharedMemory_t* memMap, idtype security, int32 finalIntendedOffset, boolean useMarketOrders, int32 additionalLeewayToLimit)
{

    int32 auIndex = memMap->strAuIndex;
    int32 epIndex = memMap->strEpIndex;

    if(macro_command_in_progress && !__macro_command_relaunched) {
        add_message("!!! Macro command hedge_security_in_EP called when another macro command was already running", STR_MSG_ERROR);
        return false;
    }

    int32 exposureEP;
    exposureEP = (int32)memMap->EquityPool[epIndex].GblSecurityExposureEPInt[security].amount;

    char msg[SLEN];
    int32 quantityToHedge=finalIntendedOffset-exposureEP;
    if(!__macro_command_relaunched) {
        macro_command_type = MACRO_COMMAND_HEDGE_SECURITY_EP;
        macro_command_in_progress = true;
        __macro_command_start_time=CALLBACK_systime;
        __macro_command_last_time_w_quotes=CALLBACK_systime;
        __macro_command_finalIntendedOffset=finalIntendedOffset;
        __macro_command_hedging_security=security;
        __macro_command_initial_index_of_historic=memMap->AccountingUnit[auIndex].indexOfHistoric;
        __macro_command_use_market_orders=useMarketOrders;
        __macro_command_additional_leeway_to_limit=additionalLeewayToLimit;

        sprintf(msg, "@@@ Macro command hedge_security_in_EP called on %s: current security exposure is %d, finalIntendedOffset is %d, therefore trading amount is %d",
                securityName[security], exposureEP, finalIntendedOffset, quantityToHedge);
//        add_message(msg, STR_MSG_TRADING_INFO);
        stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);

    }


    if(abs(quantityToHedge)<minimum_order_size) {


        sprintf(msg, "@@@ Macro command hedge_security_in_EP finished: %s", macro_command_output_msg);
//        add_message(msg,STR_MSG_ERROR);
        stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
        macro_command_status = MACRO_COMMAND_STATUS_SUCCEEDED;
        macro_command_in_progress = false;
        __macro_command_relaunched = false;

    } else {

        boolean anyTradeSent=
                send_best_immediate_order_PB_TI(memMap, security, quantityToHedge,
                                                false, 0, false, 0, (useMarketOrders ? MACRO_COMMAND_MARKET_HEDGING_RESERVED_DATA_ID : MACRO_COMMAND_LIMIT_HEDGING_RESERVED_DATA_ID),
                                                true, true,
                                                useMarketOrders, additionalLeewayToLimit,
                                                true, maximum_hedge_size);

        if(!anyTradeSent) {

            if(!__macro_command_relaunched) {

                stubDbg(DEBUG_INFO_LEVEL, "@@@ No tradeable quotes - now waiting for a while\n");

            } else {

                stubDbg(DEBUG_INFO_LEVEL, "@@@ No tradeable quotes (again, we keep waiting)\n");

            }

            if(my_getTimeDifSigned(&__macro_command_last_time_w_quotes, &CALLBACK_systime) > MACRO_COMMAND_MAX_WAIT_TIME_FOR_QUOTES) {

                sprintf(macro_command_output_msg, "No tradeable quotes"); // we overwrite macro_command_output_msg (previously set by send_best_immediate_order_PB_TI)
                sprintf(msg, "!!! Macro command hedge_security_in_EP failed: %s", macro_command_output_msg);
                add_message(msg,STR_MSG_ERROR);
//                stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
                macro_command_status = MACRO_COMMAND_STATUS_FAILED;
                macro_command_failures++;
                macro_command_in_progress=false;
                __macro_command_relaunched=false;
            }

        }

    }

    return true;

}



boolean macro_command_hedge_all_securities_in_EP(sharedMemory_t* memMap)
{
    int32 auIndex = memMap->strAuIndex;
    int32 epIndex = memMap->strEpIndex;

    if(macro_command_in_progress && !__macro_command_relaunched) {
        add_message("!!! Macro command hedge_all_securities_in_EP called when another macro command was already running", STR_MSG_ERROR);
        return false;
    }

    char msg[SLEN];

    if(!__macro_command_relaunched) {
        macro_command_type = MACRO_COMMAND_HEDGE_ALL_SECURITIES_EP;
        macro_command_in_progress = true;
        __macro_command_start_time=CALLBACK_systime;
        __macro_command_last_time_w_quotes=CALLBACK_systime;
        __macro_command_initial_index_of_historic=memMap->AccountingUnit[auIndex].indexOfHistoric;

        sprintf(msg, "@@@ Macro command hedge_all_securities_in_EP called");
//        add_message(msg, STR_MSG_TRADING_INFO);
        stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);

    }

    boolean anythingToHedge=false;
    boolean anyTradeSent=false;

    for(int security=1; security<NUM_SECURITIES; security++) {

        int32 currentExposureEP;
        currentExposureEP = (int32)memMap->EquityPool[epIndex].GblSecurityExposureEPInt[security].amount;

        if(abs(currentExposureEP)>=minimum_order_size) {

            anythingToHedge=true;
            if(send_best_immediate_order_PB_TI(memMap, security, -currentExposureEP,
                                               false, 0, false, 0, MACRO_COMMAND_MARKET_HEDGING_RESERVED_DATA_ID,
                                               true, true,
                                               true, 0,
                                               true, maximum_hedge_size))
                anyTradeSent=true;

        } // currentExp >= minimum order size

    } // Loop: securities

    if(!anythingToHedge) {

        sprintf(macro_command_output_msg, "No hedging is necessary");
        sprintf(msg, "@@@ Macro command hedge_all_securities_in_EP finished: %s", macro_command_output_msg);
//        add_message(msg,STR_MSG_ERROR);
        stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
        macro_command_status = MACRO_COMMAND_STATUS_SUCCEEDED;
        macro_command_in_progress = false;
//        macro_command_failures = 0; // We did nothing really
        __macro_command_relaunched = false;

    } // nothing to hedge

    if(!anyTradeSent) {

        if(!__macro_command_relaunched) {

            stubDbg(DEBUG_INFO_LEVEL, "@@@ No tradeable quotes - now waiting for a while\n");

        } else {

            stubDbg(DEBUG_INFO_LEVEL, "@@@ No tradeable quotes (again, we keep waiting)\n");

        }

        if(my_getTimeDifSigned(&__macro_command_last_time_w_quotes, &CALLBACK_systime) > MACRO_COMMAND_MAX_WAIT_TIME_FOR_QUOTES) {

            sprintf(macro_command_output_msg, "No tradeable quotes"); // we overwrite macro_command_output_msg (previously set by send_best_immediate_order_PB_TI)
            sprintf(msg, "!!! Macro command hedge_all_securities_in_EP failed: %s", macro_command_output_msg);
            add_message(msg,STR_MSG_ERROR);
//            stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
            macro_command_status = MACRO_COMMAND_STATUS_FAILED;
            macro_command_failures++;
            macro_command_in_progress=false;
            __macro_command_relaunched=false;
        }

    }

    return true;

}



boolean macro_command_cancel_all_pending_orders_in_AU (sharedMemory_t* memMap) {

    int32 auIndex = memMap->strAuIndex;

    if(macro_command_in_progress) {
        add_message("!!! Macro command cancel_all_pending_orders_in_AU called when another macro command was already running", STR_MSG_ERROR);
        return false;
    }

    __macro_command_start_time=CALLBACK_systime;
    __macro_command_number_of_cancel_tries=0;
    __macro_command_last_cancel_sent_when=CALLBACK_systime;


    char msg[NLEN];
    sprintf(msg, "@@@ Macro command cancel_all_pending_orders_in_AU called (%d trades alive)", memMap->AccountingUnit[auIndex].numberOfAlives);
//    add_message(msg, STR_MSG_TRADING_INFO);
    stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
    macro_command_type = MACRO_COMMAND_CANCEL_ALL_PENDING_IN_AU;

    if(memMap->AccountingUnit[auIndex].numberOfAlives>0) {

        stubDbg (DEBUG_INFO_LEVEL, "@@@ Canceling all orders alive\n");
        sendCancelsToAllPendingOrders(memMap, auIndex);
        macro_command_in_progress = true;
        __macro_command_relaunched = false;

    } // numberOfAlives > 0
    else {

        sprintf(macro_command_output_msg, "No trades alive");
//        add_message("@@@ Macro command cancel_all_pending_orders_in_AU finished (no trades alive were found)",STR_MSG_TRADING_INFO);
        stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
        macro_command_status = MACRO_COMMAND_STATUS_SUCCEEDED;
//        macro_command_failures = 0; // We really did nothing
        macro_command_in_progress = false; // redundant
        __macro_command_relaunched = false;

    } // numberOfAlives == 0

    return true;

}



boolean macro_command_close_all_posis_in_AU (sharedMemory_t* memMap) {

    int32 auIndex = memMap->strAuIndex;

    char msg[MLEN];
    if(macro_command_in_progress && !__macro_command_relaunched) {
        add_message("!!! Macro command close_all_posis_in_AU called when another macro command was already running", STR_MSG_ERROR);
        return false;
    }


    if(!__macro_command_relaunched) {

        macro_command_type = MACRO_COMMAND_CLOSE_ALL_POSIS_IN_AU;
        macro_command_in_progress = true;
        __macro_command_start_time=CALLBACK_systime;
        __macro_command_last_time_w_quotes=CALLBACK_systime;
        sprintf(msg, "@@@ Macro command close_all_posis_in_AU called");
//        add_message(msg, STR_MSG_TRADING_INFO);
        stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
        __macro_command_initial_index_of_historic=memMap->AccountingUnit[auIndex].indexOfHistoric;

    }

    boolean anyPosiToClose=false;
    boolean anyTradeSucceeded=false;

    for(int sec=0; sec<NUM_SECURITIES; sec++) {

//        boolean anyPosiToCloseInThisSecurity=false;

        for(int pb=0; pb<memMap->nPrimeBrokers; pb++) {

            if(abs(memMap->AccountingUnit[auIndex].SecurityExposureInt[pb][sec].amount)>=minimum_order_size) {

                int32 quantityToClose=-memMap->AccountingUnit[auIndex].SecurityExposureInt[pb][sec].amount;
                anyPosiToClose=true;
//                anyPosiToCloseInThisSecurity=true;
                sprintf(msg, "@@@ (in close_all_posis_in_AU) Closing %s position on PB #%d (%s), current exposure is %d",
                        securityName[sec], pb, memMap->primeBrokerName[pb], memMap->AccountingUnit[auIndex].SecurityExposureInt[pb][sec].amount);
//                add_message(msg, STR_MSG_TRADING_INFO);
                stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);

                anyTradeSucceeded=
                        send_best_immediate_order_PB_TI(memMap, sec, quantityToClose,
                                                        true, pb, false, 0, MACRO_COMMAND_CLOSE_ALL_RESERVED_DATA_ID,
                                                        false, false,
                                                        true, 0,
                                                        false, 0);

            } // if exposure > minimum order size

        } // loop prime brokers

//        if(anyPosiToCloseInThisSecurity) print_prices_full_book(memMap, sec);

    } // loop (securities)

    if(!anyPosiToClose) {

        sprintf(macro_command_output_msg, "No more posis to close");
        sprintf(msg, "@@@ (in close_all_posis_in_AU) Macro command happily finished: %s", macro_command_output_msg);
//        add_message(msg, STR_MSG_TRADING_INFO);
        stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
        macro_command_status = MACRO_COMMAND_STATUS_SUCCEEDED;
        // macro_command_failures = 0; // We really did nothing
        macro_command_in_progress = false;
        __macro_command_relaunched = false;

    } // Posis to close
    else if(!anyTradeSucceeded) {

        if(!__macro_command_relaunched) {

            stubDbg(DEBUG_INFO_LEVEL, "@@@ Macro command launched, but no tradeable quotes - now waiting for a while\n");

        } else {

            stubDbg(DEBUG_INFO_LEVEL, "@@@ No tradeable quotes (again, we keep waiting)\n");

        }

        if(my_getTimeDifSigned(&__macro_command_last_time_w_quotes, &CALLBACK_systime) > MACRO_COMMAND_MAX_WAIT_TIME_FOR_QUOTES) {

            sprintf(macro_command_output_msg, "No tradeable quotes"); // we overwrite macro_command_output_msg (previously set by send_best_immediate_order_PB_TI)
            sprintf(msg, "!!! Macro command close_all_posis_in_AU failed: %s", macro_command_output_msg);
            add_message(msg,STR_MSG_ERROR);
//            stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
            macro_command_status = MACRO_COMMAND_STATUS_FAILED;
            macro_command_failures++;
            macro_command_in_progress = false;
            __macro_command_relaunched = false;

        }

    } // Not a single trade succeeded

    return true;

}




boolean macro_command_wipe_PB_from_AU(sharedMemory_t* memMap, idtype pbIndex) {

    int32 auIndex = memMap->strAuIndex;

    if(macro_command_in_progress) {
//        add_message("!!! Macro command wipe_PB_from_AU called when another macro command was already running", STR_MSG_ERROR);
        return false;
    }

    __macro_command_start_time=CALLBACK_systime;
    __macro_command_PB_to_wipe=pbIndex;
    macro_command_type = MACRO_COMMAND_WIPE_PB_FROM_AU;

    char msg[MLEN];

//    add_message("@@@ Macro command wipe_PB_from_AU called", STR_MSG_TRADING_INFO);

    if(do_wipe_PB_from_AU(memMap, pbIndex, auIndex)) {

        if(__macro_command_nothing_left_to_wipe) {

            sprintf(msg, "@@@ Macro command wipe_PB_from_AU finished: %s", macro_command_output_msg);
//            add_message(msg,STR_MSG_TRADING_INFO);
            stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
            macro_command_status = MACRO_COMMAND_STATUS_SUCCEEDED;
            // macro_command_failures = 0; // We really didn't do much
            macro_command_in_progress = false; // redundant

        } // nothing left to wipe
        else {

            sprintf(msg, "@@@ Macro command wipe_PB_from_AU successfully launched");
//            add_message("@@@ Macro command wipe_PB_from_AU successfully launched", STR_MSG_TRADING_INFO);
            stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
            macro_command_in_progress = true;

        } // command launched as there were things to wipe

    } // do_wipe_PB_from_AU succeeded
    else {

        sprintf(msg, "!!! Macro command wipe_PB_from_AU failed: %s", macro_command_output_msg);
//        add_message(msg,STR_MSG_ERROR);
        stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
        macro_command_status = MACRO_COMMAND_STATUS_FAILED;
        macro_command_failures++;
        macro_command_in_progress = false;

    } // do_wipe_PB_from_AU failed

    return true;

}



/*********************************************************************************/
/* NON-USER CALLABLE API FUNCTIONS (PROCESS AND INIT)                            */
/*********************************************************************************/


boolean process_macro_commands(sharedMemory_t* memMap)
{
    int32 auIndex = memMap->strAuIndex;
    int32 epIndex = memMap->strEpIndex;

    boolean result = OK;
    char msg[MLEN];

    if(macro_command_in_progress) {

        int32 delay = my_getTimeDifSigned(&__macro_command_start_time, &CALLBACK_systime);

        if(delay >= MACRO_COMMAND_TIMEOUT_THRESHOLD) {

            char whatCommand[NLEN];
            switch(macro_command_type) {

                case MACRO_COMMAND_HEDGE_SECURITY_AU :
                    sprintf(whatCommand, "hedge_security_AU"); break;
                case MACRO_COMMAND_HEDGE_ALL_SECURITIES_AU :
                    sprintf(whatCommand, "hedge_all_securities_in_AU"); break;
                case MACRO_COMMAND_HEDGE_SECURITY_EP :
                    sprintf(whatCommand, "hedge_security_EP"); break;
                case MACRO_COMMAND_HEDGE_ALL_SECURITIES_EP :
                    sprintf(whatCommand, "hedge_all_securities_in_EP"); break;
                case MACRO_COMMAND_CLOSE_ALL_POSIS_IN_AU :
                    sprintf(whatCommand, "close_all_posis_in_AU"); break;
                case MACRO_COMMAND_CANCEL_ALL_PENDING_IN_AU :
                    sprintf(whatCommand, "cancel_all_pending_in_AU"); break;
                default :
                    sprintf(whatCommand, "??? Unknown macro command!");

            }

            sprintf(macro_command_output_msg, "macro command timed out and failed (after %d us)", delay);
            sprintf(msg, "??? Macro command [%s] failed: %s", whatCommand, macro_command_output_msg);
            add_message(msg, STR_MSG_CRITICAL);
            macro_command_status = MACRO_COMMAND_STATUS_FAILED;
            macro_command_failures++;
            macro_command_in_progress = false;
            __macro_command_relaunched = false;

            if(macro_command_failures>=n_macro_command_failures_to_disconnect) { // This is not working well

                MalfunctionAlarm=true;
                sprintf(MalfunctionAlarmText, "Too many macro command time out failures");
                add_message("??? Too many macro command time out failures - now disconnecting the strategy (system malfunction)", STR_MSG_CRITICAL);

                stubDbg(DEBUG_INFO_LEVEL, "@@@ Now printing out list of trades alive, if any:\n");

                int tradesProcessed=0;
                for(int t=0; (t<MAX_TRADES_LIST_ALIVE) && (tradesProcessed<memMap->AccountingUnit[auIndex].numberOfAlives); t++) {

                    if(memMap->AccountingUnit[auIndex].alive[t].alive) {

                        tradeElement_t *thisTrade=&(memMap->AccountingUnit[auIndex].alive[t]);
                        tradeCommand_t *thisTradeCommand=&(thisTrade->params);
                        idtype thisTI=memMap->tradingInterfaceIndex[thisTradeCommand->tiId];
                        char typeS[NLEN];
                        getTradeTypeString(thisTradeCommand->tradeType, typeS);
                        char timeInForceS[NLEN];
                        getTimeInForceString(thisTradeCommand->timeInForce, timeInForceS);
                        char statusS[NLEN];
                        getTradeStatusString(thisTrade->info.status, thisTrade->info.substatus, statusS);
                        stubDbg(DEBUG_INFO_LEVEL, "--- Trade %s, TI #%d (%s), %d %s %s, %s, limit = %d, %s, status = %s, filled %d @ %d, reserved = %u\n",
                                                  thisTrade->ids.fixId,
                                                  thisTI,
                                                  memMap->tradingInterfaceName[thisTI],
                                                  thisTradeCommand->quantity,
                                                  securityName[thisTradeCommand->security],
                                                  (thisTradeCommand->side == TRADE_SIDE_BUY ? "BUY" : "SELL"),
                                                  typeS,
                                                  thisTradeCommand->limitPrice,
                                                  timeInForceS,
                                                  statusS,
                                                  thisTradeCommand->finishedQuantity,
                                                  thisTradeCommand->finishedPrice,
                                                  thisTradeCommand->reservedData
                                );

                    } // if trade alive

                } // for trades alive

//                auto_management_connected=false;
                strategy_connected=false;

            } // if too many macro command failures

            return result;

        } // if timeout

        boolean atLeastOneTradeStillAlive=false;
        int tradesProcessed=0;
        switch(macro_command_type) {

            case MACRO_COMMAND_HEDGE_SECURITY_AU :
            case MACRO_COMMAND_HEDGE_ALL_SECURITIES_AU :
            case MACRO_COMMAND_HEDGE_SECURITY_EP :
            case MACRO_COMMAND_HEDGE_ALL_SECURITIES_EP :
            case MACRO_COMMAND_CLOSE_ALL_POSIS_IN_AU :

                for(int t=0; (t<MAX_TRADES_LIST_ALIVE) && (tradesProcessed<memMap->AccountingUnit[auIndex].numberOfAlives); t++) {
                    if(memMap->AccountingUnit[auIndex].alive[t].alive) {
                        tradesProcessed++;

                        if(
                                (memMap->AccountingUnit[auIndex].alive[t].params.reservedData==MACRO_COMMAND_MARKET_HEDGING_RESERVED_DATA_ID) ||
                                (memMap->AccountingUnit[auIndex].alive[t].params.reservedData==MACRO_COMMAND_LIMIT_HEDGING_RESERVED_DATA_ID) ||
                                (memMap->AccountingUnit[auIndex].alive[t].params.reservedData==MACRO_COMMAND_CLOSE_ALL_RESERVED_DATA_ID)
                           ) {

                            atLeastOneTradeStillAlive=true;
                            break;

                        }

                    }

                }

                if(!atLeastOneTradeStillAlive) {
                    // All trades should be finished, so we should look for them in the historical trades list

                    for(int t=__macro_command_initial_index_of_historic;
                        t<memMap->AccountingUnit[auIndex].indexOfHistoric  + (__macro_command_initial_index_of_historic > memMap->AccountingUnit[auIndex].indexOfHistoric ? MAX_TRADES_LIST_HISTORIC : 0);
                        t++) {

                        int hIndex=t % MAX_TRADES_LIST_HISTORIC;
                        tradeInfo_t *thisTradeInfo = &(memMap->AccountingUnit[auIndex].historic[hIndex].info);
                        tradeCommand_t *thisTradeParams = &(memMap->AccountingUnit[auIndex].historic[hIndex].params);
                        idtype tiIndex=memMap->tradingInterfaceIndex[thisTradeParams->tiId];
                        idtype security=thisTradeParams->security;
                        boolean isBuy = (thisTradeParams->side == TRADE_SIDE_BUY);

                        if(
                                (thisTradeParams->reservedData!=MACRO_COMMAND_MARKET_HEDGING_RESERVED_DATA_ID) ||
                                (thisTradeParams->reservedData!=MACRO_COMMAND_LIMIT_HEDGING_RESERVED_DATA_ID) ||
                                (thisTradeParams->reservedData!=MACRO_COMMAND_CLOSE_ALL_RESERVED_DATA_ID)
                           ) continue;

                        // Ok, so this trade was launched by this macro command - now we check what happened in reality

                        if(
                                (thisTradeInfo->status != TRADE_STATE_EXECUTED) ||
                                (thisTradeParams->finishedQuantity != thisTradeParams->quantity)
                           ) {

                            stubDbg(DEBUG_INFO_LEVEL, "--- Trade from macro command finished on TI #%d (%s), and failed (only %d out of %d was filled in %s)\n",
                                    tiIndex, memMap->tradingInterfaceName[tiIndex], thisTradeParams->finishedQuantity, thisTradeParams->quantity, securityName[security]);
                            // Trade has failed :(
                            // Now marking as failed the quotes we didn't trade

                            for(int q=0; q<__macro_command_number_of_marked_quotes[tiIndex][security]; q++) {

                                int thisQuoteIndex = __macro_command_markedQuoteIndexes[tiIndex][security][q];
                                mtbookElement_t *thisQuote = (isBuy ?
                                                                    &(memMap->mapFBQuotesAsk[tiIndex][security][thisQuoteIndex]) :
                                                                    &(memMap->mapFBQuotesBid[tiIndex][security][thisQuoteIndex]));
                                quoteTradingHistory_t *thisQuoteTradingHistory = (isBuy ?
                                                                                     &(quotesTradingHistoryAsk[tiIndex][security][thisQuoteIndex]) :
                                                                                     &(quotesTradingHistoryBid[tiIndex][security][thisQuoteIndex]));
                                if(
                                        (thisQuote->timestamp.sec==thisQuoteTradingHistory->currentQuoteTimestamp.sec) &&
                                        (thisQuote->timestamp.usec==thisQuoteTradingHistory->currentQuoteTimestamp.usec)
                                   ) {

                                    thisQuoteTradingHistory->currentQuoteFailed=true;
                                    stubDbg(DEBUG_INFO_LEVEL,
                                            "--- Marking quote #%d of TI #%d (%s) w timestamp %d:%d as failed\n",
                                            q,
                                            tiIndex,
                                            memMap->tradingInterfaceName[tiIndex],
                                            thisQuote->timestamp.sec,
                                            thisQuote->timestamp.usec
                                           );

                                }

                            } // for loop in market quotes list

                            // We also update yellow and red cards
                            tiOperation[tiIndex].badQuotesToday++;

                        } // Trade failed
                        else {

                            // Trade succeeded!
                            // Now to reset yellow cards etc.
                            stubDbg(DEBUG_INFO_LEVEL, "--- Trade from macro command finished on TI #%d (%s), and succeeded (executed quantity is %d, %s)\n",
                                    tiIndex, memMap->tradingInterfaceName[tiIndex], thisTradeParams->finishedQuantity, securityName[security]);

                            // We also update yellow and red cards
                            tiOperation[tiIndex].goodQuotesToday++;

                        } // Trade succeeded

                    } // for historical trades list

                    // All trades have finished
                    // Now to check total pending and send best possible trade again if > minimum trade size

                    if(macro_command_type == MACRO_COMMAND_HEDGE_SECURITY_AU) {

                        int32 quantityToHedge=__macro_command_finalIntendedOffset-(int32)memMap->AccountingUnit[auIndex].GblSecurityExposureAUInt[__macro_command_hedging_security].amount;

                        if(abs(quantityToHedge)>minimum_order_size) {
                            stubDbg(DEBUG_INFO_LEVEL, "@@@ All orders finished in macro command, but %d still pending - now calling hedge macro command again\n",quantityToHedge);
                            __macro_command_relaunched=true;
                            macro_command_hedge_security_in_AU(memMap, __macro_command_hedging_security, __macro_command_finalIntendedOffset,
                                                               __macro_command_use_market_orders, __macro_command_additional_leeway_to_limit);

                        } else {
                        // if no pending amount, then command is happily finished
                            sprintf(macro_command_output_msg, "Success");
                            sprintf(msg, "@@@ Macro command hedge_security_in_AU happily finished");
//                            add_message(msg, STR_MSG_TRADING_INFO);
                            stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
                            macro_command_status = MACRO_COMMAND_STATUS_SUCCEEDED;
                            macro_command_failures = 0;
                            macro_command_in_progress = false;
                            __macro_command_relaunched = false;
                        }

                    } // macro_command_type is HEDGE AU
                    else if(macro_command_type == MACRO_COMMAND_HEDGE_ALL_SECURITIES_AU) {

                        boolean anyExposurePending=false;

                        for(int sec=1; sec<NUM_SECURITIES; sec++) {

                            int32 currentExp=memMap->AccountingUnit[auIndex].GblSecurityExposureAUInt[sec].amount;

                            if(abs(currentExp)>minimum_order_size) {
                                stubDbg(DEBUG_INFO_LEVEL, "@@@ All orders finished in macro command, but exposure still exists (at least %d still pending in %s) - now calling hedge macro command again\n",
                                        currentExp, securityName[sec]);
                                __macro_command_relaunched=true;
                                anyExposurePending=true;
                                macro_command_hedge_all_securities_in_AU(memMap);
                                break;

                            }

                        } // Loop: securities

                        if(!anyExposurePending) {
                        // if no pending amount, then command is happily finished
                            sprintf(macro_command_output_msg, "Success");
                            add_message("@@@ Macro command hedge_all_securities_in_AU happily finished", STR_MSG_TRADING_INFO);
                            macro_command_status = MACRO_COMMAND_STATUS_SUCCEEDED;
                            macro_command_failures = 0;
                            macro_command_in_progress = false;
                            __macro_command_relaunched = false;
                        }


                    } // macro_command_type is HEDGE ALL AU
                    else if(macro_command_type == MACRO_COMMAND_HEDGE_SECURITY_EP) {

                        int32 quantityToHedge;
                            quantityToHedge=__macro_command_finalIntendedOffset-(int32)memMap->EquityPool[epIndex].GblSecurityExposureEPInt[__macro_command_hedging_security].amount;

                        if(abs(quantityToHedge)>minimum_order_size) {
                            stubDbg(DEBUG_INFO_LEVEL, "@@@ All orders finished in macro command, but %d still pending - now calling hedge macro command again\n",quantityToHedge);
                            __macro_command_relaunched=true;
                            macro_command_hedge_security_in_EP(memMap, __macro_command_hedging_security, __macro_command_finalIntendedOffset,
                                    __macro_command_use_market_orders, __macro_command_additional_leeway_to_limit);

                        } else {
                        // if no pending amount, then command is happily finished
                            sprintf(macro_command_output_msg, "Success");
                            add_message("@@@ Macro command hedge_security_EP happily finished", STR_MSG_TRADING_INFO);
                            macro_command_status=MACRO_COMMAND_STATUS_SUCCEEDED;
                            macro_command_failures = 0;
                            macro_command_in_progress = false;
                            __macro_command_relaunched = false;
                        }

                    } // macro_command_type is HEDGE AU
                    else if(macro_command_type == MACRO_COMMAND_HEDGE_ALL_SECURITIES_EP) {

                        boolean anyExposurePending=false;

                        for(int sec=1; sec<NUM_SECURITIES; sec++) {

                            int32 currentExpEP;
                            {
                                currentExpEP=__macro_command_finalIntendedOffset-(int32)memMap->EquityPool[epIndex].GblSecurityExposureEPInt[__macro_command_hedging_security].amount;
                            }

                            if(abs(currentExpEP)>minimum_order_size) {
                                stubDbg(DEBUG_INFO_LEVEL, "@@@ All orders finished in macro command, but exposure still exists (at least %d still pending in %s) - now calling hedge macro command again\n",
                                        currentExpEP, securityName[sec]);
                                __macro_command_relaunched=true;
                                anyExposurePending=true;
                                macro_command_hedge_all_securities_in_EP(memMap);
                                break;

                            }

                        } // Loop: securities

                        if(!anyExposurePending) {
                        // if no pending amount, then command is happily finished
                            sprintf(macro_command_output_msg, "Success");
                            add_message("@@@ Macro command hedge_all_securities_in_EP happily finished", STR_MSG_TRADING_INFO);
                            macro_command_status = MACRO_COMMAND_STATUS_SUCCEEDED;
                            macro_command_failures = 0;
                            macro_command_in_progress = false;
                            __macro_command_relaunched = false;
                        }


                    } // macro_command_type is HEDGE ALL EP
                    else if(macro_command_type == MACRO_COMMAND_CLOSE_ALL_POSIS_IN_AU) {

                        boolean anyPosiToClose=false;
                        for(int sec=0; sec<NUM_SECURITIES; sec++) {
                            for(int pb=0; pb<memMap->nPrimeBrokers; pb++) {
                                if(abs(memMap->AccountingUnit[auIndex].SecurityExposureInt[pb][sec].amount)>=minimum_order_size) {
                                    anyPosiToClose=true;
                                    break;
                                }
                            }
                            if(anyPosiToClose) break;
                        }

                        if(anyPosiToClose) {
                            sprintf(msg, "@@@ All orders finished in macro command, but some positions still exist - now calling close all macro command again\n");
                            stubDbg(DEBUG_INFO_LEVEL, msg);
//                            add_message(msg, STR_MSG_TRADING_INFO);
                            __macro_command_relaunched=true;
                            macro_command_close_all_posis_in_AU(memMap);

                        } else {
                        // if no pending amount, then command is happily finished
                            sprintf(macro_command_output_msg, "Success");
                            add_message("@@@ Macro command happily finished", STR_MSG_TRADING_INFO);
                            macro_command_status=MACRO_COMMAND_STATUS_SUCCEEDED;
                            macro_command_failures = 0;
                            macro_command_in_progress = false;
                            __macro_command_relaunched = false;
                        }

                    } // macro_command_type is CLOSE ALL

                } // no trades still alive

                break;

            case MACRO_COMMAND_CANCEL_ALL_PENDING_IN_AU :

                if(memMap->AccountingUnit[auIndex].numberOfAlives>0) {
                    if(__macro_command_number_of_cancel_tries<=MACRO_COMMAND_TOO_MANY_CANCEL_TRIES) {
                        sendCancelsToAllPendingOrders(memMap, auIndex);
                    } else {
                        sprintf(macro_command_output_msg, "Too many cancel tries");
                        sprintf(msg, "!!! Macro command cancel_all_pending_in_AU failed: %s\n", macro_command_output_msg);
                        add_message(msg, STR_MSG_TRADING_INFO);
                        macro_command_status=MACRO_COMMAND_STATUS_FAILED;
                        macro_command_failures++;
                        macro_command_in_progress = false;
                        __macro_command_relaunched = false;
                    }
                } else {
                    sprintf(macro_command_output_msg, "Success");
                    add_message("@@@ Macro command cancel_all_pending_in_AU happily finished\n", STR_MSG_TRADING_INFO);
                    macro_command_status = MACRO_COMMAND_STATUS_SUCCEEDED;
                    macro_command_failures = 0;
                    macro_command_in_progress = false;
                    __macro_command_relaunched = false;
                }

                break;

            case MACRO_COMMAND_WIPE_PB_FROM_AU :

                if(do_wipe_PB_from_AU(memMap, __macro_command_PB_to_wipe, auIndex)) {

                    if(__macro_command_nothing_left_to_wipe) {

                        sprintf(msg, "@@@ Macro command wipe_PB_from_AU finished: %s", macro_command_output_msg);
                        add_message(msg,STR_MSG_TRADING_INFO);
                        macro_command_status = MACRO_COMMAND_STATUS_SUCCEEDED;
                        macro_command_failures = 0;
                        macro_command_in_progress = false;
                        return true;

                    } // nothing left to wipe

                } // do_wipe_PB_from_AU succeeded
                else {

                    sprintf(msg, "!!! Macro command wipe_PB_from_AU failed: %s", macro_command_output_msg);
                    add_message(msg,STR_MSG_ERROR);
                    macro_command_status = MACRO_COMMAND_STATUS_FAILED;
                    macro_command_failures++;
                    macro_command_in_progress=false;
                    return false;

                } // do_wipe_PB_from_AU failed

                break;

            default :
                sprintf(macro_command_output_msg, "Unknown macro command type (%d)", macro_command_type);
                sprintf(msg, "!!! Error processing macro command: %s", macro_command_output_msg);
                add_message(msg, STR_MSG_ERROR);
                macro_command_status=MACRO_COMMAND_STATUS_FAILED;
                macro_command_failures++;
                macro_command_in_progress = false;
                __macro_command_relaunched = false;

        } // switch macro_command_trype

    } // if macro_command_in_progress

    return result;

}


void init_macro_commands(void) {
    macro_command_in_progress = false;
    __macro_command_relaunched = false;
    macro_command_type = MACRO_COMMAND_NO_COMMAND;
    macro_command_status = MACRO_COMMAND_STATUS_NO_STATUS;
    macro_command_failures = 0;
    strcpy(macro_command_output_msg, "");

    for(int ti=0; ti<MAX_NUMBER_TI; ti++) {
        for(int s=0; s<NUM_SECURITIES; s++) {
            for(int q=0; q<MAX_QUOTES; q++) {
//                quotesTradingHistoryAsk[ti][s][q].initialQuoteTimestamp.sec=0;
//                quotesTradingHistoryAsk[ti][s][q].initialQuoteTimestamp.usec=0;
                quotesTradingHistoryAsk[ti][s][q].currentQuoteTimestamp.sec=0;
                quotesTradingHistoryAsk[ti][s][q].currentQuoteTimestamp.usec=0;
//                quotesTradingHistoryAsk[ti][s][q].tradeAttempts=0;
//                quotesTradingHistoryAsk[ti][s][q].failedAttempts=0;
                quotesTradingHistoryAsk[ti][s][q].currentQuoteFailed=false;
            } // Loop: quotes
        }  // Loop: securities
    } // Loop: TIs
}
