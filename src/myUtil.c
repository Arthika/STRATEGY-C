

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>

#include "hftUtils_types.h"
#include "util.h"
#include "trade.h"
#include "map.h"
#include "stubDbg.h"
#include "myUtil.h"
#include "strategy.h"
#include "monitoring.h"
#include "parameters.h"
#include "ti_marks.h"
#include "macro_commands.h"
#include "params.h"

/***************************************************************************************************
* LOCAL DEFINES AND TYPES                                                                          *
***************************************************************************************************/


/***************************************************************************************************
* STATIC DATA                                                                                      *
***************************************************************************************************/

char securityName[NUM_SECURITIES][NLEN];
char assetName[NUM_ASSETS][NLEN];

/***************************************************************************************************
* LOCAL PROTOYPES                                                                                  *
***************************************************************************************************/


/***************************************************************************************************
* DEFINITIONS                                                                                      *
***************************************************************************************************/

boolean isToBPriceObsolete(sharedMemory_t* memMap, idtype tradingInterface, idtype secId, uint8 side) {

    boolean result = false;
    if(side == TRADE_SIDE_BUY) {
        if(getTimeDifToNow (&(memMap->mapTiTbAsk[tradingInterface][secId].timestamp)) >= price_obsolescence_threshold) {
            result = true;
        }
    } else {
        if(getTimeDifToNow (&(memMap->mapTiTbBid[tradingInterface][secId].timestamp)) >= price_obsolescence_threshold) {
            result = true;
        }
    }
    return result;
}


int32 getTradeTypeString (idtype tradeType, char *ptr) {
    int32 result = OK;

    switch (tradeType) {

        case TRADE_TYPE_MARKET : sprintf(ptr, "Market"); break;
        case TRADE_TYPE_LIMIT : sprintf(ptr, "Limit"); break;
        case TRADE_TYPE_ICEBERG : sprintf(ptr, "Iceberg"); break;
        default : sprintf(ptr, "?? Undefined trade type (%d)", tradeType); result = ERROR;

    }

    return result;
}


int32 getTimeInForceString (char timeInForce, char *ptr) {
    int32 result = OK;

    switch (timeInForce) {

        case TRADE_TIMEINFORCE_DAY : sprintf(ptr, "Day"); break;
        case TRADE_TIMEINFORCE_GOOD_TILL_CANCEL : sprintf(ptr, "Good Till Cancel"); break;
        case TRADE_TIMEINFORCE_INMED_OR_CANCEL : sprintf(ptr, "Immediate or Cancel"); break;
        case TRADE_TIMEINFORCE_FILL_OR_KILL : sprintf(ptr, "Fill or Kill"); break;
        case TRADE_TIMEINFORCE_DATE : sprintf(ptr, "Good till Date"); break;
        case TRADE_TIMEINFORCE_GOOD_FOR_SECS : sprintf(ptr, "Good for Seconds"); break;
        case TRADE_TIMEINFORCE_GOOD_FOR_MSECS : sprintf(ptr, "Good for Milliseconds"); break;
        default : sprintf(ptr, "?? Undefined TimeInForce (%d)", timeInForce); result = ERROR;

    }

    return result;
}


int32 getTradeStatusString (idtype tradeStatus, idtype tradeSubStatus, char *ptr) {
    int32 result = OK;

    switch (tradeStatus) {

        // Trades states

        case TRADE_STATE_INVALID : sprintf(ptr, "INVALID (%d)", tradeSubStatus); break;
        case TRADE_STATE_IN_FLUX : sprintf(ptr, "IN FLUX (%d)", tradeSubStatus); break;
        case TRADE_STATE_PENDING : sprintf(ptr, "PENDING (%d)", tradeSubStatus); break;
        case TRADE_STATE_INDETERMINED : sprintf(ptr, "INDETERMINED!!!!! (%d)", tradeSubStatus); break;
        case TRADE_STATE_EXECUTED : sprintf(ptr, "EXECUTED (%d)", tradeSubStatus); break;
        case TRADE_STATE_CANCELED : sprintf(ptr, "CANCELED BY VENUE (%d)", tradeSubStatus); break;
        case TRADE_STATE_REJECTED : sprintf(ptr, "REJECTED (%d)", tradeSubStatus); break;
        case TRADE_STATE_ERROR_SEND : sprintf(ptr, "ERROR SEND (%d)", tradeSubStatus); break;
        case TRADE_STATE_REPLACED_TO_NEW : sprintf(ptr, "REPLACED ID TO NEW (%d)", tradeSubStatus); break;
        case TRADE_STATE_REPLACED_TO_CANCEL : sprintf(ptr, "REPLACED ID TO CANCEL (%d)", tradeSubStatus); break;
        case TRADE_STATE_CANCELED_BY_USER : sprintf(ptr, "CANCELED BY USER (%d)", tradeSubStatus); break;



        default : sprintf(ptr, "?? Undefined trade status (%d - %d)", tradeStatus, tradeSubStatus); result = ERROR;

    }

    return result;
}



int32 getTIStatusString (idtype TIStatus, char *ptr) {

    int32 result = OK;

    switch (TIStatus) {

        // TI states

        case TI_STATUS_NOK : sprintf(ptr, "Not OK"); break;
        case TI_STATUS_OK : sprintf(ptr, "OK"); break;
        case TI_STATUS_OK_NO_PRICE : sprintf(ptr, "OK but no prices"); break;

        default : sprintf(ptr, "?? Undefined TI status (%d)", TIStatus); result = ERROR;
    }

    return result;

}



void renderWithDots(char *where, int64 what, boolean with_sign) {

    strcpy(where, "");

    int sign;
    if(what > 0) sign = 1;
    else if(what == 0) sign = 0;
    else {sign = -1; what = - what;}

    char so_far[NLEN];

    do {

        int64 last_three = what % 1000;
        what /= 1000;
        sprintf(so_far, "%s", where);

        if(!strcmp("", so_far) && (what == 0)) {
            sprintf(where, "%ld", last_three);
        } else if(!strcmp("", so_far) && (what != 0)) {
            sprintf(where, "%03ld", last_three);
        } else if(strcmp("", so_far) && (what == 0)) {
            sprintf(where, "%ld.%s", last_three, so_far);
        } else if(strcmp("", so_far) && (what != 0)) {
            sprintf(where, "%03ld.%s", last_three, so_far);
        } else {
            sprintf(where, "Error in renderWithDots!! %s", so_far);
        }
    } while(what != 0);

    sprintf(so_far, "%s", where);

    if(sign == -1) {
        sprintf(where, "-%s", so_far);
    } else if(with_sign) {
        sprintf(where, "+%s", so_far);
    }


}



void renderPct(char *where, int64 numerator, int64 denominator, boolean with_sign) {

    char sign[5] = "";
    if(numerator >= 0) {
        if(with_sign) sprintf(sign, "+");
    } else {
        sprintf(sign, "-");
        numerator = - numerator;
    }

    if (denominator <= 0) {
        sprintf(where, "N.A.");
        return;
    } else
        sprintf(where, "%s%.1f%%", sign, (100.0 * (number) numerator) / (number) denominator);

}



void renderPrice(char *where, int32 price, int nDecimals) {

    if(price < 0) {
        sprintf(where, "NEG!!");

        char original[NLEN];
        sprintf(original, " [%d]", price);
        strcat(where, original);

        return;
    }

    int32 multiplier = 1;

    for(int i=0; i<nDecimals; i++) {
        multiplier *= 10;
    }

    int32 intPart = price / multiplier;
    int32 decPart = price - (intPart * multiplier);

    char format[SLEN];

    sprintf(format, "%%d.%%0%dd", nDecimals);

    sprintf(where, format, intPart, decPart);

}






boolean isTradeAlive(sharedMemory_t* memMap, idtype tradingInterfaceIndex, uint8 side, uint32 reserved_MIN, uint32 reserved_MAX, int32 *tradeNumber) {

    return isTradeAlive_au(memMap, memMap->strAuIndex, tradingInterfaceIndex, side, reserved_MIN, reserved_MAX, tradeNumber);

}





boolean isTradeAlive_au(sharedMemory_t* memMap, int32 auIndex, idtype tradingInterfaceIndex, uint8 side, uint32 reserved_MIN, uint32 reserved_MAX, int32 *tradeNumber) {

    boolean result = false;
    uint32 orders_processed=0;
    for(int o = 0; (orders_processed<memMap->AccountingUnit[auIndex].numberOfAlives) && (o < MAX_TRADES_LIST_ALIVE); o++) {

        if(memMap->AccountingUnit[auIndex].alive[o].alive) {

            orders_processed++;

            idtype thisTradeTI = memMap->tradingInterfaceIndex[memMap->AccountingUnit[auIndex].alive[o].params.tiId];
            if(
                    (thisTradeTI == tradingInterfaceIndex) &&
                    (memMap->AccountingUnit[auIndex].alive[o].params.side == side) &&
                    (memMap->AccountingUnit[auIndex].alive[o].params.reservedData >= reserved_MIN) &&
                    (memMap->AccountingUnit[auIndex].alive[o].params.reservedData <= reserved_MAX)
              ) {

                result = true;
                if(tradeNumber != NULL) *tradeNumber = o;
                break;

            } // Trade exists

        } // Trade is alive

    } // (for) loop to find trade within the list of trades alive

    return result;

}



void printHistoricalTradeReport(sharedMemory_t* memMap, uint32 whichTrade, char *where) {

    printHistoricalTradeReport_au(memMap, memMap->strAuIndex, whichTrade, where);

}




void printHistoricalTradeReport_au(sharedMemory_t* memMap, int32 auIndex, uint32 whichTrade, char *where) {
    tradeElement_t *thisTrade=&(memMap->AccountingUnit[auIndex].historic[whichTrade]);
    char statusS[SLEN];
    getTradeStatusString (thisTrade->info.status, thisTrade->info.substatus, statusS);
    idtype tiIndex=memMap->tradingInterfaceIndex[thisTrade->params.tiId];
    if(thisTrade->params.tradeType==TRADE_TYPE_MARKET) {
        sprintf(where, "--- MARKET %s %s Order %s on %s was finished, status %s, filled %d @ %d (out of %d @ %d, slippage %d, resvd %u) - exec time %.1fms @ MTtime %s",
                (thisTrade->params.side == TRADE_SIDE_BUY ? "BUY" : "SELL"),
                securityName[thisTrade->params.security],
                thisTrade->ids.fixId,
                memMap->tradingInterfaceName[tiIndex],
                statusS,
                thisTrade->params.finishedQuantity,
                thisTrade->params.finishedPrice,
                thisTrade->params.quantity,
                thisTrade->params.priceAtStart,
                (thisTrade->params.side == TRADE_SIDE_BUY ?
                        (int32) thisTrade->params.finishedPrice - thisTrade->params.priceAtStart :
                        (int32) thisTrade->params.priceAtStart - thisTrade->params.finishedPrice),
                thisTrade->params.reservedData,
                ((double) my_getTimeDif(&(thisTrade->info.timeRxCore), &(thisTrade->info.timeExecuted))) / 1000.0,
                MTtimeStr
           );
    } else {
        char timeInForceStr[NLEN];
        getTimeInForceString(thisTrade->params.timeInForce, timeInForceStr);
        sprintf(where, "--- LIMIT (%s) %s %s Order %s on %s was finished, status is %s, filled %d @ %d (out of %d, limit %d, resvd %u) - exec time %.1fms, @ MT time %s",
                timeInForceStr,
                (thisTrade->params.side == TRADE_SIDE_BUY ? "BUY" : "SELL"),
                securityName[thisTrade->params.security],
                thisTrade->ids.fixId,
                memMap->tradingInterfaceName[tiIndex],
                statusS,
                thisTrade->params.finishedQuantity,
                thisTrade->params.finishedPrice,
                thisTrade->params.quantity,
                thisTrade->params.limitPrice,
                thisTrade->params.reservedData,
                ((double) my_getTimeDif(&(thisTrade->info.timeRxCore), &(thisTrade->info.timeExecuted))) / 1000.0,
                MTtimeStr
           );
    }
}




void printLiveOrderReport(sharedMemory_t* memMap, uint32 whichTrade, char *where) {

    printLiveOrderReport_au(memMap, memMap->strAuIndex, whichTrade, where);

}




void printLiveOrderReport_au(sharedMemory_t* memMap, int32 auIndex, uint32 whichTrade, char *where) {
    tradeElement_t *thisTrade=&(memMap->AccountingUnit[auIndex].alive[whichTrade]);
    char statusS[SLEN];
    getTradeStatusString (thisTrade->info.status, thisTrade->info.substatus, statusS);
    idtype tiIndex=memMap->tradingInterfaceIndex[thisTrade->params.tiId];
    char timeInForceS[NLEN];
    getTimeInForceString(thisTrade->params.timeInForce, timeInForceS);
    if(thisTrade->params.tradeType==TRADE_TYPE_MARKET) {
        sprintf(where, "--- MARKET %s %s Order %s on %s, status is %s, amount is %d, ToB price was %d, timeInForce is %s, reserved was %u) - MT time is %s",
                (thisTrade->params.side == TRADE_SIDE_BUY ? "BUY" : "SELL"),
                securityName[thisTrade->params.security],
                thisTrade->ids.fixId,
                memMap->tradingInterfaceName[tiIndex],
                statusS,
                thisTrade->params.quantity,
                thisTrade->params.priceAtStart,
                timeInForceS,
                thisTrade->params.reservedData,
                MTtimeStr
           );
    } else {
        sprintf(where, "--- LIMIT %s %s Order %s on %s, status is %s, amount is %d, limit is %d, timeInForce is %s, reserved was %u), MT time is %s",
                (thisTrade->params.side == TRADE_SIDE_BUY ? "BUY" : "SELL"),
                securityName[thisTrade->params.security],
                thisTrade->ids.fixId,
                memMap->tradingInterfaceName[tiIndex],
                statusS,
                thisTrade->params.quantity,
                thisTrade->params.limitPrice,
                timeInForceS,
                thisTrade->params.reservedData,
                MTtimeStr
           );
    }
}



boolean enoughMarginToTrade(sharedMemory_t* memMap, char* user, idtype tradingInterfaceIndex, int32 amount, idtype security, uint8 side) {

    if(amount<0) {
        stubDbg(DEBUG_INFO_LEVEL, "!!! Error: called enoughMarginToTrade with negative amount!!\n");
        return false;
    }

/*
    // Ñapa mientras que arreglan lo de la simulación de trades
        idtype pb=memMap->primeBrokerIndex[memMap->whichPrimeBroker[tradingInterfaceIndex]];
        number currentExp=memMap->SecurityExposure[pb][security].exposure;
        number currentAbsExp=abs(currentExp);
        uint8 currentExpSide=currentExp>0 ? TRADE_SIDE_BUY : TRADE_SIDE_SELL;
        idtype baseId, termId;
        getBaseAndTerm(security,&baseId, &termId);
        number applicableLeverageNewPosition = ((side == TRADE_SIDE_BUY) ? memMap->pbml[pb][termId] : memMap->pbml[pb][baseId]);
        number applicableLeverageCurrentPosition = ((currentExpSide == TRADE_SIDE_BUY) ? memMap->pbml[pb][termId] : memMap->pbml[pb][baseId]);
        number marginToFree=0;
        number newMarginNeeded=0;
        if(side != currentExpSide) { // There is margin to free
            if(amount<=currentAbsExp) {
                return true; // For sure this makes the situation better:)
            } else {
                marginToFree=currentAbsExp/applicableLeverageCurrentPosition;
                newMarginNeeded=(amount-currentAbsExp)/applicableLeverageNewPosition;
            }
        } else {
            newMarginNeeded=amount/applicableLeverageNewPosition;
        }
        number currentFreeMargin=memMap->FreeMargin_STR[pb];
        number newFreeMargin=currentFreeMargin+marginToFree-newMarginNeeded;
        boolean result = (newFreeMargin >= 0) || (newFreeMargin>currentFreeMargin);

        stubDbg(DEBUG_INFO_LEVEL, "(in enoughMarginToTrade) simulating %s %d %s on TI #%d (%s), PB #%d (%s): Free margin is %.0f, new free margin is %.0f => %s\n",
                (side == TRADE_SIDE_BUY ? "BUYING" : "SELLING"),
                amount,
                securityName[security],
                tradingInterfaceIndex,
                memMap->tradingInterfaceName[tradingInterfaceIndex],
                pb,
                memMap->primeBrokerName[pb],
                currentFreeMargin,
                newFreeMargin,
                (result ? "Ok to trade" : "NOT Ok to trade")
                );

        return result;
    // Done
*/

    tradeCommand_t tc;

    tc.tiId = memMap->tradingInterface[tradingInterfaceIndex];
    tc.security = security;
    tc.quantity = amount;
    tc.side = side;
    tc.tradeType = TRADE_TYPE_MARKET; // This is irrelevant for simulation purposes
    // tc.limitPrice not needed
    // tc.maxShowQuantity not needed
    tc.timeInForce = TRADE_TIMEINFORCE_INMED_OR_CANCEL; // Again irrelevant
    // tc.seconds not used
    // tc.mseconds not used
    // tc.expiration not used
    tc.unconditional = false; // Same!
    tc.checkWorstCase = false;
    // tc.reservedData not used

    number AUfreemargin;
    number EPfreemargin;
    number PBfreemargin;
    boolean result;
    SimulateTrade(memMap, user, &tc, &AUfreemargin, &EPfreemargin, &PBfreemargin, &result);
/*
    idtype pb=memMap->primeBrokerIndex[memMap->primeBroker[tradingInterfaceIndex]];
    stubDbg(DEBUG_INFO_LEVEL, "Free margin is %.0f, simulated (SU/EP/PB) free margin is %.0f / %.0f / .0f [TI is %d (%s), PB is %d (%s)]\n",
            memMap->FreeMargin_STR[pb], AUfreemargin, EPfreemargin, PBfreemargin,
            tradingInterfaceIndex, memMap->tradingInterfaceName[tradingInterfaceIndex],
            pb, memMap->primeBrokerName[pb]);
*/
    return result;

}




int32 willThisTradeWork(sharedMemory_t* memMap, tradeCommand_t *tc) {

    return willThisTradeWork_au(memMap, memMap->strAuIndex, tc);

}




int32 willThisTradeWork_au(sharedMemory_t* memMap, int32 auIndex, tradeCommand_t *tc) {
/*
#define __everythingOK 0
#define __notEnoughLiquidityInToB 1
#define __isPriceObsolete 2
#define __notEnoughMargin 3
#define __tradingInterfaceNotOK 4
*/

    idtype tiIndex = memMap->tradingInterfaceIndex[tc->tiId];
    idtype security = tc->security;
    idtype amountToSend = tc->quantity;
    mtbookElement_t *bookEntry =
            tc->side == TRADE_SIDE_BUY ? &(memMap->mapTiTbAsk[tiIndex][security]) : &(memMap->mapTiTbBid[tiIndex][security]);

    if(!memMap->tradingInterfaceOk[tiIndex]) {
/*
        stubDbg(DEBUG_INFO_LEVEL, "Skipping TI #%d (%s) since this TI is not OK\n",
                                  tiIndex,
                                  memMap->tradingInterfaceName[tiIndex]
               );
*/
        return __tradingInterfaceNotOK;
    }

    boolean succeeded;

    number AUfreemargin;
    number EPfreemargin;
    number PBfreemargin;

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

    SimulateTrade(memMap, user, tc, &AUfreemargin, &EPfreemargin, &PBfreemargin, &succeeded);

    if(!succeeded) {
/*
        stubDbg(DEBUG_INFO_LEVEL, "Skipping TI #%d (%s) due to insufficient margin (SimulateTrade failed)\n",
                                  tiIndex,
                                  memMap->tradingInterfaceName[tiIndex]
               );
*/
        return __notEnoughMargin;
    }

    if(!isToBPriceObsolete(memMap, tiIndex, security, TRADE_SIDE_BUY)) {
/*
        stubDbg(DEBUG_INFO_LEVEL, "Skipping TI #%d (%s) since this price is obsolete (%d seconds old)\n",
                                  tiIndex,
                                  memMap->tradingInterfaceName[tiIndex],
                                  bookEntry->liquidity,
                                  getTimeDifToNow(&(bookEntry->timestamp))
               );
*/
        return __isPriceObsolete;
    }

    if(bookEntry->liquidity < amountToSend) {
/*
        stubDbg(DEBUG_INFO_LEVEL, "Skipping TI #%d (%s) due to insufficient liquidity (only %d available)\n",
                                  tiIndex,
                                  memMap->tradingInterfaceName[tiIndex],
                                  bookEntry->liquidity
               );
*/
        return __notEnoughLiquidityInToB;
    }

    return __everythingOK;

}


/*
boolean sendBestHedgingTradeLimit(sharedMemory_t* memMap, int32 amount, idtype security, boolean market, int32 purpose, idtype *tradingInterfaceIndex) {
    // If we do this as a blocking function, we should add a returning parameter with the (new) trade index in the alive[] array!

    // returns true if hedge was sent, false otherwise

    stubDbg(DEBUG_INFO_LEVEL, ">> Called sendBestHedgingTradeLimit w amount = %d, in %s, type %s, and purpose %d\n",
                              amount,
                              securityName[security],
                              (market ? "MARKET" : "LIMIT"),
                              purpose
           );

    print_prices(memMap, security);

    int32 amountToSend = roundTrade(amount);
    if(amountToSend != amount) {
        stubDbg(DEBUG_INFO_LEVEL, "  --> Rounding trade as it had an odd size: %d requested, rounded to %d\n",
                                  amount,
                                  amountToSend);
    }

    tradeCommand_t tc;

    // tc.tiId to be set down below;
    tc.security = security;
    tc.quantity = (amountToSend > 0 ? amountToSend : -amountToSend);
    tc.side = (amountToSend > 0 ? TRADE_SIDE_BUY : TRADE_SIDE_SELL);
    tc.tradeType = (market ? TRADE_TYPE_MARKET : TRADE_TYPE_LIMIT);
    // tc.limitPrice to be set down below depending on side
    // tc.maxShowQuantity not needed
    tc.timeInForce = TRADE_TIMEINFORCE_INMED_OR_CANCEL;
    // tc.seconds not used
    // tc.mseconds not used
    // tc.expiration not used
    tc.unconditional = false;
    tc.checkWorstCase = false;

    tc.reservedData = purpose;

    boolean foundit = false; // To be set to true when the resulting TI to send the trade is designated
    idtype resultingTIindex = -1; // This is to store the resulting TI where to send the trade


    if(amountToSend >= 0) { // We have to BUY => we need ask prices

        // We try the prices from the sorting array, considering only non-obsolete ones and with enough depth
        for(int ti = 0; ti < memMap->mapSortAskNumberOfTi[security]; ti++) {

            idtype tiIndex = memMap->mapSortingAsk[security][ti].tiIndex;
            tc.tiId=memMap->tradingInterface[tiIndex];
            tc.limitPrice=memMap->mapTiTbAsk[tiIndex][security].price + leeway_for_limit_hedging;

            if(willThisTradeWork(memMap, &tc) == __everythingOK) {
                foundit = true;
                resultingTIindex = tiIndex;
                break;
            }

        } // for loop over the sorting array

        if(!foundit) {

            // Try again, and now obsolete prices are ok
            for(int tiIndex = 0; tiIndex <= memMap->nTradingInterfaces; tiIndex++) {

                tc.tiId=memMap->tradingInterface[tiIndex];
                tc.limitPrice=memMap->mapTiTbAsk[tiIndex][security].price + leeway_for_limit_hedging;

                if(willThisTradeWork(memMap, &tc) <= __isPriceObsolete) {
                    foundit = true;
                    resultingTIindex = tiIndex;
                    break;
                }

            } // for loop over all available TIs array

        } // foundit

    } else if(amountToSend < 0) { // We have to SELL => we need bid prices

        // i) We try the prices from the sorting array, considering only non-obsolete ones and with enough depth
        for(int ti = 0; ti < memMap->mapSortBidNumberOfTi[security]; ti++) {

            idtype tiIndex = memMap->mapSortingBid[security][ti].tiIndex;
            tc.tiId=memMap->tradingInterface[tiIndex];
            tc.limitPrice=memMap->mapTiTbBid[tiIndex][security].price - leeway_for_limit_hedging;

            if(willThisTradeWork(memMap, &tc) == __everythingOK) {
                foundit = true;
                resultingTIindex = tiIndex;
                break;
            }

        } // for loop over the sorting array

        if(!foundit) {

            // Try again, and now obsolete prices are ok
            for(int tiIndex = 0; tiIndex <= memMap->nTradingInterfaces; tiIndex++) {

                tc.tiId=memMap->tradingInterface[tiIndex];
                tc.limitPrice=memMap->mapTiTbBid[tiIndex][security].price - leeway_for_limit_hedging;

                if(willThisTradeWork(memMap, &tc) <= __isPriceObsolete) {
                    foundit = true;
                    resultingTIindex = tiIndex;
                    break;
                }

            } // for loop over all available TIs array

        } // foundit

    } // AmonuntToSend > or < 0


    if(foundit) {
        boolean succeeded;
        mySendTradeToCore(memMap, &tc, NULL, &succeeded);
        if(succeeded) {

            char limitS[NLEN];
            renderPrice(limitS,tc.limitPrice, 5);
            stubDbg(DEBUG_INFO_LEVEL, ">> Sent new trade to core on TI #%d (%s): %d %s %s\n",
                                      resultingTIindex,
                                      memMap->tradingInterfaceName[resultingTIindex],
                                      tc.quantity,
                                      securityName[tc.security],
                                      (tc.side == TRADE_SIDE_BUY ? "BUY" : "SELL")
                   );
            stubDbg(DEBUG_INFO_LEVEL,">> This was a LIMIT trade with limit price = %s\n", limitS);
            if(tradingInterfaceIndex != NULL) *tradingInterfaceIndex = resultingTIindex;
            return true;

        } else {

            stubDbg(DEBUG_INFO_LEVEL, "!! Called mySendTradeToCore but the trade failed (this should not have happened)\n");
            return false;

        }

    }
    // Whoa - if we are here, that means it has been impossible to send the trade :(
    stubDbg(DEBUG_INFO_LEVEL, "?? No available TI was found to send this hedge, so no trade was actually sent ;-(\n");

    return false;


}
*/



int32 roundTrade(int32 amountToRound) {

    int32 result;
    int32 sign = amountToRound > 0 ? 1 : -1;
    amountToRound *= sign;

    result = (amountToRound / min_trade_step) * min_trade_step;
    if(amountToRound - result > min_trade_step / 2)
        result += min_trade_step;

    result *= sign;

    return result;

}



// These next variables should be in the memMap at some point:

void init_myUtils(sharedMemory_t *memMap) {

    // Init ID arrays:

    // Init security names arrays
    for(int secId = 1; secId < NUM_SECURITIES; secId++) {

        char baseStr[NLEN];
        char termStr[NLEN];

        int32 foundit = getBaseAndTermStr (secId, baseStr, termStr);

        if(OK == foundit) {
            sprintf(securityName[secId],"%s%c%s", baseStr, '/', termStr);
        } else {
            sprintf(securityName[secId],"?? Undefined security (%d)", secId);
        }

    } // secId loop


    // Init asset names arrays
    for(int assetId = 1; assetId < NUM_ASSETS; assetId++) {

        int32 foundit = getAssetString(assetId, assetName[assetId]);

        if(OK != foundit) {
            sprintf(assetName[assetId],"?? Undefined asset (%d)", assetId);
        }

    } // assetId loop


}


void print_prices(sharedMemory_t *memMap, idtype sec, boolean printAsks, boolean printBids) {

    if(printAsks) {

        stubDbg(DEBUG_INFO_LEVEL, "  -- Available ask prices for security #%d (%s):\n", sec, securityName[sec]);

        for(int i=0; i<memMap->mapSortAskNumberOfTi[sec]; i++) {
            idtype ti=memMap->mapSortingAsk[sec][i].tiIndex;
            stubDbg(DEBUG_INFO_LEVEL, "    %d @%d on %s\n",
                    memMap->mapTiTbAsk[ti][sec].liquidity,
                    memMap->mapTiTbAsk[ti][sec].price,
                    memMap->tradingInterfaceName[ti]);
        }

        stubDbg(DEBUG_INFO_LEVEL, "MT time is %s\n", MTtimeStr);

    }


    if(printBids) {

        stubDbg(DEBUG_INFO_LEVEL, "  -- Available bid prices for security #%d (%s):\n", sec, securityName[sec]);

        for(int i=0; i<memMap->mapSortBidNumberOfTi[sec]; i++) {
            idtype ti=memMap->mapSortingBid[sec][i].tiIndex;
            stubDbg(DEBUG_INFO_LEVEL, "    %d @%d on %s\n",
                    memMap->mapTiTbBid[ti][sec].liquidity,
                    memMap->mapTiTbBid[ti][sec].price,
                    memMap->tradingInterfaceName[ti]);
        }

        stubDbg(DEBUG_INFO_LEVEL, "MT time is %s\n", MTtimeStr);

    }

}



void print_prices_full_book(sharedMemory_t *memMap, idtype sec) {

    stubDbg(DEBUG_INFO_LEVEL, "--- Now printing available full book prices for security #%d (%s)\n", sec, securityName[sec]);

    stubDbg(DEBUG_INFO_LEVEL, "  -- Available ask prices:\n");

    for(int q=0; q < memMap->mapFBNumberofsortingAsk[sec]; q++) {
        idtype tiIndex=memMap->mapFBSortingAsk[sec][q].tiIndex;
        int quoteIndex=memMap->mapFBSortingAsk[sec][q].quoteIndex;
        stubDbg(DEBUG_INFO_LEVEL, "    %d @%d on %s%s\n",
                memMap->mapFBQuotesAsk[tiIndex][sec][quoteIndex].liquidity,
                memMap->mapFBQuotesAsk[tiIndex][sec][quoteIndex].price,
                memMap->tradingInterfaceName[tiIndex],
                (
                        (
                                quotesTradingHistoryAsk[tiIndex][sec][quoteIndex].currentQuoteFailed &&
                               (quotesTradingHistoryAsk[tiIndex][sec][quoteIndex].currentQuoteTimestamp.sec == memMap->mapFBQuotesAsk[tiIndex][sec][quoteIndex].timestamp.sec) &&
                               (quotesTradingHistoryAsk[tiIndex][sec][quoteIndex].currentQuoteTimestamp.usec == memMap->mapFBQuotesAsk[tiIndex][sec][quoteIndex].timestamp.usec)
                        ) ?
                                " (BAD QUOTE!)" :
                                ""
                )
               );
    }

    stubDbg(DEBUG_INFO_LEVEL, "  -- Available bid prices:\n");

    for(int q=0; q < memMap->mapFBNumberofsortingBid[sec]; q++) {
        idtype tiIndex=memMap->mapFBSortingBid[sec][q].tiIndex;
        int quoteIndex=memMap->mapFBSortingBid[sec][q].quoteIndex;
        stubDbg(DEBUG_INFO_LEVEL, "    %d @%d on %s%s\n",
                memMap->mapFBQuotesBid[tiIndex][sec][quoteIndex].liquidity,
                memMap->mapFBQuotesBid[tiIndex][sec][quoteIndex].price,
                memMap->tradingInterfaceName[tiIndex],
                (
                        (
                                quotesTradingHistoryBid[tiIndex][sec][quoteIndex].currentQuoteFailed &&
                               (quotesTradingHistoryBid[tiIndex][sec][quoteIndex].currentQuoteTimestamp.sec == memMap->mapFBQuotesBid[tiIndex][sec][quoteIndex].timestamp.sec) &&
                               (quotesTradingHistoryBid[tiIndex][sec][quoteIndex].currentQuoteTimestamp.usec == memMap->mapFBQuotesBid[tiIndex][sec][quoteIndex].timestamp.usec)
                        ) ?
                                " (BAD QUOTE!)" :
                                ""
                )
               );
    }

    stubDbg(DEBUG_INFO_LEVEL, "MT time is %s\n", MTtimeStr);

}




void print_exposures(sharedMemory_t *memMap, idtype sec) {

    print_exposures_au(memMap, memMap->strAuIndex, sec);

}






void print_exposures_au(sharedMemory_t *memMap, int32 auIndex, idtype sec) {

    idtype baseId, termId;
    getBaseAndTerm (sec, &baseId, &termId);
    char baseStr[NLEN], termStr[NLEN];
    getBaseAndTermStr (sec, baseStr, termStr);

    for(int pb=0; pb<memMap->nPrimeBrokers; pb++) {
        stubDbg(DEBUG_INFO_LEVEL, "--- Exposures for PB #%d (%s): free margin is %.2f, security exp (%s) is %.0f, %s exp is %.0f, %s exp is %.0f\n",
                pb,
                memMap->primeBrokerName[pb],
                memMap->AccountingUnit[auIndex].FreeMargin_STR[pb],
                securityName[sec],
                memMap->AccountingUnit[auIndex].SecurityExposure[pb][sec].exposure,
                baseStr,
                memMap->AccountingUnit[auIndex].AssetExposure[pb][baseId],
                termStr,
                memMap->AccountingUnit[auIndex].AssetExposure[pb][termId]
               );
    }

    stubDbg(DEBUG_INFO_LEVEL, "MT time is %s\n", MTtimeStr);

}





boolean areAllPosisClosed(sharedMemory_t *memMap) {

    return areAllPosisClosed_au(memMap, memMap->strAuIndex);

}





boolean areAllPosisClosed_au(sharedMemory_t *memMap, int32 auIndex) {

    if(memMap->AccountingUnit[auIndex].numberOfAlives != 0) return false;

    for(int pb=0; pb<memMap->nPrimeBrokers; pb++) {

        for(int sec=0; sec<NUM_SECURITIES; sec++) {

            if(memMap->AccountingUnit[auIndex].SecurityExposureInt[pb][sec].amount!=0) {

                return false;

            }

        } // for securities

    } // for PBs

    return true;

}




int newest_msg_index = 0;
int oldest_msg_index = 0;
msg_queue_t strategy_messages[MAX_MESSAGES] = {{"",0,{0}}};
char default_message[SLEN] = "ERROR IN STRATEGY MESSAGE!";

int many_messages = 0;


void add_message(char *msg, int type) {

    newest_msg_index = (1 + newest_msg_index) % MAX_MESSAGES;
    if(newest_msg_index == oldest_msg_index) oldest_msg_index = (1 + oldest_msg_index) % MAX_MESSAGES;
    many_messages++;
    if(many_messages > MAX_MESSAGES) many_messages = MAX_MESSAGES;
    strategy_messages[newest_msg_index].type = type;
    strcpy(strategy_messages[newest_msg_index].msg, msg);
    stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
    getStructTimeNowUTC(&(strategy_messages[newest_msg_index].strtime), &(strategy_messages[newest_msg_index].systime));

}


void get_message(int msg_index, msg_queue_t *msg) {

    timestruct_t zerostr = {0};
    timetype_t zerotime = {0};

    if((msg_index >= 0) && (msg_index < MAX_MESSAGES)) {
        int real_index = (newest_msg_index + MAX_MESSAGES - msg_index) % MAX_MESSAGES;
        strcpy(msg->msg, strategy_messages[real_index].msg);
        msg->type = strategy_messages[real_index].type;
        msg->strtime = strategy_messages[real_index].strtime;
        msg->systime = strategy_messages[real_index].systime;
    } else {
        strcpy(msg->msg, default_message);
        msg->type = 0;
        msg->strtime = zerostr;
        msg->systime = zerotime;
    }

}


void clear_all_messages(void) {

    timestruct_t zerostr = {0};
    timetype_t zerotime = {0};

    for(int i = 0; i < MAX_MESSAGES; i++) {
        strcpy(strategy_messages[i].msg, "");
        strategy_messages[i].type = 0;
        strategy_messages[i].strtime = zerostr;
        strategy_messages[i].systime = zerotime;
    }

    many_messages = 0;

}


void render_message(int msg_index, char *where) {

    msg_queue_t msg;
    get_message(msg_index, &msg);
    char line[NLEN];

    switch(msg.type) {
        case STR_MSG_CRITICAL : sprintf(where, "[CRITICAL]"); break;
        case STR_MSG_ERROR : sprintf(where, "[ERROR]"); break;
        case STR_MSG_ALARM : sprintf(where, "[ALARM]"); break;
        case STR_MSG_TRADING_INFO : sprintf(where, "[TRADING INFO]"); break;
        case STR_MSG_SYSTEM_INFO : sprintf(where, "[SYSTEM INFO]"); break;
        case STR_MSG_DEBUG : sprintf(where, "[DEBUG]"); break;
    }

    sprintf(line, " @%04d/%02d/%02d-%02d:%02d:%02d.%03d : ",
                  msg.strtime.year,
                  msg.strtime.month,
                  msg.strtime.day,
                  msg.strtime.hour,
                  msg.strtime.min,
                  msg.strtime.sec,
                  msg.systime.usec / 1000
           );

    strcat(where, line);
    strcat(where, msg.msg);

}


void render_message_HTML(int msg_index, char *where) {

#define COLOR_CRITICAL      "#FF0000"
#define COLOR_ERROR         "#FF0000"
#define COLOR_ALARM         "#FF8800"
#define COLOR_TRADING_INFO  "#00CC00"
#define COLOR_SYSTEM_INFO   "#00CC00"
#define COLOR_DEBUG         "#000000"

    msg_queue_t msg;
    get_message(msg_index, &msg);
    char line[NLEN];

    switch(msg.type) {
        case STR_MSG_CRITICAL : sprintf(where, "<font color=%s>[CRITICAL]</font>", COLOR_CRITICAL); break;
        case STR_MSG_ERROR : sprintf(where, "<font color=%s>[ERROR]</font>", COLOR_ERROR); break;
        case STR_MSG_ALARM : sprintf(where, "<font color=%s>[ALARM]</font>", COLOR_ALARM); break;
        case STR_MSG_TRADING_INFO : sprintf(where, "<font color=%s>[TRADING INFO]</font>", COLOR_TRADING_INFO); break;
        case STR_MSG_SYSTEM_INFO : sprintf(where, "<font color=%s>[SYSTEM INFO]</font>", COLOR_SYSTEM_INFO); break;
        case STR_MSG_DEBUG : sprintf(where, "<font color=%s>[DEBUG]</font>", COLOR_DEBUG); break;
    }

    sprintf(line, " @%04d/%02d/%02d-%02d:%02d:%02d.%03d : ",
                  msg.strtime.year,
                  msg.strtime.month,
                  msg.strtime.day,
                  msg.strtime.hour,
                  msg.strtime.min,
                  msg.strtime.sec,
                  msg.systime.usec / 1000
           );

    strcat(where, line);
    strcat(where, msg.msg);

}


int32 mySendTradeToCore(sharedMemory_t* memMap, char* user, tradeCommand_t* tradeCommand, uint32* tempTradeId, boolean *succeeded) {

    int result = OK;
    if(succeeded != NULL) *succeeded = false;

    int32 amountToSend = roundTrade(tradeCommand->quantity);
    if(amountToSend != tradeCommand->quantity) {
        stubDbg(DEBUG_INFO_LEVEL, "--- Rounding trade as it had an odd size: %d requested, rounded to %d\n",
                                  tradeCommand->quantity,
                                  amountToSend);
    }

//    idtype baseId, termId;
//    getBaseAndTerm (tradeCommand->security, &baseId, &termId);
//    int32 baseQ = tradeCommand->quantity * memMap->mapBookAp[baseId].price; // Check this!!!

    idtype tiIndex = memMap->tradingInterfaceIndex[tradeCommand->tiId];

    if(!memMap->tradingInterfaceOk[tiIndex]) {

        stubDbg(DEBUG_INFO_LEVEL, "!!! TI #%d (%s) is NOT OK, therefore trade was not sent\n",
                tiIndex, memMap->tradingInterfaceName[tiIndex]);

/*
    } else if(baseQ < minimum_order_size) {

        stubDbg(DEBUG_INFO_LEVEL, "!!! Order does not reach minimum order size: %d (equivalent) eurs requested, minimum is %d\n",
                baseQ, minimum_order_size);
*/
} else if(tradeCommand->quantity < minimum_order_size) {

        stubDbg(DEBUG_INFO_LEVEL, "!!! Order does not reach minimum order size: %d (equivalent) eurs requested, minimum is %d\n",
                tradeCommand->quantity, minimum_order_size);

/*
    } else if(baseQ > maximum_order_size) {

        stubDbg(DEBUG_INFO_LEVEL, "!!! Order exceeds maximum order size: %d (equivalent) eurs requested, maximum is %d\n",
                baseQ, maximum_order_size);
*/
} else if(tradeCommand->quantity > maximum_order_size) {

    stubDbg(DEBUG_INFO_LEVEL, "!!! Order exceeds maximum order size: %d (equivalent) eurs requested, maximum is %d\n",
            tradeCommand->quantity, maximum_order_size);

    } else {
        SendTradeToCore(memMap, user, tradeCommand, tempTradeId);
        if(succeeded != NULL) *succeeded = true;
        nTradesThisSecond++;
        nTradesThis10Seconds++;
        nTradesThisMinute++;
    }

    return result;
}


int32 myModifyTradeToCore(sharedMemory_t* memMap, char* user, char* fixTradeId, idtype tiIndex, uint32 price, int32 quantity, boolean unconditional) {

    int result = ModifyTradeToCore(memMap, user, fixTradeId, price, quantity, unconditional);
//    nTradesThisSecond++;
//    nTradesThis10Seconds++;
//    nTradesThisMinute++;
    return result;
}


int32 myCancelTradeToCore(sharedMemory_t* memMap, char* user, char* fixTradeId, idtype tiIndex) {

    int result = CancelTradeToCore(memMap, user, fixTradeId);
    return result;
}



void removeOwnOrdersFromQuoteList(
        // inputs:
        sharedMemory_t *memMap,
        idtype security,
        boolean isBid, // true for BID, false for ASK

        // outputs:
        sortingFB_t OthersQuotes[MAX_QUOTES*MAX_NUMBER_TI],
        int *NumberOthersQuotes,
        sortingFB_t OwnQuotes[MAX_TRADES_LIST_ALIVE],
        int *NumberOwnQuotes,
        sortingFBbyTI_t OthersQuotesByTI[MAX_NUMBER_TI][MAX_QUOTES],
        int NumberOthersQuotesbyTI[MAX_NUMBER_TI],
        sortingFBbyTI_t OwnQuotesByTI[MAX_NUMBER_TI][MAX_TRADES_LIST_ALIVE],
        int NumberOwnQuotesbyTI[MAX_NUMBER_TI]

            ) {

    int32 auIndex = memMap->strAuIndex;

    boolean orderRemoved[MAX_TRADES_LIST_ALIVE][MAX_NUMBER_TI]={{false}};

    *NumberOthersQuotes=0;
    *NumberOwnQuotes=0;
    for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
        NumberOthersQuotesbyTI[ti]=0;
        NumberOwnQuotesbyTI[ti]=0;
    }

    for(int q=0; q < (isBid ? memMap->mapFBNumberofsortingBid[security] : memMap->mapFBNumberofsortingAsk[security]); q++) {

        sortingFB_t *thisSortingElement = (isBid ?
                                                 &(memMap->mapFBSortingBid[security][q]) :
                                                 &(memMap->mapFBSortingAsk[security][q]));
        uint32 thisPrice=thisSortingElement->price;
        idtype thisTIindex=thisSortingElement->tiIndex;
//        idtype thisTIid=memMap->tradingInterface[thisTIindex];
        idtype thisQuoteIndex=thisSortingElement->quoteIndex;
        idtype thisVenue=memMap->whichVenue[thisTIindex];
        mtbookElement_t *thisQuote = (isBid ?
                                            &(memMap->mapFBQuotesBid[thisTIindex][security][thisQuoteIndex]) :
                                            &(memMap->mapFBQuotesAsk[thisTIindex][security][thisQuoteIndex]));
        if(thisPrice!=thisQuote->price) add_message("??? Error in Full Book sorting - sorting element price different from quote element price", STR_MSG_ERROR);
        boolean quoteRemoved=false;

        int ordersProcessed=0;
        for(int o=0; (ordersProcessed<memMap->AccountingUnit[auIndex].numberOfAlives) && (o<MAX_TRADES_LIST_ALIVE); o++) {

            if(!(memMap->AccountingUnit[auIndex].alive[o].alive)) {
                continue;
            } else {
                ordersProcessed++;
                if(
                        !orderRemoved[o][thisTIindex] && // So we don't remove similar quotes because of the same limit order
//                        (thisTIid==memMap->alive[o].params.tiId) &&
                        (thisVenue==memMap->whichVenue[memMap->tradingInterfaceIndex[memMap->AccountingUnit[auIndex].alive[o].params.tiId]]) &&
                        (security==memMap->AccountingUnit[auIndex].alive[o].params.security) &&
                        (thisQuote->price==memMap->AccountingUnit[auIndex].alive[o].params.limitPrice) &&
                        (thisQuote->liquidity==memMap->AccountingUnit[auIndex].alive[o].params.quantity) &&
                        ( (isBid ? TRADE_SIDE_BUY : TRADE_SIDE_SELL) == memMap->AccountingUnit[auIndex].alive[o].params.side) &&
                        (memMap->AccountingUnit[auIndex].alive[o].params.timeInForce!=TRADE_TIMEINFORCE_FILL_OR_KILL) &&
                        (memMap->AccountingUnit[auIndex].alive[o].params.timeInForce!=TRADE_TIMEINFORCE_INMED_OR_CANCEL)
                        // Should add as well that the trade has at least been sent through FIX
                        // May check here as well if liquidity provider is us
                   ) {

                    // This is our own quote!
                    OwnQuotes[*NumberOwnQuotes].price=thisPrice;
                    OwnQuotes[*NumberOwnQuotes].tiIndex=thisTIindex;
                    OwnQuotes[*NumberOwnQuotes].quoteIndex=thisQuoteIndex;
                    (*NumberOwnQuotes)++;
                    OwnQuotesByTI[thisTIindex][NumberOwnQuotesbyTI[thisTIindex]].price=thisPrice;
                    OwnQuotesByTI[thisTIindex][NumberOwnQuotesbyTI[thisTIindex]].quoteIndex=thisQuoteIndex;
                    NumberOwnQuotesbyTI[thisTIindex]++;
                    orderRemoved[o][thisTIindex]=true;
                    quoteRemoved=true;
                    break;

                } // If own quote

            } // If trade is alive or not

        } // for (trades alive)

        if(!quoteRemoved) {

            // This is NOT our own quote
            OthersQuotes[*NumberOthersQuotes].price=thisPrice;
            OthersQuotes[*NumberOthersQuotes].tiIndex=thisTIindex;
            OthersQuotes[*NumberOthersQuotes].quoteIndex=thisQuoteIndex;
            (*NumberOthersQuotes)++;
            OthersQuotesByTI[thisTIindex][NumberOthersQuotesbyTI[thisTIindex]].price=thisPrice;
            OthersQuotesByTI[thisTIindex][NumberOthersQuotesbyTI[thisTIindex]].quoteIndex=thisQuoteIndex;
            NumberOthersQuotesbyTI[thisTIindex]++;

        } // quoteRemoved

    } // quotes in FB sorting array

}



void topOfBooksExOwnOrders(

        // inputs:
        sharedMemory_t *memMap,
        idtype security,
        boolean isBid,

        // outputs:
        int *nSortedPrices,
        uint32 sortedPrices[MAX_NUMBER_TI],
        idtype sortedTiIndex[MAX_NUMBER_TI],
        uint32 nonSortedPrices[MAX_NUMBER_TI],
        boolean tiInSortArray[MAX_NUMBER_TI]

            ) {

    int32 auIndex = memMap->strAuIndex;

    *nSortedPrices = 0;
    for(int ti = 0; ti < memMap->nTradingInterfaces; ti++) {
        tiInSortArray[ti] = false;
        nonSortedPrices[ti] = isBid ? memMap->mapTiTbBid[ti][security].price : memMap->mapTiTbAsk[ti][security].price;
    }

    for(int q=0; q < (isBid ? memMap->mapFBNumberofsortingBid[security] : memMap->mapFBNumberofsortingAsk[security]); q++) {

        sortingFB_t *thisSortingElement = (isBid ?
                                                 &(memMap->mapFBSortingBid[security][q]) :
                                                 &(memMap->mapFBSortingAsk[security][q]));
        uint32 thisPrice=thisSortingElement->price;
        idtype thisTIindex=thisSortingElement->tiIndex;
        if(tiInSortArray[thisTIindex]) continue; // We already found the top of book ex own orders for this TI

        idtype thisQuoteIndex=thisSortingElement->quoteIndex;
        idtype thisVenue=memMap->whichVenue[thisTIindex];
        mtbookElement_t *thisQuote = (isBid ?
                                            &(memMap->mapFBQuotesBid[thisTIindex][security][thisQuoteIndex]) :
                                            &(memMap->mapFBQuotesAsk[thisTIindex][security][thisQuoteIndex]));
//        if(thisPrice!=thisQuote->price) add_message("??? Error in Full Book sorting - sorting element price different from quote element price", STR_MSG_ERROR);

        boolean ownQuote = false;
        int ordersProcessed=0;
        for(int o=0; (ordersProcessed<memMap->AccountingUnit[auIndex].numberOfAlives) && (o<MAX_TRADES_LIST_ALIVE); o++) {

            if(!(memMap->AccountingUnit[auIndex].alive[o].alive)) {
                continue;
            } else {
                ordersProcessed++;
                if(
                        (thisVenue==memMap->whichVenue[memMap->tradingInterfaceIndex[memMap->AccountingUnit[auIndex].alive[o].params.tiId]]) &&
                        (security==memMap->AccountingUnit[auIndex].alive[o].params.security) &&
//                        (thisQuote->price==memMap->alive[o].params.limitPrice) && // as it could be changing! => need to sophisticate this
                        (thisQuote->liquidity==memMap->AccountingUnit[auIndex].alive[o].params.quantity) &&
                        ( (isBid ? TRADE_SIDE_BUY : TRADE_SIDE_SELL) == memMap->AccountingUnit[auIndex].alive[o].params.side)
//                        (memMap->alive[o].params.timeInForce!=TRADE_TIMEINFORCE_FILL_OR_KILL) && // Removed just to be in the safe side
//                        (memMap->alive[o].params.timeInForce!=TRADE_TIMEINFORCE_INMED_OR_CANCEL)
                        // Should add as well that the trade has at least been sent through FIX
                        // May check here as well if liquidity provider is us
                   ) {

                    // This is our own quote!
                    ownQuote = true;
                    break;

                } // If own quote

            } // If trade is alive or not

        } // for (trades alive)

        if(!ownQuote) {

            sortedPrices[*nSortedPrices] = thisPrice;
            sortedTiIndex[*nSortedPrices] = thisTIindex;
            tiInSortArray[thisTIindex] = true;
            nonSortedPrices[thisTIindex] = thisPrice;
            (*nSortedPrices)++;

        }

        if(*nSortedPrices >= (isBid ? memMap->mapSortBidNumberOfTi[security] : memMap->mapSortAskNumberOfTi[security])) break; // we are done!

    } // quotes in FB sorting array

}





void printTime(timetype_t *t, char *ptr) {

    timestruct_t ts;
    timestampToDate(t->sec, &ts);

    sprintf(ptr, "%04d/%02d/%02d - %02d:%02d:%02d.%03d(%03d)",
                 ts.year, ts.month, ts.day,
                 ts.hour, ts.min, ts.sec,
                 t->usec / 1000, t->usec % 1000);

}


int64  my_getTimeDifSigned (timetype_t* systimeStart, timetype_t* systimeEnd) {

    int64 result =
            ((int64) 1000000) * (((int64) systimeEnd->sec) - ((int64) systimeStart->sec)) +
            ((int64) systimeEnd->usec) - ((int64) systimeStart->usec);
    return result;

}


uint64 my_getTimeDif (timetype_t* systimeStart, timetype_t* systimeEnd) {

    if(systimeEnd->sec > systimeStart->sec) {
        return
                1000000 * (systimeEnd->sec - systimeStart->sec) +
                systimeEnd->usec - systimeStart->usec;
    } else if(
                 (systimeEnd->sec == systimeStart->sec) &&
                 (systimeEnd->usec > systimeStart->usec)
             ) {
        return systimeEnd->usec - systimeStart->usec;
    } else {
        return 0;
    }

}



boolean checkWithinOperatingHours(void) {

    int now = CALLBACK_strtime.min + 60 * CALLBACK_strtime.hour;

    int currentWeekDay=CALLBACK_strtime.dayofweek;
    if(
            (currentWeekDay<weekly_start_time_weekday) ||
            (
                    (currentWeekDay==weekly_start_time_weekday) &&
                    (now < weekly_start_time_min + 60 * weekly_start_time_hour)
            )
      )
        return false; // Before week start

    if(
            (currentWeekDay>weekly_end_time_weekday) ||
            (
                    (currentWeekDay==weekly_end_time_weekday) &&
                    (now >= weekly_end_time_min + 60 * weekly_end_time_hour)
            )
      )
        return false; // after week end

    if(
            (
                    (now >= daily_stop_1_start_min + 60 * daily_stop_1_start_hour) &&
                    (now < daily_stop_1_end_min + 60 * daily_stop_1_end_hour)
            ) ||
            (
                    (now >= daily_stop_2_start_min + 60 * daily_stop_2_start_hour) &&
                    (now < daily_stop_2_end_min + 60 * daily_stop_2_end_hour)
            ) ||
            (
                    (now >= daily_stop_3_start_min + 60 * daily_stop_3_start_hour) &&
                    (now < daily_stop_3_end_min + 60 * daily_stop_3_end_hour)
            )
       )
        return false; // in the middle of the maintenance window

    return true; // otherwise ;-)

}
