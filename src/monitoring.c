/*
 * monitoring.c
 *
 *  Created on: May 21, 2014
 *      Author: julio
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

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
#include "main.h"

// Global variables for monitoring and automatic management:

boolean             ValuationChangeAlarm = false;
boolean             DrawdownAlarm = false;
boolean             TooManyTradesAlarm = false;
boolean             MalfunctionAlarm = false;
boolean             withinOperatingHours;

char                MalfunctionAlarmText[4096] = "";

uint32              nTradesThisSecond = 0;
uint32              nTradesThis10Seconds = 0;
uint32              nTradesThisMinute = 0;
int                 lastSecond, last10Seconds, lastMinute;

boolean             pricesDelayed[MAX_NUMBER_TI];
boolean             pricesDelayedLast[MAX_NUMBER_TI];
int64               minFixToStubDelay_ms[MAX_NUMBER_TI];
timetype_t          pricesDelayedInitialTimestamp[MAX_NUMBER_TI];
uint64              totalPricesDelayedDowntime_ms[MAX_NUMBER_TI];
uint                totalPricesDelayedOccurrences[MAX_NUMBER_TI];

boolean             noTicks[MAX_NUMBER_TI];
boolean             noTicksLast[MAX_NUMBER_TI];
timetype_t          lastTickTimestamp[MAX_NUMBER_TI];
timetype_t          noTicksInitialTimestamp[MAX_NUMBER_TI];
uint64              totalNoTicksDowntime_ms[MAX_NUMBER_TI];
uint                totalNoTicksOccurrences[MAX_NUMBER_TI];


int                 today;

ti_performance_t    tiPerformanceInfo[MAX_NUMBER_TI];
ti_operation_t      tiOperation[MAX_NUMBER_TI];


idtype              many_securities = 8;
idtype              which_securities[NUM_SECURITIES] = {AUD_USD, EUR_GBP, EUR_JPY, EUR_USD, GBP_JPY, GBP_USD, USD_CAD, USD_JPY};
idtype              many_assets = 6;
idtype              which_assets[NUM_ASSETS] = {AUD, CAD, EUR, GBP, JPY, USD};

boolean             auto_management_connected = false;
boolean             strategy_connected = false;

uint32              numberOfAlives_last = 0;
uint32              indexOfHistoric_last_trade_reporting = 0;
uint32              indexOfHistoric_last_yellowCards = 0;
byte                tradingInterfaceOk_last[MAX_NUMBER_TI] = {TI_STATUS_NOK};
number              GlobalEquity_STR_last;
number              maxEquity;


histogram_t         FixToCore[MAX_NUMBER_TI];
histogram_t         FixToStub[MAX_NUMBER_TI];

histogram_t         CoreToStub;
histogram_t         TickToOrder;
histogram_t         Callback;

histogram_t         profilingHistograms[TEST_TICKTOTRADE_TIME_MESAURES+1]; // Histograms for tick-to-trade latency measurements breakdown

histogram_t         ExecTimesImmediateLimit[MAX_NUMBER_TI]; // TI index!
histogram_t         ExecTimesMarket[MAX_NUMBER_TI]; // TI index!
histogram_t         ExecTimesCancel[MAX_NUMBER_TI]; // TI index!
histogram_t         ExecTimesReplace[MAX_NUMBER_TI]; // TI index!


int                 profilingPtr[MAX_NUMBER_TI] = {0};
char                FixTimesStr[MAX_PROFILING_SAMPLES][MAX_NUMBER_TI][MAX_FIX_TIME_STRING_SIZE] = {{{""}}};
timetype_t          CoreTimes[MAX_PROFILING_SAMPLES][MAX_NUMBER_TI] = {{{0}}};
timetype_t          StubTimes[MAX_PROFILING_SAMPLES][MAX_NUMBER_TI] = {{{0}}};
uint32              callback_numbers[MAX_PROFILING_SAMPLES][MAX_NUMBER_TI] = {{0}};
boolean             profilingBufferFull[MAX_NUMBER_TI] = {false};

int                 callbackDelaysPtr = 0;
uint64              callbackDelays[MAX_PROFILING_SAMPLES] = {0};
boolean             callbackDelaysBufferFull = false;

int                 execTimesImmediateLimitDelaysPtr[MAX_NUMBER_TI] = {0};
uint64              execTimesImmediateLimitDelays[MAX_PROFILING_SAMPLES][MAX_NUMBER_TI] = {{0}};
boolean             execTimesImmediateLimitDelaysBufferFull[MAX_NUMBER_TI] = {false};

int                 execTimesMarketDelaysPtr[MAX_NUMBER_TI] = {0};
uint64              execTimesMarketDelays[MAX_PROFILING_SAMPLES][MAX_NUMBER_TI] = {{0}};
boolean             execTimesMarketDelaysBufferFull[MAX_NUMBER_TI] = {false};

int                 execTimesCancelDelaysPtr[MAX_NUMBER_TI] = {0};
uint64              execTimesCancelDelays[MAX_PROFILING_SAMPLES][MAX_NUMBER_TI] = {{0}};
boolean             execTimesCancelDelaysBufferFull[MAX_NUMBER_TI] = {false};

int                 execTimesReplaceDelaysPtr[MAX_NUMBER_TI] = {0};
uint64              execTimesReplaceDelays[MAX_PROFILING_SAMPLES][MAX_NUMBER_TI] = {{0}};
boolean             execTimesReplaceDelaysBufferFull[MAX_NUMBER_TI] = {false};

boolean             initiallyDiscardedOVerwritten=false;


tradeIndicators_t   indicators[NUM_TIMEFRAMES][MAX_NUMBER_PB];
int32               equityValue[NUM_TIMEFRAMES][MAX_NUMBER_PB];



/************************************************************************************/
/* Quote chains                                                                     */

boolean  chainsEnabled;                      // Set to true if to be enabled - please through resetChains()
idtype   chainSecurity;                      // Chains only available for one security at a time (remember to reset chains if you change this!)

uint16   nextChain;                          // This is the chain next to the last one that has been started (will neeed to look for free one anyway)
boolean  isChainUsed[MAX_CHAINS];            // This says if a chain is used (chains are kept in a circular buffer)

uint16   whichChainBid[MAX_QUOTES];          // This gives you the chain index corresponding to each quote
uint16   whichChainAsk[MAX_QUOTES];
uint16   nQuotesInChainBid[MAX_CHAINS];      // This gives you the number of samples available in a particular chain
uint16   nQuotesInChainAsk[MAX_CHAINS];
uint16   lastQuoteInChainBid[MAX_CHAINS];    // This gives you the last (most current) sample in the chain (which is a circular buffer)
uint16   lastQuoteInChainAsk[MAX_CHAINS];

quoteChainElement_t quoteChains[MAX_CHAINS][MAX_CHAIN_SAMPLES]; // This is the actual repository of quote chains

void resetChains(boolean enable, idtype whichSecurity) {

    chainsEnabled = enable;
    chainSecurity = whichSecurity;
    nextChain = 0;
    for(int i = 0; i < MAX_CHAINS; i++) isChainUsed[i] = false;

}

void saveChainsToFile(char *filename) {

}




void initHistograms(sharedMemory_t* memMap);


void eraseTIPerformanceRecords(sharedMemory_t* memMap, int32 auIndex) {

    for(int tiIndex=0; tiIndex<MAX_NUMBER_TI; tiIndex++) {

        tiPerformanceInfo[tiIndex].fullyFilledImmediateLimitTradesToday=0;
        tiPerformanceInfo[tiIndex].partiallyFilledImmediateLimitTradesToday=0;
        tiPerformanceInfo[tiIndex].failedAndRejectedImmediateLimitTradesToday=0;
        tiPerformanceInfo[tiIndex].userCanceledImmediateLimitTradesToday=0;

        tiPerformanceInfo[tiIndex].fullyFilledMarketTradesToday=0;
        tiPerformanceInfo[tiIndex].partiallyFilledMarketTradesToday=0;
        tiPerformanceInfo[tiIndex].failedAndRejectedMarketTradesToday=0;
        tiPerformanceInfo[tiIndex].userCanceledMarketTradesToday=0;

        tiPerformanceInfo[tiIndex].fullyFilledOtherTradesToday=0;
        tiPerformanceInfo[tiIndex].partiallyFilledOtherTradesToday=0;
        tiPerformanceInfo[tiIndex].failedAndRejectedOtherTradesToday=0;
        tiPerformanceInfo[tiIndex].userCanceledOtherTradesToday=0;

        tiPerformanceInfo[tiIndex].replacesDoneToday=0;

        tiPerformanceInfo[tiIndex].amountTradedToday=0;

    }
    indexOfHistoric_last_trade_reporting=memMap->AccountingUnit[auIndex].indexOfHistoric;

}


void eraseTIOperationRecords(sharedMemory_t* memMap, int32 auIndex) {

    for(int tiIndex=0; tiIndex<MAX_NUMBER_TI; tiIndex++) {

        tiOperation[tiIndex].rejectsInARow=0;
        tiOperation[tiIndex].failedImmediateLimitTradesInaRow=0;
        tiOperation[tiIndex].successfulImmediateLimitTradesInaRow=0;
        tiOperation[tiIndex].successfulMarketTradesInaRow=0;

        tiOperation[tiIndex].goodQuotesToday=0;
        tiOperation[tiIndex].badQuotesToday=0;

        tiOperation[tiIndex].yellowCardCounter=0;
        tiOperation[tiIndex].yellowCardTimestamp=0;
        tiOperation[tiIndex].yellowCard=false;;
        tiOperation[tiIndex].redCard=false;

    }

    indexOfHistoric_last_yellowCards=memMap->AccountingUnit[auIndex].indexOfHistoric;

}



void resetIndicatorsAndEquity(sharedMemory_t* memMap, int32 auIndex, idtype PBindex, tradeIndicators_t *indResult, int32 *eqResult) {
    indResult->totalTrades=0;
    indResult->fullyFilledTrades=0;
    indResult->partiallyFilledTrades=0;
    indResult->failedAndRejectedTrades=0;
    indResult->userCanceledTrades=0;
    indResult->replacesDone=0;
    indResult->amountTraded=0;
    *eqResult=memMap->AccountingUnit[auIndex].Equity_STR[PBindex];
}




void writeIndicators(idtype PBindex, idtype timeframe, tradeIndicators_t *ind, int32 eq) {

    mkdir(INDICATORS_DIR, 0777);

    char filename[SLEN];
    sprintf(filename, "%s/indicators_%s_%d_%d.txt", INDICATORS_DIR, userName, PBindex, timeframe);


    FILE *output_file=fopen(filename,"wo");

    fprintf(output_file, "date is %d:%d:%d:%d\n", CALLBACK_strtime.year, CALLBACK_strtime.month, CALLBACK_strtime.day, CALLBACK_strtime.dayofyear);
    fprintf(output_file, "totalTrades = %d\n", ind->totalTrades);
    fprintf(output_file, "fullyFilledTrades = %d\n", ind->fullyFilledTrades);
    fprintf(output_file, "partiallyFilledTrades = %d\n", ind->partiallyFilledTrades);
    fprintf(output_file, "failedAndRejectedTrades = %d\n", ind->failedAndRejectedTrades);
    fprintf(output_file, "userCanceledTrades = %d\n", ind->userCanceledTrades);
    fprintf(output_file, "replacesDone = %d\n", ind->replacesDone);
    fprintf(output_file, "amountTraded = %.0lf\n", ind->amountTraded);
    fprintf(output_file, "equityValue = %d\n", eq);

    fprintf(output_file, "#done!\n");
    fclose(output_file);

}




boolean readIndicatorsFromFile(sharedMemory_t* memMap, int32 auIndex, idtype PBindex, idtype timeframe, tradeIndicators_t *indResult, int32 *eqResult,
                            int *year, int *month, int *day, int *dayOfYear) {

    resetIndicatorsAndEquity(memMap, auIndex, PBindex, indResult, eqResult);
    *year = 0;
    *month = 0;
    *day = 0;
    *dayOfYear = 0;

    char filename[SLEN];
    sprintf(filename, "%s/indicators_%s_%d_%d.txt", INDICATORS_DIR, userName, PBindex, timeframe);

    FILE *input_file;
    input_file=fopen(filename,"r");

    if(!input_file) return false;

    boolean errorInFile = false;
    if(fscanf(input_file, "date is %d:%d:%d:%d\n", year, month, day, dayOfYear) != 4) errorInFile=true;
    if(fscanf(input_file, "totalTrades = %d\n", &(indResult->totalTrades)) !=1) errorInFile=true;
    if(fscanf(input_file, "fullyFilledTrades = %d\n", &(indResult->fullyFilledTrades)) !=1) errorInFile=true;
    if(fscanf(input_file, "partiallyFilledTrades = %d\n", &(indResult->partiallyFilledTrades)) !=1) errorInFile=true;
    if(fscanf(input_file, "failedAndRejectedTrades = %d\n", &(indResult->failedAndRejectedTrades)) !=1) errorInFile=true;
    if(fscanf(input_file, "userCanceledTrades = %d\n", &(indResult->userCanceledTrades)) !=1) errorInFile=true;
    if(fscanf(input_file, "replacesDone = %d\n", &(indResult->replacesDone)) !=1) errorInFile=true;
    if(fscanf(input_file, "amountTraded = %lf\n", &(indResult->amountTraded)) !=1) errorInFile=true;
    if(fscanf(input_file, "equityValue = %d\n", eqResult) !=1) errorInFile=true;

    if(errorInFile) {

        resetIndicatorsAndEquity(memMap, auIndex, PBindex, indResult, eqResult);
        *year = 0;
        *month = 0;
        *day = 0;
        *dayOfYear = 0;

    }

    fclose(input_file);
    if(errorInFile) remove(filename); // Since this file is incorrect or out of date

    return !errorInFile;

}



boolean buildTodayNonSavedTradeIndicatorsForPB(sharedMemory_t* memMap, idtype PBindex, tradeIndicators_t *result) {

    if((PBindex<0) || (PBindex>=memMap->nPrimeBrokers)) return false;

    result->totalTrades=0;
    result->fullyFilledTrades=0;
    result->partiallyFilledTrades=0;
    result->failedAndRejectedTrades=0;
    result->userCanceledTrades=0;
    result->replacesDone=0;
    result->amountTraded=0;

    for(idtype tiIndex=0; tiIndex<memMap->nTradingInterfaces; tiIndex++) {

        if(memMap->primeBrokerIndex[memMap->whichPrimeBroker[tiIndex]] == PBindex) {

            uint32 fullyFilledTrades = tiPerformanceInfo[tiIndex].fullyFilledImmediateLimitTradesToday +
                                       tiPerformanceInfo[tiIndex].fullyFilledMarketTradesToday +
                                       tiPerformanceInfo[tiIndex].fullyFilledOtherTradesToday;

            uint32 partiallyFilledTrades = tiPerformanceInfo[tiIndex].partiallyFilledImmediateLimitTradesToday +
                                           tiPerformanceInfo[tiIndex].partiallyFilledMarketTradesToday +
                                           tiPerformanceInfo[tiIndex].partiallyFilledOtherTradesToday;

            uint32 failedAndRejectedTrades = tiPerformanceInfo[tiIndex].failedAndRejectedImmediateLimitTradesToday +
                                             tiPerformanceInfo[tiIndex].failedAndRejectedMarketTradesToday +
                                             tiPerformanceInfo[tiIndex].failedAndRejectedOtherTradesToday;

            uint32 userCanceledTrades = tiPerformanceInfo[tiIndex].userCanceledImmediateLimitTradesToday +
                                        tiPerformanceInfo[tiIndex].userCanceledMarketTradesToday +
                                        tiPerformanceInfo[tiIndex].userCanceledOtherTradesToday;

            uint32 totalTrades = fullyFilledTrades +
                                 partiallyFilledTrades +
                                 failedAndRejectedTrades +
                                 userCanceledTrades;

            result->totalTrades += totalTrades;
            result->fullyFilledTrades += fullyFilledTrades;
            result->partiallyFilledTrades += partiallyFilledTrades;
            result->failedAndRejectedTrades += failedAndRejectedTrades;
            result->userCanceledTrades += userCanceledTrades;
            result->replacesDone += tiPerformanceInfo[tiIndex].replacesDoneToday;
            result->amountTraded += tiPerformanceInfo[tiIndex].amountTradedToday;

        }

    } // for tiIndex

    return true;

}


void buildTodayNonSavedTradeIndicatorsForAllPBs(sharedMemory_t* memMap, tradeIndicators_t *result) {

    result->totalTrades=0;
    result->fullyFilledTrades=0;
    result->partiallyFilledTrades=0;
    result->failedAndRejectedTrades=0;
    result->userCanceledTrades=0;
    result->replacesDone=0;
    result->amountTraded=0;

    tradeIndicators_t PBindicators = {0};

    for(int PBindex = 0; PBindex < memMap->nPrimeBrokers; PBindex++) {

        buildTodayNonSavedTradeIndicatorsForPB(memMap, PBindex, &PBindicators);

        result->totalTrades+=PBindicators.totalTrades;
        result->fullyFilledTrades+=PBindicators.fullyFilledTrades;
        result->partiallyFilledTrades+=PBindicators.partiallyFilledTrades;
        result->failedAndRejectedTrades+=PBindicators.failedAndRejectedTrades;
        result->userCanceledTrades+=PBindicators.userCanceledTrades;
        result->replacesDone+=PBindicators.replacesDone;
        result->amountTraded+=PBindicators.amountTraded;

    } // for PBindex

}



int32 init_monitoring(sharedMemory_t* memMap, flagsChanges_t* changesMap) {

    int32 result = OK;
    int32 auIndex = memMap->strAuIndex;

    today=INITIAL_strtime.day;

#ifdef _SHOW_DEBUG_PRICES
    char dbg_msg[8192];
    char dbg_str[8192];
    sprintf(dbg_msg, ",Prices_data,");
    for(int i = 0; i < memMap ->nTradingInterfaces; i++) {
        sprintf(dbg_str, "#%d EUR_USD Bid,#%d EUR_USD Ask,#%d GBP_USD Bid,#%d GBP_USD Ask,#%d USD_CHF Bid,#%d USD_CHF Ask,", i, i, i, i, i, i);
        strcat(dbg_msg, dbg_str);
    }
    sprintf(dbg_str, "Sys EUR_USD Bid,Sys EUR_USD Ask,Sys GBP_USD Bid,Sys GBP_USD Ask,Sys USD_CHF Bid,Sys USD_CHF Ask,EUR,USD,GBP,CHF,Eq EP,Eq AU 3\n");
    strcat(dbg_msg, dbg_str);
    stubDbg(DEBUG_PERFORMANCE, dbg_msg);
#endif

    GlobalEquity_STR_last = memMap->AccountingUnit[auIndex].GlobalEquity_STR;
    maxEquity = memMap->AccountingUnit[auIndex].GlobalEquity_STR;

    lastSecond = INITIAL_strtime.sec;
    last10Seconds = INITIAL_strtime.sec / 10;
    lastMinute = INITIAL_strtime.min;

    initHistograms(memMap);

#ifdef _SHOW_DEBUG_PRICES
    stubDbg(DEBUG_INFO_LEVEL, "_SHOW_DEBUG_PRICES activated - now generating price logs"\n);
#endif

#ifdef _CHECK_SORTING
    stubDbg(DEBUG_INFO_LEVEL, "_CHECK_SORTING activated - now checking top of book and full book sorting on security #%d (%s)\n",
            _SECURITY_TO_CHECK,
            securityName[_SECURITY_TO_CHECK]);
#endif

#ifdef _CHECK_FULL_BOOK_AGGREGATION
    stubDbg(DEBUG_INFO_LEVEL, "_CHECK_FULL_BOOK_AGGREGATION activated - now checking top of book consistency from full book (disaggregated) quotes array\n");
#endif

#ifdef _CHECK_TRADES_ALIVE
    stubDbg(DEBUG_INFO_LEVEL, "_CHECK_TRADES_ALIVE activated - now checking whether there are alive trades in final states\n");
#endif


    eraseTIPerformanceRecords(memMap, auIndex);
    eraseTIOperationRecords(memMap, auIndex);


    // Indicators initialization:

    for(idtype pbIndex=0; pbIndex<memMap->nPrimeBrokers; pbIndex++) {

        int year, month, day, dayOfYear; // Common for all readIndicatorsFromFile
        boolean fileExists;


        // 1. SINCE INCEPTION
        readIndicatorsFromFile(memMap, auIndex, pbIndex, TIMEFRAME_SINCE_INCEPTION, &(indicators[TIMEFRAME_SINCE_INCEPTION][pbIndex]), &(equityValue[TIMEFRAME_SINCE_INCEPTION][pbIndex]), &year, &month, &day, &dayOfYear);


        // 2. THIS MONTH
        fileExists =
                readIndicatorsFromFile(memMap, auIndex, pbIndex, TIMEFRAME_THIS_MONTH, &(indicators[TIMEFRAME_THIS_MONTH][pbIndex]), &(equityValue[TIMEFRAME_THIS_MONTH][pbIndex]), &year, &month, &day, &dayOfYear);
        if(
                fileExists &&
                (
                        (year == INITIAL_strtime.year) &&
                        (month == INITIAL_strtime.month)
                )
          ) {
            // This file applies

        } else {
            // This file is old
            resetIndicatorsAndEquity(memMap, auIndex, pbIndex, &(indicators[TIMEFRAME_THIS_MONTH][pbIndex]), &(equityValue[TIMEFRAME_THIS_MONTH][pbIndex]));

        }


        // 3. THIS WEEK
        fileExists =
                readIndicatorsFromFile(memMap, auIndex, pbIndex, TIMEFRAME_THIS_WEEK, &(indicators[TIMEFRAME_THIS_WEEK][pbIndex]), &(equityValue[TIMEFRAME_THIS_WEEK][pbIndex]), &year, &month, &day, &dayOfYear);
        int this_sunday = INITIAL_strtime.dayofyear - INITIAL_strtime.dayofweek;
        boolean fileApplies = false;

        if(fileExists) {

            if(this_sunday>0) {

                if(
                        (year == INITIAL_strtime.year) &&
                        (dayOfYear >= this_sunday)
                  ) {
                    // This file applies
                    fileApplies=true;
                }
            } else {
                this_sunday=31+this_sunday;
                if(
                        (year == INITIAL_strtime.year) ||
                        (
                                (year == INITIAL_strtime.year-1) &&
                                (day >= this_sunday)
                        )
                  ) {
                    // This file applies
                    fileApplies=true;
                }
            }

        } // file exists

        if(!fileApplies) {
            // This file is old
            resetIndicatorsAndEquity(memMap, auIndex, pbIndex, &(indicators[TIMEFRAME_THIS_WEEK][pbIndex]), &(equityValue[TIMEFRAME_THIS_WEEK][pbIndex]));

        }


        // 4. TODAY
        fileExists =
                readIndicatorsFromFile(memMap, auIndex, pbIndex, TIMEFRAME_TODAY, &(indicators[TIMEFRAME_TODAY][pbIndex]), &(equityValue[TIMEFRAME_TODAY][pbIndex]), &year, &month, &day, &dayOfYear);
        if(
                fileExists &&
                (
                        (year == INITIAL_strtime.year) &&
                        (day == INITIAL_strtime.dayofyear)
                )
          ) {
            // This file applies

        } else {

            if(fileExists) {
                // This file is old, but we should consolidate these numbers in the other files
                // First, the since inception data
                indicators[TIMEFRAME_SINCE_INCEPTION][pbIndex].totalTrades+=indicators[TIMEFRAME_TODAY][pbIndex].totalTrades;
                indicators[TIMEFRAME_SINCE_INCEPTION][pbIndex].fullyFilledTrades+=indicators[TIMEFRAME_TODAY][pbIndex].fullyFilledTrades;
                indicators[TIMEFRAME_SINCE_INCEPTION][pbIndex].partiallyFilledTrades+=indicators[TIMEFRAME_TODAY][pbIndex].partiallyFilledTrades;
                indicators[TIMEFRAME_SINCE_INCEPTION][pbIndex].failedAndRejectedTrades+=indicators[TIMEFRAME_TODAY][pbIndex].failedAndRejectedTrades;
                indicators[TIMEFRAME_SINCE_INCEPTION][pbIndex].userCanceledTrades+=indicators[TIMEFRAME_TODAY][pbIndex].userCanceledTrades;
                indicators[TIMEFRAME_SINCE_INCEPTION][pbIndex].replacesDone+=indicators[TIMEFRAME_TODAY][pbIndex].replacesDone;
                indicators[TIMEFRAME_SINCE_INCEPTION][pbIndex].amountTraded+=indicators[TIMEFRAME_TODAY][pbIndex].amountTraded;

                // second, the data for this month
                if(
                        (year==INITIAL_strtime.year) &&
                        (month==INITIAL_strtime.month)
                  ) {
                    indicators[TIMEFRAME_THIS_MONTH][pbIndex].totalTrades+=indicators[TIMEFRAME_TODAY][pbIndex].totalTrades;
                    indicators[TIMEFRAME_THIS_MONTH][pbIndex].fullyFilledTrades+=indicators[TIMEFRAME_TODAY][pbIndex].fullyFilledTrades;
                    indicators[TIMEFRAME_THIS_MONTH][pbIndex].partiallyFilledTrades+=indicators[TIMEFRAME_TODAY][pbIndex].partiallyFilledTrades;
                    indicators[TIMEFRAME_THIS_MONTH][pbIndex].failedAndRejectedTrades+=indicators[TIMEFRAME_TODAY][pbIndex].failedAndRejectedTrades;
                    indicators[TIMEFRAME_THIS_MONTH][pbIndex].userCanceledTrades+=indicators[TIMEFRAME_TODAY][pbIndex].userCanceledTrades;
                    indicators[TIMEFRAME_THIS_MONTH][pbIndex].replacesDone+=indicators[TIMEFRAME_TODAY][pbIndex].replacesDone;
                    indicators[TIMEFRAME_THIS_MONTH][pbIndex].amountTraded+=indicators[TIMEFRAME_TODAY][pbIndex].amountTraded;
                }

                // and third, the data for this week

                int this_sunday = INITIAL_strtime.dayofyear - INITIAL_strtime.dayofweek;
                fileApplies = false;
                if(this_sunday>0) {

                    if(
                            (year == INITIAL_strtime.year) &&
                            (dayOfYear >= this_sunday)
                      ) {
                        // This file applies
                        fileApplies=true;
                    }
                } else {
                    this_sunday=31+this_sunday;
                    if(
                            (year == INITIAL_strtime.year) ||
                            (
                                    (year == INITIAL_strtime.year-1) &&
                                    (day >= this_sunday)
                            )
                      ) {
                        // This file applies
                        fileApplies=true;
                    }
                }

                if(fileApplies) {
                    indicators[TIMEFRAME_THIS_WEEK][pbIndex].totalTrades+=indicators[TIMEFRAME_TODAY][pbIndex].totalTrades;
                    indicators[TIMEFRAME_THIS_WEEK][pbIndex].fullyFilledTrades+=indicators[TIMEFRAME_TODAY][pbIndex].fullyFilledTrades;
                    indicators[TIMEFRAME_THIS_WEEK][pbIndex].partiallyFilledTrades+=indicators[TIMEFRAME_TODAY][pbIndex].partiallyFilledTrades;
                    indicators[TIMEFRAME_THIS_WEEK][pbIndex].failedAndRejectedTrades+=indicators[TIMEFRAME_TODAY][pbIndex].failedAndRejectedTrades;
                    indicators[TIMEFRAME_THIS_WEEK][pbIndex].userCanceledTrades+=indicators[TIMEFRAME_TODAY][pbIndex].userCanceledTrades;
                    indicators[TIMEFRAME_THIS_WEEK][pbIndex].replacesDone+=indicators[TIMEFRAME_TODAY][pbIndex].replacesDone;
                    indicators[TIMEFRAME_THIS_WEEK][pbIndex].amountTraded+=indicators[TIMEFRAME_TODAY][pbIndex].amountTraded;
                }

            } // file exists

            resetIndicatorsAndEquity(memMap, auIndex, pbIndex, &(indicators[TIMEFRAME_TODAY][pbIndex]), &(equityValue[TIMEFRAME_TODAY][pbIndex]));

        } // file does not apply


        // Finally, we overwrite all files (needed if we found an old "today" file not consolidated into the other timeframe files
        for(idtype tf=0; tf<NUM_TIMEFRAMES; tf++) {

            writeIndicators(pbIndex,tf, &(indicators[tf][pbIndex]), equityValue[tf][pbIndex]);

        } // for timeframes

    } // for pbs


    // TIs downtime flags

    for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {

        pricesDelayed[ti] = false;
        pricesDelayedLast[ti] = false;
        minFixToStubDelay_ms[ti] = (INITIAL_systime.usec / 1000) + 1000 * INITIAL_systime.sec;
        pricesDelayedInitialTimestamp[ti].sec = 0;
        pricesDelayedInitialTimestamp[ti].usec = 0;
        totalPricesDelayedDowntime_ms[ti] = 0;
        totalPricesDelayedOccurrences[ti] = 0;

        noTicks[ti] = false;
        noTicksLast[ti] = false;
        lastTickTimestamp[ti].sec = 0;
        lastTickTimestamp[ti].usec = 0;
        noTicksInitialTimestamp[ti].sec = 0;
        noTicksInitialTimestamp[ti].usec = 0;
        totalNoTicksDowntime_ms[ti] = 0;
        totalNoTicksOccurrences[ti] = 0;

    }



    return (result);

}




int32 do_trade_reporting (sharedMemory_t* memMap,
                     flagsChanges_t* changesMap)
{

    char msg[SLEN];
    int32 auIndex = memMap->strAuIndex;

    if(memMap->AccountingUnit[auIndex].numberOfAlives != numberOfAlives_last) {
/*
        sprintf(msg, "--- Number of trades alive just changed from %d to %d - MT time is %s",
                numberOfAlives_last, memMap->numberOfAlives, MTtimeStr);
        stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
//        add_message(msg, STR_MSG_SYSTEM_INFO);
*/

        numberOfAlives_last = memMap->AccountingUnit[auIndex].numberOfAlives;

    } // if numberOfAlives changed


    if(memMap->AccountingUnit[auIndex].indexOfHistoric != indexOfHistoric_last_trade_reporting) {

/*
        sprintf(msg,  "--- Number of historical trades just changed from %d to %d%s - MT time is %s",
                indexOfHistoric_last_trade_reporting, memMap->indexOfHistoric,
                (indexOfHistoric_last_trade_reporting > memMap->indexOfHistoric ? " (array overflow)" : ""),
                MTtimeStr
               );
        stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
//        add_message(msg, STR_MSG_SYSTEM_INFO);
*/

        for(int t = indexOfHistoric_last_trade_reporting;
            t < memMap->AccountingUnit[auIndex].indexOfHistoric + (indexOfHistoric_last_trade_reporting > memMap->AccountingUnit[auIndex].indexOfHistoric ? MAX_TRADES_LIST_HISTORIC : 0);
            t++) {
            int t_idx = t % MAX_TRADES_LIST_HISTORIC;
            tradeElement_t *thisTrade=&(memMap->AccountingUnit[auIndex].historic[t_idx]);
            idtype tiIndex = memMap->tradingInterfaceIndex[thisTrade->params.tiId];

            uint64 thisDelay = addExecTime(EXEC_TIME_REPLACE, &(thisTrade->info.timeReplaceRequest), &(thisTrade->info.timeExecuted), tiIndex);

            if(thisTrade->info.status==TRADE_STATE_REPLACED_TO_NEW) {

/*
                sprintf(msg, "--- Order %s was changed on %s (original static fix ID was %s, amount was %d, %s, limit was %d, reserved is %u)- exec time was %.1fms, MT time is %s",
                    thisTrade->ids.fixId,
                    memMap->tradingInterfaceName[tiIndex],
                    thisTrade->ids.staticFixId,
                    thisTrade->params.quantity,
                    (thisTrade->params.side == TRADE_SIDE_BUY ? "BUY" : "SELL"),
                    thisTrade->params.limitPrice,
                    thisTrade->params.reservedData,
                    ((double) thisDelay) / 1000.0,
                    MTtimeStr
                   );
  */
// Less verbose:
                sprintf(msg, "--- %s order changed on %s, resvd %u, took %.1fms @ MTtime %s",
                    (thisTrade->params.side == TRADE_SIDE_BUY ? "BUY" : "SELL"),
                    memMap->tradingInterfaceName[tiIndex],
                    thisTrade->params.reservedData,
                    ((double) thisDelay) / 1000.0,
                    MTtimeStr
                   );


//                add_message(msg, STR_MSG_SYSTEM_INFO);
                stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);
                tiPerformanceInfo[tiIndex].replacesDoneToday++;

/*
                char timeRxCore[SLEN], timeSentToVenue[SLEN], timeAckFromVenue[SLEN],
                     timeExecuted[SLEN], timeReplaceRequest[SLEN], timeCancelRequest[SLEN];

                printTime(&(thisTrade->info.timeRxCore), timeRxCore);
                printTime(&(thisTrade->info.timeSentToVenue), timeSentToVenue);
                printTime(&(thisTrade->info.timeAckFromVenue), timeAckFromVenue);
                printTime(&(thisTrade->info.timeExecuted), timeExecuted);
                printTime(&(thisTrade->info.timeReplaceRequest), timeReplaceRequest);
                printTime(&(thisTrade->info.timeCancelRequest), timeCancelRequest);

                stubDbg(DEBUG_INFO_LEVEL, "Replaced ID - fixID is %s\n"
                                          "   timeRxCore is %s\n"
                                          "   timeSentToVenue is %s\n"
                                          "   timeAckFromVenue is %s\n"
                                          "   timeExecuted is %s\n"
                                          "   timeReplaceRequest is %s\n"
                                          "   timeCancelRequest is %s\n",
                                          thisTrade->ids.fixId,
                                          timeRxCore,
                                          timeSentToVenue,
                                          timeAckFromVenue,
                                          timeExecuted,
                                          timeReplaceRequest,
                                          timeCancelRequest
                       );
*/
            } // status was TRADE_STATE_REPLACED_TO_NEW
            else if(thisTrade->info.status!=TRADE_STATE_REPLACED_TO_CANCEL) {

                printHistoricalTradeReport_au(memMap, auIndex, t_idx, msg);
//                add_message(msg, STR_MSG_SYSTEM_INFO);
                stubDbg(DEBUG_INFO_LEVEL, "%s\n", msg);

                if(thisTrade->params.tradeType==TRADE_TYPE_MARKET) {

                    addExecTime(EXEC_TIME_MARKET, &(thisTrade->info.timeRxCore), &(thisTrade->info.timeExecuted), tiIndex);

                    if(
                            (thisTrade->info.status==TRADE_STATE_EXECUTED) &&
                            (abs(thisTrade->params.finishedQuantity - thisTrade->params.quantity) < min_trade_step)
                      ) {
                        // This is a fully filled market trade
                        tiPerformanceInfo[tiIndex].fullyFilledMarketTradesToday++;
                    } else if(
                            (thisTrade->info.status==TRADE_STATE_EXECUTED) &&
                            (abs(thisTrade->params.finishedQuantity)>=min_trade_step)
                      ) {
                        // This is a partially filled market trade
                        tiPerformanceInfo[tiIndex].partiallyFilledMarketTradesToday++;
                    } else if(
                            (thisTrade->info.status==TRADE_STATE_CANCELED) ||
                            (thisTrade->info.status==TRADE_STATE_CANCELED_BY_USER)
                      ) {
                        // This is a user-canceled market trade (impossible I guess)
                        tiPerformanceInfo[tiIndex].userCanceledMarketTradesToday++;
                    } else {
                        // This trade failed for some other reason
                        tiPerformanceInfo[tiIndex].failedAndRejectedMarketTradesToday++;
                    }

                } // if market
                else if(
                            (thisTrade->params.tradeType==TRADE_TYPE_LIMIT) &&
                            (
                                    (thisTrade->params.timeInForce==TRADE_TIMEINFORCE_INMED_OR_CANCEL) ||
                                    (thisTrade->params.timeInForce==TRADE_TIMEINFORCE_FILL_OR_KILL)
                            )
                       ) {

                    addExecTime(EXEC_TIME_IMMEDIATE_LIMIT, &(thisTrade->info.timeRxCore), &(thisTrade->info.timeExecuted), tiIndex);

                    if(
                            (thisTrade->info.status==TRADE_STATE_EXECUTED) &&
                            (abs(thisTrade->params.finishedQuantity - thisTrade->params.quantity) < min_trade_step)
                      ) {
                        // This is a fully filled immediate limit trade
                        tiPerformanceInfo[tiIndex].fullyFilledImmediateLimitTradesToday++;
                    } else if(
                            (thisTrade->info.status==TRADE_STATE_EXECUTED) &&
                            (abs(thisTrade->params.finishedQuantity)>=min_trade_step)
                      ) {
                        // This is a partially filled immediate limit trade
                        tiPerformanceInfo[tiIndex].partiallyFilledImmediateLimitTradesToday++;
                    } else if(
                            (thisTrade->info.status==TRADE_STATE_CANCELED) ||
                            (thisTrade->info.status==TRADE_STATE_CANCELED_BY_USER)
                      ) {
                        // This is a user-canceled immediate limit trade (unusual, but not impossible I guess)
                        tiPerformanceInfo[tiIndex].userCanceledImmediateLimitTradesToday++;
                    } else {
                        // This trade failed for some other reason
                        tiPerformanceInfo[tiIndex].failedAndRejectedImmediateLimitTradesToday++;
                    }

                } // if immediate limit

                else {

                    // No exec times to report here

                    if(
                            (thisTrade->info.status==TRADE_STATE_EXECUTED) &&
                            (abs(thisTrade->params.finishedQuantity - thisTrade->params.quantity) < min_trade_step)
                      ) {
                        // This is a fully filled other trade
                        tiPerformanceInfo[tiIndex].fullyFilledOtherTradesToday++;
                    } else if(
                            (thisTrade->info.status==TRADE_STATE_EXECUTED) &&
                            (abs(thisTrade->params.finishedQuantity)>=min_trade_step)
                      ) {
                        // This is a partially filled other trade
                        tiPerformanceInfo[tiIndex].partiallyFilledOtherTradesToday++;
                    } else if(
                            (thisTrade->info.status==TRADE_STATE_CANCELED) ||
                            (thisTrade->info.status==TRADE_STATE_CANCELED_BY_USER)
                      ) {
                        // This is a user-canceled other trade (this one is perfectly possible)
                        tiPerformanceInfo[tiIndex].userCanceledOtherTradesToday++;
                    } else {
                        // This trade failed for some other reason
                        tiPerformanceInfo[tiIndex].failedAndRejectedOtherTradesToday++;
                    }

                } // if other type


                idtype baseId, termId;
                getBaseAndTerm(thisTrade->params.security, &baseId, &termId);

                tiPerformanceInfo[tiIndex].amountTradedToday+=memMap->mapBookAp[baseId].price * (number) thisTrade->params.finishedQuantity;

            } // status was not CHANGED_ID


    //            print_prices_full_book(memMap, thisTrade->params.security);
        }

        indexOfHistoric_last_trade_reporting = memMap->AccountingUnit[auIndex].indexOfHistoric;

    } // if indexOfHistoric changed

    return OK;

}


int32 do_yellow_cards (sharedMemory_t* memMap,
                     flagsChanges_t* changesMap)
{

    char msg[SLEN];
    int32 auIndex = memMap->strAuIndex;

    if(!connect_yellow_cards) return OK;

    if(memMap->AccountingUnit[auIndex].indexOfHistoric != indexOfHistoric_last_yellowCards) {

        for(int t = indexOfHistoric_last_yellowCards;
            t < memMap->AccountingUnit[auIndex].indexOfHistoric + (indexOfHistoric_last_yellowCards > memMap->AccountingUnit[auIndex].indexOfHistoric ? MAX_TRADES_LIST_HISTORIC : 0);
            t++) {

            int t_idx = t % MAX_TRADES_LIST_HISTORIC;
            tradeElement_t *thisTrade=&(memMap->AccountingUnit[auIndex].historic[t_idx]);
            idtype tiIndex = memMap->tradingInterfaceIndex[thisTrade->params.tiId];


            if(thisTrade->params.tradeType==TRADE_TYPE_MARKET) {

                if (
                            (thisTrade->info.status!=TRADE_STATE_EXECUTED) ||
                            (abs(thisTrade->params.finishedQuantity - thisTrade->params.quantity) >= min_trade_step)
                    ) {

                        // This is a failed market trade!!
                        tiOperation[tiIndex].yellowCardCounter++;
                        tiOperation[tiIndex].yellowCardTimestamp=CALLBACK_systime.sec;
                        tiOperation[tiIndex].yellowCard=true;
                        tiOperation[tiIndex].successfulMarketTradesInaRow=0;
                        sprintf(msg, "--- TI #%d (%s) given a yellow card!! (market trade failed)",
                                tiIndex, memMap->tradingInterfaceName[tiIndex]);
                        add_message(msg, STR_MSG_SYSTEM_INFO);

               } else {

                           tiOperation[tiIndex].successfulMarketTradesInaRow++;

               }

            } // Trade is MARKET


            if(
                            (thisTrade->params.tradeType==TRADE_TYPE_LIMIT) &&
                            (
                                    (thisTrade->params.timeInForce==TRADE_TIMEINFORCE_INMED_OR_CANCEL) ||
                                    (thisTrade->params.timeInForce==TRADE_TIMEINFORCE_FILL_OR_KILL)
                            ) &&
                            (thisTrade->params.reservedData >= MACRO_COMMAND_RESERVED_DATA_MIN_RANGE) &&
                            (thisTrade->params.reservedData <= MACRO_COMMAND_RESERVED_DATA_MAX_RANGE)
                     ) {

                    if(
                            (thisTrade->info.status!=TRADE_STATE_EXECUTED) ||
                            (abs(thisTrade->params.finishedQuantity - thisTrade->params.quantity) >= min_trade_step)
                       ) {
                        // This is a failed immediate limit trade

                        tiOperation[tiIndex].failedImmediateLimitTradesInaRow++;
                        tiOperation[tiIndex].successfulImmediateLimitTradesInaRow=0;

                        if(tiOperation[tiIndex].failedImmediateLimitTradesInaRow>=n_failed_immediate_limits_in_a_row_for_yellow_card) {

                            tiOperation[tiIndex].failedImmediateLimitTradesInaRow=0;
                            tiOperation[tiIndex].yellowCardCounter++;
                            tiOperation[tiIndex].yellowCardTimestamp=CALLBACK_systime.sec;
                            tiOperation[tiIndex].yellowCard=true;
                            sprintf(msg, "--- TI #%d (%s) given a yellow card!! (too many failed immediate limits in a row)",
                                    tiIndex, memMap->tradingInterfaceName[tiIndex]);
                            add_message(msg, STR_MSG_SYSTEM_INFO);

                        }

                    } else {

                            tiOperation[tiIndex].failedImmediateLimitTradesInaRow=0;
                            tiOperation[tiIndex].successfulImmediateLimitTradesInaRow++;
                    }

            } // Trade is immediate limit and is supervised


            if(thisTrade->info.status==TRADE_STATE_REJECTED) {

                tiOperation[memMap->tradingInterfaceIndex[thisTrade->params.tiId]].rejectsInARow++;

            } else {

                tiOperation[memMap->tradingInterfaceIndex[thisTrade->params.tiId]].rejectsInARow=0;

            }

    //            print_prices_full_book(memMap, thisTrade->params.security);
        } // for

        indexOfHistoric_last_yellowCards = memMap->AccountingUnit[auIndex].indexOfHistoric;


    } // if indexOfHistoric changed


    // Now managing yellow and red cards

    for(int tiIndex=0; tiIndex<memMap->nTradingInterfaces; tiIndex++) {

        if(!tiOperation[tiIndex].redCard) {

            if(tiOperation[tiIndex].rejectsInARow>= n_rejects_in_a_row_for_yellow_card) {
                tiOperation[tiIndex].rejectsInARow=0;
                tiOperation[tiIndex].yellowCardCounter++;
                tiOperation[tiIndex].yellowCardTimestamp=CALLBACK_systime.sec;
                tiOperation[tiIndex].yellowCard=true;
                sprintf(msg, "--- TI #%d (%s) given a yellow card!! (too many rejects in a row)",
                        tiIndex, memMap->tradingInterfaceName[tiIndex]);
                add_message(msg, STR_MSG_SYSTEM_INFO);
            }

            if(
                    tiOperation[tiIndex].yellowCard &&
                    (CALLBACK_systime.sec-tiOperation[tiIndex].yellowCardTimestamp>=yellow_card_putout_time)
              ) {
                    tiOperation[tiIndex].yellowCard=false;
                    sprintf(msg, "--- Yellow card removed from TI #%d (%s) (after put out time)",
                            tiIndex, memMap->tradingInterfaceName[tiIndex]);
                    add_message(msg, STR_MSG_SYSTEM_INFO);
            }

            if(
                    (tiOperation[tiIndex].yellowCardCounter>0) &&
                    (tiOperation[tiIndex].successfulImmediateLimitTradesInaRow >= n_successful_immediate_limits_to_remove_yellow_card)
              ) {
                tiOperation[tiIndex].successfulImmediateLimitTradesInaRow=0;
                tiOperation[tiIndex].yellowCardCounter--;
                tiOperation[tiIndex].yellowCard=false;
                tiOperation[tiIndex].redCard=false; // just in case ;-D
                sprintf(msg, "--- Yellow card counter decreased for TI #%d (%s) (good quote trades in a row)",
                        tiIndex, memMap->tradingInterfaceName[tiIndex]);
                add_message(msg, STR_MSG_SYSTEM_INFO);
            }

            if(
                    (tiOperation[tiIndex].yellowCardCounter>0) &&
                    (tiOperation[tiIndex].successfulMarketTradesInaRow >= n_successful_markets_to_remove_yellow_card)
              ) {
                tiOperation[tiIndex].successfulMarketTradesInaRow=0;
                tiOperation[tiIndex].yellowCardCounter--;
                tiOperation[tiIndex].yellowCard=false;
                tiOperation[tiIndex].redCard=false; // just in case ;-D
                sprintf(msg, "--- Yellow card counter decreased for TI #%d (%s) (good market trades in a row)",
                        tiIndex, memMap->tradingInterfaceName[tiIndex]);
                add_message(msg, STR_MSG_SYSTEM_INFO);
            }

            if(tiOperation[tiIndex].yellowCardCounter>=n_yellow_cards_for_red_card) {
                tiOperation[tiIndex].yellowCardCounter=0;
                tiOperation[tiIndex].redCard=true;
                sprintf(msg, "--- TI #%d (%s) given a red card!! (too many yellow cards)",
                        tiIndex, memMap->tradingInterfaceName[tiIndex]);
                add_message(msg, STR_MSG_SYSTEM_INFO);
            }

        } // !redCard

    } // for tiIndex


    return OK;

}




int32 do_monitoring (sharedMemory_t* memMap,
                     flagsChanges_t* changesMap)
{

    char msg[SLEN];
    int32 auIndex = memMap->strAuIndex;
    int32 epIndex = memMap->strEpIndex;

    withinOperatingHours=checkWithinOperatingHours();
    int last_day = today;
    today = CALLBACK_strtime.day;

#ifdef _CHECK_SORTING
    // Top of book sorting check
        boolean sortOk = true;
        for (int ti = 0; !MalfunctionAlarm && (ti < memMap->mapSortBidNumberOfTi[_SECURITY_TO_CHECK]-1); ti++)
        {
            if (memMap->mapTiTbBid[memMap->mapSortingBid[_SECURITY_TO_CHECK][ti].tiIndex][_SECURITY_TO_CHECK].price <
                memMap->mapTiTbBid[memMap->mapSortingBid[_SECURITY_TO_CHECK][ti+1].tiIndex][_SECURITY_TO_CHECK].price)
            {
                stubDbg(DEBUG_ERROR_LEVEL, "??? Found a sorting error in Top of Book BID sorting array\n");
                sortOk=false;
                break;
            }
        }
        for (int ti = 0; !MalfunctionAlarm && (ti < memMap->mapSortAskNumberOfTi[_SECURITY_TO_CHECK]-1); ti++)
        {
            if (memMap->mapTiTbAsk[memMap->mapSortingAsk[_SECURITY_TO_CHECK][ti].tiIndex][_SECURITY_TO_CHECK].price >
                memMap->mapTiTbAsk[memMap->mapSortingAsk[_SECURITY_TO_CHECK][ti+1].tiIndex][_SECURITY_TO_CHECK].price)
            {
                stubDbg(DEBUG_ERROR_LEVEL, "??? Found a sorting error in Top of Book ASK sorting array\n");
                sortOk=false;
                break;
            }
        }
        if(!MalfunctionAlarm && !sortOk) {
            add_message("Top of book sorting error found", STR_MSG_ERROR);
            sprintf(MalfunctionAlarmText, "Top of book sorting error found");
            MalfunctionAlarm = true;
            print_prices(memMap, _SECURITY_TO_CHECK, true, true);
            print_prices_full_book(memMap, _SECURITY_TO_CHECK);
        }
    // Done

// Full book sorting check
        sortOk = true;
        for(int q=0; !MalfunctionAlarm && (q < memMap->mapFBNumberofsortingBid[_SECURITY_TO_CHECK]-1); q++)
        {
            if (memMap->mapFBQuotesBid[memMap->mapFBSortingBid[_SECURITY_TO_CHECK][q].tiIndex][_SECURITY_TO_CHECK][memMap->mapFBSortingBid[_SECURITY_TO_CHECK][q].quoteIndex].price <
                memMap->mapFBQuotesBid[memMap->mapFBSortingBid[_SECURITY_TO_CHECK][q+1].tiIndex][_SECURITY_TO_CHECK][memMap->mapFBSortingBid[_SECURITY_TO_CHECK][q+1].quoteIndex].price)
            {
                stubDbg(DEBUG_ERROR_LEVEL, "??? Found a sorting error in Full Book BID sorting array\n");
                sortOk=false;
                break;
            }
            if (memMap->mapFBQuotesBid[memMap->mapFBSortingBid[_SECURITY_TO_CHECK][q].tiIndex][_SECURITY_TO_CHECK][memMap->mapFBSortingBid[_SECURITY_TO_CHECK][q].quoteIndex].liquidity <= 0)
            {
                stubDbg(DEBUG_ERROR_LEVEL, "??? Found a zero liquidity quote in BID Full Book (quote #%d, TI #%d - %s)\n",
                        q,
                        memMap->mapFBSortingBid[_SECURITY_TO_CHECK][q].tiIndex,
                        memMap->tradingInterfaceName[memMap->mapFBSortingBid[_SECURITY_TO_CHECK][q].tiIndex]
                        );
                sortOk=false;
            }
        } // loop
        if (
                !MalfunctionAlarm &&
                (memMap->mapFBNumberofsortingBid[_SECURITY_TO_CHECK] > 0) &&
                (memMap->mapFBQuotesBid[memMap->mapFBSortingBid[_SECURITY_TO_CHECK][memMap->mapFBNumberofsortingBid[_SECURITY_TO_CHECK]-1].tiIndex][_SECURITY_TO_CHECK][memMap->mapFBSortingBid[_SECURITY_TO_CHECK][memMap->mapFBNumberofsortingBid[_SECURITY_TO_CHECK]-1].quoteIndex].liquidity <= 0)
           ) {
            stubDbg(DEBUG_ERROR_LEVEL, "??? Found a zero liquidity quote in BID Full Book (quote #%d, TI #%d - %s)\n",
                    memMap->mapFBNumberofsortingBid[_SECURITY_TO_CHECK]-1,
                    memMap->mapFBSortingBid[_SECURITY_TO_CHECK][memMap->mapFBNumberofsortingBid[_SECURITY_TO_CHECK]-1].tiIndex,
                    memMap->tradingInterfaceName[memMap->mapFBSortingBid[_SECURITY_TO_CHECK][memMap->mapFBNumberofsortingBid[_SECURITY_TO_CHECK]-1].tiIndex]
                    );
            sortOk=false;
        }
        for(int q=0; !MalfunctionAlarm && (q < memMap->mapFBNumberofsortingAsk[_SECURITY_TO_CHECK]-1); q++)
        {
            if (memMap->mapFBQuotesAsk[memMap->mapFBSortingAsk[_SECURITY_TO_CHECK][q].tiIndex][_SECURITY_TO_CHECK][memMap->mapFBSortingAsk[_SECURITY_TO_CHECK][q].quoteIndex].price >
                memMap->mapFBQuotesAsk[memMap->mapFBSortingAsk[_SECURITY_TO_CHECK][q+1].tiIndex][_SECURITY_TO_CHECK][memMap->mapFBSortingAsk[_SECURITY_TO_CHECK][q+1].quoteIndex].price)
            {
                stubDbg(DEBUG_ERROR_LEVEL, "??? Found a sorting error in Full Book Ask sorting array\n");
                sortOk=false;
                break;
            }
            if (memMap->mapFBQuotesAsk[memMap->mapFBSortingAsk[_SECURITY_TO_CHECK][q].tiIndex][_SECURITY_TO_CHECK][memMap->mapFBSortingAsk[_SECURITY_TO_CHECK][q].quoteIndex].liquidity <= 0)
            {
                stubDbg(DEBUG_ERROR_LEVEL, "??? Found a zero liquidity quote in ASK Full Book (quote #%d, TI #%d - %s)\n",
                        q,
                        memMap->mapFBSortingAsk[_SECURITY_TO_CHECK][q].tiIndex,
                        memMap->tradingInterfaceName[memMap->mapFBSortingAsk[_SECURITY_TO_CHECK][q].tiIndex]
                        );
                sortOk=false;
            }
        } // loop
        if (
                !MalfunctionAlarm &&
                (memMap->mapFBNumberofsortingAsk[_SECURITY_TO_CHECK]) &&
                (memMap->mapFBQuotesAsk[memMap->mapFBSortingAsk[_SECURITY_TO_CHECK][memMap->mapFBNumberofsortingAsk[_SECURITY_TO_CHECK]-1].tiIndex][_SECURITY_TO_CHECK][memMap->mapFBSortingAsk[_SECURITY_TO_CHECK][memMap->mapFBNumberofsortingAsk[_SECURITY_TO_CHECK]-1].quoteIndex].liquidity <= 0)
           ) {
            stubDbg(DEBUG_ERROR_LEVEL, "??? Found a zero liquidity quote in ASK Full Book (quote #%d, TI #%d - %s)\n",
                    memMap->mapFBNumberofsortingAsk[_SECURITY_TO_CHECK]-1,
                    memMap->mapFBSortingAsk[_SECURITY_TO_CHECK][memMap->mapFBNumberofsortingAsk[_SECURITY_TO_CHECK]-1].tiIndex,
                    memMap->tradingInterfaceName[memMap->mapFBSortingAsk[_SECURITY_TO_CHECK][memMap->mapFBNumberofsortingAsk[_SECURITY_TO_CHECK]-1].tiIndex]
                    );
            sortOk=false;
        }
        if(!MalfunctionAlarm && !sortOk) {
            add_message("Full book sorting error found", STR_MSG_ERROR);
            sprintf(MalfunctionAlarmText, "Full book sorting error found");
            MalfunctionAlarm = true;
            print_prices_full_book(memMap, _SECURITY_TO_CHECK);
        }
// Done

#endif



#ifdef _CHECK_FULL_BOOK_AGGREGATION

        for(idtype tiIndex=0; !MalfunctionAlarm && (tiIndex<memMap->nTradingInterfaces); tiIndex++) {

            for(idtype sec=0; sec<NUM_SECURITIES; sec++) {

                int firstquote=-1;
                for(int q=0; q<memMap->mapFBNumberofsortingAsk[sec]; q++) {
                    if(memMap->mapFBSortingAsk[sec][q].tiIndex==tiIndex) {
                        firstquote=q;
                        break;
                    }
                }

                if(firstquote>=0) {

                    if(
                            (memMap->mapFBQuotesAsk[tiIndex][sec][memMap->mapFBSortingAsk[sec][firstquote].quoteIndex].price != memMap->mapTiTbAsk[tiIndex][sec].price)||
                            (memMap->mapFBSortingAsk[sec][firstquote].price != memMap->mapTiTbAsk[tiIndex][sec].price)
                       ) {
                        stubDbg(DEBUG_ERROR_LEVEL,
                                "??? Inconsistent top of book ASK price for TI #%d (%s) in %s\n"
                                "  => mapTiTbAsk price is %d\n"
                                "  => mapFBQuotesAsk price is %d\n"
                                "  => mapFBSortingAsk price is %d\n",
                                tiIndex,
                                memMap->tradingInterfaceName[tiIndex],
                                securityName[sec],
                                memMap->mapTiTbAsk[tiIndex][sec].price,
                                memMap->mapFBQuotesAsk[tiIndex][sec][memMap->mapFBSortingAsk[sec][firstquote].quoteIndex].price,
                                memMap->mapFBSortingAsk[sec][firstquote].price
                               );
                        add_message("Inconsistent top of book ASK price", STR_MSG_ERROR);
                        sprintf(MalfunctionAlarmText, "Inconsistent top of book ASK price");
                        MalfunctionAlarm = true;
                        print_prices_full_book(memMap, sec);
                    }

                } else {
                    // Do nothing: no prices are available from this TI, although there will be a top of book price
                }


                firstquote=-1;
                for(int q=0; q<memMap->mapFBNumberofsortingBid[sec]; q++) {
                    if(memMap->mapFBSortingBid[sec][q].tiIndex==tiIndex) {
                        firstquote=q;
                        break;
                    }
                }

                if(firstquote>=0) {

                    if(
                            (memMap->mapFBQuotesBid[tiIndex][sec][memMap->mapFBSortingBid[sec][firstquote].quoteIndex].price != memMap->mapTiTbBid[tiIndex][sec].price)||
                            (memMap->mapFBSortingBid[sec][firstquote].price != memMap->mapTiTbBid[tiIndex][sec].price)
                       ) {
                        stubDbg(DEBUG_ERROR_LEVEL,
                                "??? Inconsistent top of book BID price for TI #%d (%s) in %s\n"
                                "  => mapTiTbBid price is %d\n"
                                "  => mapFBQuotesBid price is %d\n"
                                "  => mapFBSortingBid price is %d\n",
                                tiIndex,
                                memMap->tradingInterfaceName[tiIndex],
                                securityName[sec],
                                memMap->mapTiTbBid[tiIndex][sec].price,
                                memMap->mapFBQuotesBid[tiIndex][sec][memMap->mapFBSortingBid[sec][firstquote].quoteIndex].price,
                                memMap->mapFBSortingBid[sec][firstquote].price
                               );
                        add_message("Inconsistent top of book BID price", STR_MSG_ERROR);
                        sprintf(MalfunctionAlarmText, "Inconsistent top of book BID price");
                        MalfunctionAlarm = true;
                        print_prices_full_book(memMap, sec);
                    }

                } else {
                    // Do nothing: no prices are available from this TI, although there will be a top of book price
                }

            } // securities loop

        } // tiIndex loop
#endif



#ifdef _SHOW_DEBUG_PRICES
    char dbg_msg[8192];
    char dbg_str[8192];
    sprintf(dbg_msg, ",Prices_data,");
    for(int i = 0; i < memMap ->nTradingInterfaces; i++) {
        char dbg_str[8192];
        sprintf(dbg_str, "%d,%d,%d,%d,%d,%d,",
                      memMap->mapTiTbBid[i][EUR_USD].price,memMap->mapTiTbAsk[i][EUR_USD].price,
                      memMap->mapTiTbBid[i][GBP_USD].price,memMap->mapTiTbAsk[i][GBP_USD].price,
                      memMap->mapTiTbBid[i][USD_CHF].price,memMap->mapTiTbAsk[i][USD_CHF].price
               );
        strcat(dbg_msg, dbg_str);
    }
    sprintf(dbg_str, "%d,%d,%d,%d,%d,%d,%.5f,%.5f,%.5f,%.5f,%.0f,%.0f\n",
                  memMap->mapBookSpBidInt[EUR_USD], memMap->mapBookSpAskInt[EUR_USD],
                  memMap->mapBookSpBidInt[GBP_USD], memMap->mapBookSpAskInt[GBP_USD],
                  memMap->mapBookSpBidInt[USD_CHF], memMap->mapBookSpAskInt[USD_CHF],
                  memMap->mapBookAp[EUR].price, memMap->mapBookAp[USD].price, memMap->mapBookAp[GBP].price, memMap->mapBookAp[CHF].price,
                  memMap->GlobalEquity_EP, memMap->GlobalEquity_STR
           );
    strcat(dbg_msg, dbg_str);
    stubDbg(DEBUG_PERFORMANCE, dbg_msg);
#endif


#ifdef _CHECK_TRADES_ALIVE
    uint32 tradesProcessed=0;
    for(uint32 t=0; !MalfunctionAlarm && (tradesProcessed<memMap->AccountingUnit[auIndex].numberOfAlives) && (t<MAX_TRADES_LIST_ALIVE); t++) {
        if(!memMap->AccountingUnit[auIndex].alive[t].alive) continue;
        tradesProcessed++;

        tradeElement_t *thisTrade=&(memMap->AccountingUnit[auIndex].alive[t]);
        tradeCommand_t *thisTradeParams=&(thisTrade->params);

        if(
                (thisTrade->info.status != TRADE_STATE_IN_FLUX) &&
                (thisTrade->info.status != TRADE_STATE_PENDING) &&
                (thisTrade->info.status != TRADE_STATE_INDETERMINED)
          ) {

            char stateS[NLEN];
            getTradeStatusString(thisTrade->info.status, thisTrade->info.substatus, stateS);
            stubDbg(DEBUG_ERROR_LEVEL, "??? Trade %s is alive but its state is %s! - this was a %s, %d %s on TI #%d (%s), resvd data %u\n",
                                       thisTrade->ids.fixId,
                                       stateS,
                                       (thisTradeParams->side == TRADE_SIDE_BUY ? "BUY" : "SELL"),
                                       thisTradeParams->quantity,
                                       securityName[thisTradeParams->security],
                                       memMap->tradingInterfaceIndex[thisTradeParams->tiId],
                                       memMap->tradingInterfaceName[memMap->tradingInterfaceIndex[thisTradeParams->tiId]],
                                       thisTradeParams->reservedData
                   );
            add_message("Illegal status found in live trade", STR_MSG_ERROR);
            sprintf(MalfunctionAlarmText, "Illegal status found in live trade");
            MalfunctionAlarm = true;
        }

    } // Loop: trades
#endif

    if(MalfunctionAlarm && strategy_connected) {
        add_message("System malfunction - now disconnecting strategy", STR_MSG_ERROR);
        strategy_connected = false;
    }


    for(int ti = 0; ti < memMap -> nTradingInterfaces; ti++) {
//        if(memMap->tradingInterfaceOk[ti] != tradingInterfaceOk_last[ti]) {
        boolean moreOrLessOkNow = (memMap->tradingInterfaceOk[ti] == TI_STATUS_OK) || (memMap->tradingInterfaceOk[ti] == TI_STATUS_OK_NO_PRICE);
        boolean moreOrLessOkLast = (tradingInterfaceOk_last[ti] == TI_STATUS_OK) || (tradingInterfaceOk_last[ti] == TI_STATUS_OK_NO_PRICE);
        if(moreOrLessOkNow != moreOrLessOkLast) {
            char old_status[SLEN], new_status[SLEN];
            getTIStatusString(tradingInterfaceOk_last[ti], old_status);
            getTIStatusString(memMap->tradingInterfaceOk[ti], new_status);
            sprintf(msg, "--- Trading interface #%d (%s) was %s and now is %s",
                    ti,
                    memMap->tradingInterfaceName[ti],
                    old_status,
                    new_status
                   );
            add_message(msg, STR_MSG_SYSTEM_INFO);
            tradingInterfaceOk_last[ti]=memMap->tradingInterfaceOk[ti];
        }
    }


    // We do some basic monitoring here



    // Now generating alarms

    boolean disconnectSet = false;

    number GlobalEquity_STR = memMap->AccountingUnit[auIndex].GlobalEquity_STR;

    if(     (disconnect_on_valuation_change_too_big == 1) &&
            (abs(GlobalEquity_STR - GlobalEquity_STR_last) > valuation_change_threshold)
       ) {
        sprintf(msg,  "!!! Strategy equity valuation changed drastically from %.2f to %.2f",
                GlobalEquity_STR_last, GlobalEquity_STR);
        add_message(msg, STR_MSG_ERROR);

        for(int s=0; s<many_securities; s++) {
            idtype sec=which_securities[s];
            print_prices(memMap, sec, true, true);
            print_exposures_au(memMap, auIndex, sec);
            stubDbg(DEBUG_INFO_LEVEL, "System price for %s is %d/%d (avg %d)\n",
                    securityName[sec],
                    memMap->mapBookSpBidInt[sec],
                    memMap->mapBookSpAskInt[sec],
                    memMap->mapBookSpInt[sec]);

        } // securities loop

        for(int a=0; a<many_assets; a++) {
            idtype asset=which_assets[a];
            stubDbg(DEBUG_INFO_LEVEL, "System (asset) price for %s is %d\n",
                    assetName[asset],
                    memMap->mapBookApInt[asset]);

        } // assets loop

        for(int pb=0; pb<memMap->nPrimeBrokers; pb++) {
            stubDbg(DEBUG_INFO_LEVEL, "AU equity mark to market for PB #%d (%s) is %.1f, EP is %.1f, total is %.1f\n",
                    pb, memMap->primeBrokerName[pb], memMap->AccountingUnit[auIndex].Equity_STR[pb], memMap->EquityPool[epIndex].Equity_EP[pb], memMap->Equity_PB[pb]);
        } // prime brokers loop

        ValuationChangeAlarm = true;
        disconnectSet = true;
    } // if valuation change too big

    GlobalEquity_STR_last = GlobalEquity_STR;

    if(GlobalEquity_STR > maxEquity + _MATERIALITY_THRES_) {
        maxEquity = GlobalEquity_STR;
        stubDbg(DEBUG_INFO_LEVEL,  "--- New maximum equity level reached @ %.2f\n", maxEquity);
//        sprintf(msg,  "--- New maximum equity level reached @ %.2f", maxEquity);
//        add_message(msg, STR_MSG_SYSTEM_INFO);
    } else if(
            (disconnect_on_drawdown_too_big == 1) &&
            (GlobalEquity_STR <= maxEquity - drawdown_threshold)
      ) {
        sprintf(msg,  "### Drawdown limit reached: maxEquity was %.2f and now current equity is %.2f\n  --@ maxEquity adjusted to %.2f",
                maxEquity, GlobalEquity_STR, GlobalEquity_STR);
        add_message(msg, STR_MSG_ALARM);
        maxEquity = GlobalEquity_STR;
        DrawdownAlarm = true;
        disconnectSet = true;
    }


    if(nTradesThisSecond > too_many_trades_threshold_1_sec) {
        nTradesThisSecond = 0;
        TooManyTradesAlarm = true;
        add_message("### Too many trades in one second!!", STR_MSG_ALARM);
        if(disconnect_on_too_many_trades == 1) disconnectSet = true;
    }

    if(nTradesThis10Seconds > too_many_trades_threshold_10_sec) {
        nTradesThis10Seconds = 0;
        TooManyTradesAlarm = true;
        add_message("### Too many trades in 10 seconds!!", STR_MSG_ALARM);
        if(disconnect_on_too_many_trades == 1) disconnectSet = true;
    }

    if(nTradesThisMinute > too_many_trades_threshold_1_min) {
        nTradesThisMinute = 0;
        TooManyTradesAlarm = true;
        add_message("### Too many trades in one minute!!", STR_MSG_ALARM);
        if(disconnect_on_too_many_trades == 1) disconnectSet = true;
    }

    if(disconnectSet) {
//        auto_management_connected = false;
        strategy_connected = false;
        add_message("### Strategy has been disconnected, as watchdog is on", STR_MSG_ALARM);
    }


    if(lastSecond != CALLBACK_strtime.sec) {
        lastSecond = CALLBACK_strtime.sec;
        nTradesThisSecond = 0;
    }

    if(last10Seconds != (CALLBACK_strtime.sec / 10)) {
        last10Seconds = CALLBACK_strtime.sec / 10;
        nTradesThis10Seconds = 0;
    }

    if(lastMinute != CALLBACK_strtime.min) {
        lastMinute = CALLBACK_strtime.min;
        nTradesThisMinute = 0;
    }



    if(last_day!=today) {
        sprintf(msg, "--- Reseting statistics at the turn of the day - printing final statistics to log");
        add_message(msg, STR_MSG_SYSTEM_INFO);
        for(int tiIndex=0; tiIndex<memMap->nTradingInterfaces; tiIndex++) {
            stubDbg(DEBUG_INFO_LEVEL, "=> TI #%d (%s)\n"
                                      "    - Total immediate limit trades finished today %d (%d fully filled, %d partially filled, %d failed, %d user canceled\n"
                                      "        (Total bad quotes: %d, Total good quotes: %d)\n"
                                      "    - Total market trades finished today %d (%d fully filled, %d partially filled, %d failed, %d user canceled\n"
                                      "    - Total other trades finished today %d (%d fully filled, %d partially filled, %d failed, %d user canceled\n"
                                      "    - Total replaces done: %d\n",
                                      tiIndex,
                                      memMap->tradingInterfaceName[tiIndex],
                                      tiPerformanceInfo[tiIndex].fullyFilledImmediateLimitTradesToday +
                                      tiPerformanceInfo[tiIndex].partiallyFilledImmediateLimitTradesToday +
                                      tiPerformanceInfo[tiIndex].failedAndRejectedImmediateLimitTradesToday +
                                      tiPerformanceInfo[tiIndex].userCanceledImmediateLimitTradesToday,
                                      tiPerformanceInfo[tiIndex].fullyFilledImmediateLimitTradesToday,
                                      tiPerformanceInfo[tiIndex].partiallyFilledImmediateLimitTradesToday,
                                      tiPerformanceInfo[tiIndex].failedAndRejectedImmediateLimitTradesToday,
                                      tiPerformanceInfo[tiIndex].userCanceledImmediateLimitTradesToday,
                                      tiOperation[tiIndex].badQuotesToday,
                                      tiOperation[tiIndex].goodQuotesToday,
                                      tiPerformanceInfo[tiIndex].fullyFilledMarketTradesToday +
                                      tiPerformanceInfo[tiIndex].partiallyFilledMarketTradesToday +
                                      tiPerformanceInfo[tiIndex].failedAndRejectedMarketTradesToday +
                                      tiPerformanceInfo[tiIndex].userCanceledMarketTradesToday,
                                      tiPerformanceInfo[tiIndex].fullyFilledMarketTradesToday,
                                      tiPerformanceInfo[tiIndex].partiallyFilledMarketTradesToday,
                                      tiPerformanceInfo[tiIndex].failedAndRejectedMarketTradesToday,
                                      tiPerformanceInfo[tiIndex].userCanceledMarketTradesToday,
                                      tiPerformanceInfo[tiIndex].fullyFilledOtherTradesToday +
                                      tiPerformanceInfo[tiIndex].partiallyFilledOtherTradesToday +
                                      tiPerformanceInfo[tiIndex].failedAndRejectedOtherTradesToday +
                                      tiPerformanceInfo[tiIndex].userCanceledOtherTradesToday,
                                      tiPerformanceInfo[tiIndex].fullyFilledOtherTradesToday,
                                      tiPerformanceInfo[tiIndex].partiallyFilledOtherTradesToday,
                                      tiPerformanceInfo[tiIndex].failedAndRejectedOtherTradesToday,
                                      tiPerformanceInfo[tiIndex].userCanceledOtherTradesToday
                   );

        }  // for (tiIndex)

        for(idtype pbIndex=0; pbIndex<memMap->nPrimeBrokers; pbIndex++) {

            tradeIndicators_t todayInd;
            buildTodayNonSavedTradeIndicatorsForPB(memMap, pbIndex, &todayInd);

            // 0. TODAY
            resetIndicatorsAndEquity(memMap, auIndex, pbIndex, &(indicators[TIMEFRAME_TODAY][pbIndex]), &(equityValue[TIMEFRAME_TODAY][pbIndex]));

            // 1. THIS WEEK
            if(CALLBACK_strtime.dayofweek==SUNDAY) {
                resetIndicatorsAndEquity(memMap, auIndex, pbIndex, &(indicators[TIMEFRAME_THIS_WEEK][pbIndex]), &(equityValue[TIMEFRAME_THIS_WEEK][pbIndex]));
            } else {
                indicators[TIMEFRAME_THIS_WEEK][pbIndex].totalTrades+=todayInd.totalTrades;
                indicators[TIMEFRAME_THIS_WEEK][pbIndex].fullyFilledTrades+=todayInd.fullyFilledTrades;
                indicators[TIMEFRAME_THIS_WEEK][pbIndex].partiallyFilledTrades+=todayInd.partiallyFilledTrades;
                indicators[TIMEFRAME_THIS_WEEK][pbIndex].failedAndRejectedTrades+=todayInd.failedAndRejectedTrades;
                indicators[TIMEFRAME_THIS_WEEK][pbIndex].userCanceledTrades+=todayInd.userCanceledTrades;
                indicators[TIMEFRAME_THIS_WEEK][pbIndex].replacesDone+=todayInd.replacesDone;
                indicators[TIMEFRAME_THIS_WEEK][pbIndex].amountTraded+=todayInd.amountTraded;
            }

            // 2. THIS MONTH
            if(CALLBACK_strtime.day==1) {
                resetIndicatorsAndEquity(memMap, auIndex, pbIndex, &(indicators[TIMEFRAME_THIS_MONTH][pbIndex]), &(equityValue[TIMEFRAME_THIS_MONTH][pbIndex]));
            } else {
                indicators[TIMEFRAME_THIS_MONTH][pbIndex].totalTrades+=todayInd.totalTrades;
                indicators[TIMEFRAME_THIS_MONTH][pbIndex].fullyFilledTrades+=todayInd.fullyFilledTrades;
                indicators[TIMEFRAME_THIS_MONTH][pbIndex].partiallyFilledTrades+=todayInd.partiallyFilledTrades;
                indicators[TIMEFRAME_THIS_MONTH][pbIndex].failedAndRejectedTrades+=todayInd.failedAndRejectedTrades;
                indicators[TIMEFRAME_THIS_MONTH][pbIndex].userCanceledTrades+=todayInd.userCanceledTrades;
                indicators[TIMEFRAME_THIS_MONTH][pbIndex].replacesDone+=todayInd.replacesDone;
                indicators[TIMEFRAME_THIS_MONTH][pbIndex].amountTraded+=todayInd.amountTraded;
            }

            // 3. SINCE INCEPTION
            indicators[TIMEFRAME_SINCE_INCEPTION][pbIndex].totalTrades+=todayInd.totalTrades;
            indicators[TIMEFRAME_SINCE_INCEPTION][pbIndex].fullyFilledTrades+=todayInd.fullyFilledTrades;
            indicators[TIMEFRAME_SINCE_INCEPTION][pbIndex].partiallyFilledTrades+=todayInd.partiallyFilledTrades;
            indicators[TIMEFRAME_SINCE_INCEPTION][pbIndex].failedAndRejectedTrades+=todayInd.failedAndRejectedTrades;
            indicators[TIMEFRAME_SINCE_INCEPTION][pbIndex].userCanceledTrades+=todayInd.userCanceledTrades;
            indicators[TIMEFRAME_SINCE_INCEPTION][pbIndex].replacesDone+=todayInd.replacesDone;
            indicators[TIMEFRAME_SINCE_INCEPTION][pbIndex].amountTraded+=todayInd.amountTraded;

            for(idtype tf=0; tf<NUM_TIMEFRAMES; tf++) {

                writeIndicators(pbIndex, tf, &(indicators[tf][pbIndex]), equityValue[tf][pbIndex]);

            } // for timeframes

        } // for pbs

        eraseTIPerformanceRecords(memMap, auIndex);

    } // if change of day


// Now storing profiling samples for histograms

    if(connect_profiling) {

        boolean delayDetectedInThisMT[MAX_NUMBER_TI] = {false};

        for(int c=0; c<changesMap->numberChanges; c++) {

            idtype tiIndex;
            idtype security;
            idtype quoteIndex;
            mtbookElement_t *quote;

            boolean toBeAdded = true;

            switch(changesMap->changes[c].mtCode) {

                case MTC_FBD_BID :
                    tiIndex = changesMap->changes[c].first;
                    security = changesMap->changes[c].second;
                    quoteIndex = changesMap->changes[c].third;
                    quote = &(memMap -> mapFBQuotesBid[tiIndex][security][quoteIndex]);
                    break;

                case MTC_FBD_ASK :
                    tiIndex = changesMap->changes[c].first;
                    security = changesMap->changes[c].second;
                    quoteIndex = changesMap->changes[c].third;
                    quote = &(memMap -> mapFBQuotesAsk[tiIndex][security][quoteIndex]);
                    break;
/*
                case MTC_FBA_BID :
                    tiIndex = changesMap->changes[c].first;
                    security = changesMap->changes[c].second;
                    numberOfLevelsFBA = changesMap->changes[C].third;
                    AND NOW WHAT??
                    break;

                case MTC_FBA_ASK :
                    tiIndex = changesMap->changes[c].first;
                    security = changesMap->changes[c].second;
                    numberOfLevelsFBA = changesMap->changes[C].third;
                    AND NOW WHAT??
                    break;
*/

// Uncomment this as soon as we can know what is the subscription level
/*
                case MTC_TOPBID :
                    tiIndex = changesMap->changes[c].first;
                    security = changesMap->changes[c].second;
                    quote = &(memMap -> mapTiTbBid[tiIndex][security]);
                    break;

                case MTC_TOPASK :
                    tiIndex = changesMap->changes[c].first;
                    security = changesMap->changes[c].second;
                    quote = &(memMap -> mapTiTbAsk[tiIndex][security]);
                    break;
*/

                default :
                    toBeAdded = false;

            }

            if(toBeAdded) {

                strcpy(FixTimesStr[profilingPtr[tiIndex]][tiIndex], quote->fixTime);
                CoreTimes[profilingPtr[tiIndex]][tiIndex]=quote->timestamp;
                StubTimes[profilingPtr[tiIndex]][tiIndex]=CALLBACK_systime;
                callback_numbers[profilingPtr[tiIndex]][tiIndex]=callback_number;

                profilingPtr[tiIndex]++;
                if((profilingPtr[tiIndex]>=n_samples_for_profiling)||(profilingPtr[tiIndex]>=MAX_PROFILING_SAMPLES)) {
                    profilingPtr[tiIndex]=0;
                    profilingBufferFull[tiIndex]=true;
                }


                // Now to measure and process quote delays
                timestruct_t fixtime_str;
                uint fixtime_ms;
                timetype_t fixtimetype;
                int args = sscanf(quote->fixTime, "%4hu%2hu%2hu-%2hu:%2hu:%2hu.%3u",
                                                  &fixtime_str.year,
                                                  &fixtime_str.month,
                                                  &fixtime_str.day,
                                                  &fixtime_str.hour,
                                                  &fixtime_str.min,
                                                  &fixtime_str.sec,
                                                  &fixtime_ms
                                 );
                if(args!=7) {
                    stubDbg(DEBUG_ERROR_LEVEL,
                            "??? Wrong FIX timestamp (%s), TI is #%d (%s), MT time is %s\n",
                            quote->fixTime,
                            tiIndex,
                            memMap->tradingInterfaceName[tiIndex],
                            MTtimeStr
                    );
                    continue;
                }

                dateTotimestamp (&fixtime_str, &(fixtimetype.sec));
                fixtimetype.usec = 1000*fixtime_ms;

                int64 thisDelay = my_getTimeDifSigned(&fixtimetype, &CALLBACK_systime) / 1000;
                if(thisDelay < minFixToStubDelay_ms[tiIndex])
                    minFixToStubDelay_ms[tiIndex] = thisDelay;

                pricesDelayedLast[tiIndex] = pricesDelayed[tiIndex];

                if(thisDelay - minFixToStubDelay_ms[tiIndex] >= fixToStubDelayThreshold_ms) {

                    delayDetectedInThisMT[tiIndex] = true;
                    pricesDelayed[tiIndex] = true;
                    if(!pricesDelayedLast[tiIndex]) {

/*
                        stubDbg(DEBUG_INFO_LEVEL, "!!! Prices delayed in TI #%d (%s): abs delay is %ld, min is %ld, "
                                                  "therefore relative delay is %ld (FIX time is %s, "
                                                  "MT time is %s)\n",
                                                  tiIndex,
                                                  memMap->tradingInterfaceName[tiIndex],
                                                  thisDelay,
                                                  minFixToStubDelay_ms[tiIndex],
                                                  thisDelay - minFixToStubDelay_ms[tiIndex],
                                                  quote->fixTime,
                                                  MTtimeStr
                                                  );
*/
// Less verbose:
                        if(reportDelaysAndNoTicks)
                            stubDbg(DEBUG_INFO_LEVEL, "!!! Price delay in %s: %ldms @ MTtime %s\n",
                                                      memMap->tradingInterfaceName[tiIndex],
                                                      thisDelay - minFixToStubDelay_ms[tiIndex],
                                                      MTtimeStr);

                        pricesDelayedInitialTimestamp[tiIndex]=CALLBACK_systime;

                    }

                } // this delay over threshold

                else if(!delayDetectedInThisMT[tiIndex]) {

                    pricesDelayed[tiIndex] = false;
                    if(pricesDelayedLast[tiIndex]) {
                        int64 downTime = my_getTimeDifSigned(&(pricesDelayedInitialTimestamp[tiIndex]), &CALLBACK_systime) / 1000;
                        totalPricesDelayedOccurrences[tiIndex]++;
                        totalPricesDelayedDowntime_ms[tiIndex] += downTime;
/*
                        stubDbg(DEBUG_INFO_LEVEL, "!!! Prices not delayed anymore in TI #%d (%s), total downtime has been %lld ms this time, already #%d times (MT time is %s)\n",
                                                  tiIndex,
                                                  memMap->tradingInterfaceName[tiIndex],
                                                  downTime,
                                                  totalPricesDelayedOccurrences[tiIndex],
                                                  MTtimeStr);
*/
// Less verbose:
                        if(reportDelaysAndNoTicks)
                            stubDbg(DEBUG_INFO_LEVEL, "!!! Price ok in %s after %lld ms @MTtime %s\n",
                                                      memMap->tradingInterfaceName[tiIndex],
                                                      downTime,
                                                      MTtimeStr);

                    }
                    lastTickTimestamp[tiIndex] = CALLBACK_systime;

                } // this delay not over threshold

            } // if toBeAdded

        } // loop changes

        // Now to check no ticks condition

        for(int ti=0; ti < memMap->nTradingInterfaces; ti++) {

            noTicksLast[ti] = noTicks[ti];

            int64 thisDelay = my_getTimeDifSigned(&(lastTickTimestamp[ti]), &CALLBACK_systime) / 1000;
            if(thisDelay > noTicksThreshold_ms) {

                noTicks[ti] = true;

                if(!noTicksLast[ti]) {

                    if(reportDelaysAndNoTicks)
                        stubDbg(DEBUG_INFO_LEVEL, "!!! No ticks in %s @ MTtime %s\n",
                                                  memMap->tradingInterfaceName[ti],
                                                  MTtimeStr);

                    noTicksInitialTimestamp[ti] = CALLBACK_systime;

                } // if !noTicks

            } // if no ticks over threshold
            else {

                noTicks[ti] = false;

                if(noTicksLast[ti]) {

                    int64 downTime = my_getTimeDifSigned(&(noTicksInitialTimestamp[ti]), &CALLBACK_systime) / 1000;
                    totalNoTicksOccurrences[ti]++;
                    totalNoTicksDowntime_ms[ti] += downTime;
/*
                    stubDbg(DEBUG_INFO_LEVEL, "!!! Ticks received again in TI #%d (%s), total downtime has been %lld ms this time, already %d times (MT time is %s)\n",
                                              ti,
                                              memMap->tradingInterfaceName[ti],
                                              downTime,
                                              totalNoTicksOccurrences[ti],
                                              MTtimeStr);
*/
// Less verbose:
                    if(reportDelaysAndNoTicks)
                        stubDbg(DEBUG_INFO_LEVEL, "!!! Ticks ok in %s after %lld ms @ MTtime %s\n",
                                                  memMap->tradingInterfaceName[ti],
                                                  downTime,
                                                  MTtimeStr);

                }

            } // if ticks ok


        } // TI loop

    } // profiling connected

    return OK;
}



void clearHistogram(histogram_t *h) {
    for(int b=0; b<MAX_HISTOGRAM_BUCKETS; b++) {
        h->bucket[b].samples=0;
        h->bucket[b].accSamples=0;
        h->bucket[b].pct=0;
        h->bucket[b].accPct=0;
    }

}



void initHistograms(sharedMemory_t* memMap) { // This initializes the histograms buckets

    for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {

        int counter = 0;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"0 ms"); FixToCore[ti].bucket[counter++].bucketValue=0;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 1 ms"); FixToCore[ti].bucket[counter++].bucketValue=1000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 2 ms"); FixToCore[ti].bucket[counter++].bucketValue=2000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 3 ms"); FixToCore[ti].bucket[counter++].bucketValue=3000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 4 ms"); FixToCore[ti].bucket[counter++].bucketValue=4000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 5 ms"); FixToCore[ti].bucket[counter++].bucketValue=5000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 6 ms"); FixToCore[ti].bucket[counter++].bucketValue=6000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 7 ms"); FixToCore[ti].bucket[counter++].bucketValue=7000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 8 ms"); FixToCore[ti].bucket[counter++].bucketValue=8000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 9 ms"); FixToCore[ti].bucket[counter++].bucketValue=9000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 10 ms"); FixToCore[ti].bucket[counter++].bucketValue=10000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 15 ms"); FixToCore[ti].bucket[counter++].bucketValue=15000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 20 ms"); FixToCore[ti].bucket[counter++].bucketValue=20000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 25 ms"); FixToCore[ti].bucket[counter++].bucketValue=25000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 30 ms"); FixToCore[ti].bucket[counter++].bucketValue=30000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 35 ms"); FixToCore[ti].bucket[counter++].bucketValue=35000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 40 ms"); FixToCore[ti].bucket[counter++].bucketValue=40000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 45 ms"); FixToCore[ti].bucket[counter++].bucketValue=45000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 50 ms"); FixToCore[ti].bucket[counter++].bucketValue=50000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 60 ms"); FixToCore[ti].bucket[counter++].bucketValue=60000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 70 ms"); FixToCore[ti].bucket[counter++].bucketValue=70000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 80 ms"); FixToCore[ti].bucket[counter++].bucketValue=80000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 90 ms"); FixToCore[ti].bucket[counter++].bucketValue=90000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 100 ms"); FixToCore[ti].bucket[counter++].bucketValue=100000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 150 ms"); FixToCore[ti].bucket[counter++].bucketValue=150000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 100 ms"); FixToCore[ti].bucket[counter++].bucketValue=200000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"up to 150 ms"); FixToCore[ti].bucket[counter++].bucketValue=250000;
        sprintf(FixToCore[ti].bucket[counter].bucketName,"over 250 ms");

        FixToCore[ti].nBuckets=counter;
        FixToCore[ti].bucket[FixToCore[ti].nBuckets].bucketValue=FixToCore[ti].bucket[FixToCore[ti].nBuckets-1].bucketValue;
        clearHistogram(&FixToCore[ti]);

        FixToStub[ti] = FixToCore[ti];
        clearHistogram(&FixToStub[ti]);


        counter = 0;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"0 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=0;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 1 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=1000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 2 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=2000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 3 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=3000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 4 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=4000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 5 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=5000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 6 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=6000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 7 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=7000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 8 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=8000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 9 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=9000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 10 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=10000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 12 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=12000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 14 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=14000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 16 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=16000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 18 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=18000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 20 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=20000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 25 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=25000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 30 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=30000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 35 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=35000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 40 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=40000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 45 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=45000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 50 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=50000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 60 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=60000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 70 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=70000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 80 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=80000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 90 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=90000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 100 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=100000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 120 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=120000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 140 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=140000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 160 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=160000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 180 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=180000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 200 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=200000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 250 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=250000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 300 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=300000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 350 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=350000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 400 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=400000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 450 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=450000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 500 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=500000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 600 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=600000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 700 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=700000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 800 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=800000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 900 ms"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=900000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"up to 1 s"); ExecTimesImmediateLimit[ti].bucket[counter++].bucketValue=1000000;
        sprintf(ExecTimesImmediateLimit[ti].bucket[counter].bucketName,"over 1 s");

        ExecTimesImmediateLimit[ti].nBuckets=counter;
        ExecTimesImmediateLimit[ti].bucket[ExecTimesImmediateLimit[ti].nBuckets].bucketValue=ExecTimesImmediateLimit[ti].bucket[ExecTimesImmediateLimit[ti].nBuckets-1].bucketValue;
        clearHistogram(&ExecTimesImmediateLimit[ti]);


        ExecTimesMarket[ti] = ExecTimesImmediateLimit[ti];
        clearHistogram(&ExecTimesMarket[ti]);

        ExecTimesCancel[ti] = ExecTimesImmediateLimit[ti];
        clearHistogram(&ExecTimesCancel[ti]);

        ExecTimesReplace[ti] = ExecTimesImmediateLimit[ti];
        clearHistogram(&ExecTimesReplace[ti]);

    } // for tiIndex

    int counter=0;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 2 us"); CoreToStub.bucket[counter++].bucketValue=2;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 4 us"); CoreToStub.bucket[counter++].bucketValue=4;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 6 us"); CoreToStub.bucket[counter++].bucketValue=6;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 8 us"); CoreToStub.bucket[counter++].bucketValue=8;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 10 us"); CoreToStub.bucket[counter++].bucketValue=10;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 15 us"); CoreToStub.bucket[counter++].bucketValue=15;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 20 us"); CoreToStub.bucket[counter++].bucketValue=20;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 25 us"); CoreToStub.bucket[counter++].bucketValue=25;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 30 us"); CoreToStub.bucket[counter++].bucketValue=30;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 35 us"); CoreToStub.bucket[counter++].bucketValue=35;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 40 us"); CoreToStub.bucket[counter++].bucketValue=40;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 45 us"); CoreToStub.bucket[counter++].bucketValue=45;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 50 us"); CoreToStub.bucket[counter++].bucketValue=50;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 60 us"); CoreToStub.bucket[counter++].bucketValue=60;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 70 us"); CoreToStub.bucket[counter++].bucketValue=70;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 80 us"); CoreToStub.bucket[counter++].bucketValue=80;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 90 us"); CoreToStub.bucket[counter++].bucketValue=90;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 100 us"); CoreToStub.bucket[counter++].bucketValue=100;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 120 us"); CoreToStub.bucket[counter++].bucketValue=120;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 140 us"); CoreToStub.bucket[counter++].bucketValue=140;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 160 us"); CoreToStub.bucket[counter++].bucketValue=160;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 180 us"); CoreToStub.bucket[counter++].bucketValue=180;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 200 us"); CoreToStub.bucket[counter++].bucketValue=200;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 250 us"); CoreToStub.bucket[counter++].bucketValue=250;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 300 us"); CoreToStub.bucket[counter++].bucketValue=300;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 350 us"); CoreToStub.bucket[counter++].bucketValue=350;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 400 us"); CoreToStub.bucket[counter++].bucketValue=400;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 450 us"); CoreToStub.bucket[counter++].bucketValue=450;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 500 us"); CoreToStub.bucket[counter++].bucketValue=500;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 600 us"); CoreToStub.bucket[counter++].bucketValue=600;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 700 us"); CoreToStub.bucket[counter++].bucketValue=700;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 800 us"); CoreToStub.bucket[counter++].bucketValue=800;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 900 us"); CoreToStub.bucket[counter++].bucketValue=900;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 1 ms"); CoreToStub.bucket[counter++].bucketValue=1000;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 1.2 ms"); CoreToStub.bucket[counter++].bucketValue=1200;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 1.4 ms"); CoreToStub.bucket[counter++].bucketValue=1400;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 1.6 ms"); CoreToStub.bucket[counter++].bucketValue=1600;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 1.8 ms"); CoreToStub.bucket[counter++].bucketValue=1800;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 2 ms"); CoreToStub.bucket[counter++].bucketValue=2000;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 2.5 ms"); CoreToStub.bucket[counter++].bucketValue=2500;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 3 ms"); CoreToStub.bucket[counter++].bucketValue=3000;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 3.5 ms"); CoreToStub.bucket[counter++].bucketValue=3500;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 4 ms"); CoreToStub.bucket[counter++].bucketValue=4000;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 4.5 ms"); CoreToStub.bucket[counter++].bucketValue=4500;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 5 ms"); CoreToStub.bucket[counter++].bucketValue=5000;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 6 ms"); CoreToStub.bucket[counter++].bucketValue=6000;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 7 ms"); CoreToStub.bucket[counter++].bucketValue=7000;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 8 ms"); CoreToStub.bucket[counter++].bucketValue=8000;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 9 ms"); CoreToStub.bucket[counter++].bucketValue=9000;
    sprintf(CoreToStub.bucket[counter].bucketName,"up to 10 ms"); CoreToStub.bucket[counter++].bucketValue=10000;
    sprintf(CoreToStub.bucket[counter].bucketName,"over 10 ms");

    CoreToStub.nBuckets=counter;
    CoreToStub.bucket[CoreToStub.nBuckets].bucketValue=CoreToStub.bucket[CoreToStub.nBuckets-1].bucketValue;
    clearHistogram(&CoreToStub);

    TickToOrder=CoreToStub;
    clearHistogram(&TickToOrder);

    for(int h=0; h <= TEST_TICKTOTRADE_TIME_MESAURES; h++) {
        profilingHistograms[h]=CoreToStub;
        clearHistogram(&profilingHistograms[h]);
    }

    counter=0;
    sprintf(Callback.bucket[counter].bucketName,"up to 1 us"); Callback.bucket[counter++].bucketValue=1;
    sprintf(Callback.bucket[counter].bucketName,"up to 2 us"); Callback.bucket[counter++].bucketValue=2;
    sprintf(Callback.bucket[counter].bucketName,"up to 3 us"); Callback.bucket[counter++].bucketValue=3;
    sprintf(Callback.bucket[counter].bucketName,"up to 4 us"); Callback.bucket[counter++].bucketValue=4;
    sprintf(Callback.bucket[counter].bucketName,"up to 5 us"); Callback.bucket[counter++].bucketValue=5;
    sprintf(Callback.bucket[counter].bucketName,"up to 6 us"); Callback.bucket[counter++].bucketValue=6;
    sprintf(Callback.bucket[counter].bucketName,"up to 7 us"); Callback.bucket[counter++].bucketValue=7;
    sprintf(Callback.bucket[counter].bucketName,"up to 8 us"); Callback.bucket[counter++].bucketValue=8;
    sprintf(Callback.bucket[counter].bucketName,"up to 9 us"); Callback.bucket[counter++].bucketValue=9;
    sprintf(Callback.bucket[counter].bucketName,"up to 10 us"); Callback.bucket[counter++].bucketValue=10;
    sprintf(Callback.bucket[counter].bucketName,"up to 12 us"); Callback.bucket[counter++].bucketValue=12;
    sprintf(Callback.bucket[counter].bucketName,"up to 14 us"); Callback.bucket[counter++].bucketValue=14;
    sprintf(Callback.bucket[counter].bucketName,"up to 16 us"); Callback.bucket[counter++].bucketValue=16;
    sprintf(Callback.bucket[counter].bucketName,"up to 18 us"); Callback.bucket[counter++].bucketValue=18;
    sprintf(Callback.bucket[counter].bucketName,"up to 20 us"); Callback.bucket[counter++].bucketValue=20;
    sprintf(Callback.bucket[counter].bucketName,"up to 25 us"); Callback.bucket[counter++].bucketValue=25;
    sprintf(Callback.bucket[counter].bucketName,"up to 30 us"); Callback.bucket[counter++].bucketValue=30;
    sprintf(Callback.bucket[counter].bucketName,"up to 35 us"); Callback.bucket[counter++].bucketValue=35;
    sprintf(Callback.bucket[counter].bucketName,"up to 40 us"); Callback.bucket[counter++].bucketValue=40;
    sprintf(Callback.bucket[counter].bucketName,"up to 45 us"); Callback.bucket[counter++].bucketValue=45;
    sprintf(Callback.bucket[counter].bucketName,"up to 50 us"); Callback.bucket[counter++].bucketValue=50;
    sprintf(Callback.bucket[counter].bucketName,"up to 60 us"); Callback.bucket[counter++].bucketValue=60;
    sprintf(Callback.bucket[counter].bucketName,"up to 70 us"); Callback.bucket[counter++].bucketValue=70;
    sprintf(Callback.bucket[counter].bucketName,"up to 80 us"); Callback.bucket[counter++].bucketValue=80;
    sprintf(Callback.bucket[counter].bucketName,"up to 90 us"); Callback.bucket[counter++].bucketValue=90;
    sprintf(Callback.bucket[counter].bucketName,"up to 100 us"); Callback.bucket[counter++].bucketValue=100;
    sprintf(Callback.bucket[counter].bucketName,"up to 120 us"); Callback.bucket[counter++].bucketValue=120;
    sprintf(Callback.bucket[counter].bucketName,"up to 140 us"); Callback.bucket[counter++].bucketValue=140;
    sprintf(Callback.bucket[counter].bucketName,"up to 160 us"); Callback.bucket[counter++].bucketValue=160;
    sprintf(Callback.bucket[counter].bucketName,"up to 180 us"); Callback.bucket[counter++].bucketValue=180;
    sprintf(Callback.bucket[counter].bucketName,"up to 200 us"); Callback.bucket[counter++].bucketValue=200;
    sprintf(Callback.bucket[counter].bucketName,"up to 250 us"); Callback.bucket[counter++].bucketValue=250;
    sprintf(Callback.bucket[counter].bucketName,"up to 300 us"); Callback.bucket[counter++].bucketValue=300;
    sprintf(Callback.bucket[counter].bucketName,"up to 350 us"); Callback.bucket[counter++].bucketValue=350;
    sprintf(Callback.bucket[counter].bucketName,"up to 400 us"); Callback.bucket[counter++].bucketValue=400;
    sprintf(Callback.bucket[counter].bucketName,"up to 450 us"); Callback.bucket[counter++].bucketValue=450;
    sprintf(Callback.bucket[counter].bucketName,"up to 500 us"); Callback.bucket[counter++].bucketValue=500;
    sprintf(Callback.bucket[counter].bucketName,"up to 600 us"); Callback.bucket[counter++].bucketValue=600;
    sprintf(Callback.bucket[counter].bucketName,"up to 700 us"); Callback.bucket[counter++].bucketValue=700;
    sprintf(Callback.bucket[counter].bucketName,"up to 800 us"); Callback.bucket[counter++].bucketValue=800;
    sprintf(Callback.bucket[counter].bucketName,"up to 900 us"); Callback.bucket[counter++].bucketValue=900;
    sprintf(Callback.bucket[counter].bucketName,"up to 1 ms"); Callback.bucket[counter++].bucketValue=1000;
    sprintf(Callback.bucket[counter].bucketName,"up to 1.2 ms"); Callback.bucket[counter++].bucketValue=1200;
    sprintf(Callback.bucket[counter].bucketName,"up to 1.4 ms"); Callback.bucket[counter++].bucketValue=1400;
    sprintf(Callback.bucket[counter].bucketName,"up to 1.6 ms"); Callback.bucket[counter++].bucketValue=1600;
    sprintf(Callback.bucket[counter].bucketName,"up to 1.8 ms"); Callback.bucket[counter++].bucketValue=1800;
    sprintf(Callback.bucket[counter].bucketName,"up to 2 ms"); Callback.bucket[counter++].bucketValue=2000;
    sprintf(Callback.bucket[counter].bucketName,"up to 2.5 ms"); Callback.bucket[counter++].bucketValue=2500;
    sprintf(Callback.bucket[counter].bucketName,"up to 3 ms"); Callback.bucket[counter++].bucketValue=3000;
    sprintf(Callback.bucket[counter].bucketName,"up to 3.5 ms"); Callback.bucket[counter++].bucketValue=3500;
    sprintf(Callback.bucket[counter].bucketName,"up to 4 ms"); Callback.bucket[counter++].bucketValue=4000;
    sprintf(Callback.bucket[counter].bucketName,"up to 4.5 ms"); Callback.bucket[counter++].bucketValue=4500;
    sprintf(Callback.bucket[counter].bucketName,"up to 5 ms"); Callback.bucket[counter++].bucketValue=5000;
    sprintf(Callback.bucket[counter].bucketName,"over 5 ms");

    Callback.nBuckets=counter;
    Callback.bucket[Callback.nBuckets].bucketValue=Callback.bucket[Callback.nBuckets-1].bucketValue;
    clearHistogram(&Callback);

}



void buildFixToUsHistograms(sharedMemory_t* memMap) {

    timetype_t FixTimes[MAX_PROFILING_SAMPLES][MAX_NUMBER_TI];
    int64 FixToCoreDelayBias[MAX_NUMBER_TI];
    int64 FixToStubDelayBias[MAX_NUMBER_TI];

    for(int ti=0; ti<MAX_NUMBER_TI; ti++) {
        for(int s=0; s<MAX_PROFILING_SAMPLES; s++) {
            FixTimes[s][ti].sec=0;
            FixTimes[s][ti].usec=0;
        }
        FixToCoreDelayBias[ti]=0;
        FixToStubDelayBias[ti]=0;
    }



    for(int tiIndex=0; tiIndex<memMap->nTradingInterfaces; tiIndex++) {

// First, we clear the histogram data structures

        clearHistogram(&FixToCore[tiIndex]);
        clearHistogram(&FixToStub[tiIndex]);

//    Second, we convert FIX time strings to timetype_t's and we calibrate bias values

        FixToCoreDelayBias[tiIndex] = INT_MAX; // Actually, this should be intialized to the maximum value of int32
        FixToStubDelayBias[tiIndex] = INT_MAX; // Actually, this should be intialized to the maximum value of int32

        int many_samples = (profilingBufferFull[tiIndex] ? n_samples_for_profiling : profilingPtr[tiIndex]);
        if(many_samples > MAX_PROFILING_SAMPLES) many_samples = MAX_PROFILING_SAMPLES; // This should not happen, but ...

        for(int sample = 0; sample < many_samples; sample++) {

            timestruct_t fixtime_str;
            uint32 fixtime_ms;
            int args = sscanf(FixTimesStr[sample][tiIndex], "%4hu%2hu%2hu-%2hu:%2hu:%2hu.%3u",
                                                            &fixtime_str.year,
                                                            &fixtime_str.month,
                                                            &fixtime_str.day,
                                                            &fixtime_str.hour,
                                                            &fixtime_str.min,
                                                            &fixtime_str.sec,
                                                            &fixtime_ms
                             );
            if(args!=7) {
                char coretime[SLEN];
                timestampToDateStr (CoreTimes[sample][tiIndex].sec, coretime);
                char stubtime[SLEN];
                timestampToDateStr (StubTimes[sample][tiIndex].sec, stubtime);
                stubDbg(DEBUG_ERROR_LEVEL,
                        "??? Wrong FIX timestamp (%s), TI is #%d (%s), Core time is %s(%06d), Stub time is %s(%06d)\n",
                        FixTimesStr[sample][tiIndex],
                        tiIndex,
                        memMap->tradingInterfaceName[tiIndex],
                        coretime,
                        CoreTimes[sample][tiIndex].usec,
                        stubtime,
                        StubTimes[sample][tiIndex].usec
                );
                continue;
            }

            dateTotimestamp (&fixtime_str, &(FixTimes[sample][tiIndex].sec));
            FixTimes[sample][tiIndex].usec=1000*fixtime_ms;

            if(my_getTimeDifSigned(&FixTimes[sample][tiIndex], &CoreTimes[sample][tiIndex]) < FixToCoreDelayBias[tiIndex])
                FixToCoreDelayBias[tiIndex]=my_getTimeDifSigned(&FixTimes[sample][tiIndex], &CoreTimes[sample][tiIndex]);

            if(my_getTimeDifSigned(&FixTimes[sample][tiIndex], &StubTimes[sample][tiIndex]) < FixToStubDelayBias[tiIndex])
                FixToStubDelayBias[tiIndex]=my_getTimeDifSigned(&FixTimes[sample][tiIndex], &StubTimes[sample][tiIndex]);

        } //samples


// Third, we build the histograms basics (i.e. just samples per bucket)

        for(int sample = 0; sample < many_samples; sample++) {

            int64 thisCoreDelay=my_getTimeDifSigned(&FixTimes[sample][tiIndex], &CoreTimes[sample][tiIndex]) - FixToCoreDelayBias[tiIndex];

            for(int b=0; b<FixToCore[tiIndex].nBuckets;b++) {
                if(thisCoreDelay<=FixToCore[tiIndex].bucket[b].bucketValue) {
                    FixToCore[tiIndex].bucket[b].samples++;
                    break;
                }
            }
            if(thisCoreDelay>FixToCore[tiIndex].bucket[FixToCore[tiIndex].nBuckets].bucketValue) {
                FixToCore[tiIndex].bucket[FixToCore[tiIndex].nBuckets].samples++;
            }



            int64 thisStubDelay=my_getTimeDifSigned(&FixTimes[sample][tiIndex], &StubTimes[sample][tiIndex]) - FixToStubDelayBias[tiIndex];

            for(int b=0; b<FixToStub[tiIndex].nBuckets;b++) {
                if(thisStubDelay<=FixToStub[tiIndex].bucket[b].bucketValue) {
                    FixToStub[tiIndex].bucket[b].samples++;
                    break;
                }
            }
            if(thisStubDelay>FixToStub[tiIndex].bucket[FixToStub[tiIndex].nBuckets].bucketValue) {
                FixToStub[tiIndex].bucket[FixToStub[tiIndex].nBuckets].samples++;
            }


        } // samples


// And finally, we build the rest of the histograms (i.e. pcts and accumulated samples)

        for(int b=0; (many_samples>0) && (b<FixToCore[tiIndex].nBuckets); b++) {
            FixToCore[tiIndex].bucket[b].pct=((number)FixToCore[tiIndex].bucket[b].samples)/(number)many_samples;
            FixToCore[tiIndex].bucket[b].accSamples+=FixToCore[tiIndex].bucket[b].samples;
            FixToCore[tiIndex].bucket[b].accPct=((number)FixToCore[tiIndex].bucket[b].accSamples)/(number)many_samples;
            FixToCore[tiIndex].bucket[b+1].accSamples=FixToCore[tiIndex].bucket[b].accSamples;
        }
        FixToCore[tiIndex].bucket[FixToCore[tiIndex].nBuckets].accSamples+=FixToCore[tiIndex].bucket[FixToCore[tiIndex].nBuckets].samples;
        FixToCore[tiIndex].bucket[FixToCore[tiIndex].nBuckets].pct=((number)FixToCore[tiIndex].bucket[FixToCore[tiIndex].nBuckets].samples)/(number)many_samples;
        FixToCore[tiIndex].bucket[FixToCore[tiIndex].nBuckets].accPct=((number)FixToCore[tiIndex].bucket[FixToCore[tiIndex].nBuckets].accSamples)/(number)many_samples;

        for(int b=0; (many_samples>0) && (b<FixToStub[tiIndex].nBuckets); b++) {
            FixToStub[tiIndex].bucket[b].pct=((number)FixToStub[tiIndex].bucket[b].samples)/(number)many_samples;
            FixToStub[tiIndex].bucket[b].accSamples+=FixToStub[tiIndex].bucket[b].samples;
            FixToStub[tiIndex].bucket[b].accPct=((number)FixToStub[tiIndex].bucket[b].accSamples)/(number)many_samples;
            FixToStub[tiIndex].bucket[b+1].accSamples=FixToStub[tiIndex].bucket[b].accSamples;
        }
        FixToStub[tiIndex].bucket[FixToStub[tiIndex].nBuckets].accSamples+=FixToStub[tiIndex].bucket[FixToStub[tiIndex].nBuckets].samples;
        FixToStub[tiIndex].bucket[FixToStub[tiIndex].nBuckets].pct=((number)FixToStub[tiIndex].bucket[FixToStub[tiIndex].nBuckets].samples)/(number)many_samples;
        FixToStub[tiIndex].bucket[FixToStub[tiIndex].nBuckets].accPct=((number)FixToStub[tiIndex].bucket[FixToStub[tiIndex].nBuckets].accSamples)/(number)many_samples;

// et voilà!


    } // tiIndex

}




void addCallbackDelay(timetype_t *callbackStart, timetype_t *callbackEnd) {

    callbackDelays[callbackDelaysPtr] = my_getTimeDif(callbackStart, callbackEnd);
    callbackDelaysPtr++;
    if(callbackDelaysPtr >= MAX_PROFILING_SAMPLES) {
        callbackDelaysPtr = 0;
        callbackDelaysBufferFull = true;
    }

}




void buildCoreHistograms(sharedMemory_t* memMap) {

    int32 auIndex = memMap->strAuIndex;

    // First, we clear the histogram data structures

    clearHistogram(&CoreToStub);
    clearHistogram(&TickToOrder);
    clearHistogram(&Callback);

    // Second, we build the CoreToStub histograms basics (i.e. just samples per bucket)

    int total_samples = 0;

    for(int tiIndex=0; tiIndex<memMap->nTradingInterfaces; tiIndex++) {

        int many_samples = (profilingBufferFull[tiIndex] ? n_samples_for_profiling : profilingPtr[tiIndex]);
        if(many_samples > MAX_PROFILING_SAMPLES) many_samples = MAX_PROFILING_SAMPLES; // This should not happen, but ...
        total_samples+=many_samples;

        for(int sample = 0; sample < many_samples; sample++) {

            int64 thisDelay=my_getTimeDifSigned(&CoreTimes[sample][tiIndex], &StubTimes[sample][tiIndex]) ;

            for(int b=0; b<CoreToStub.nBuckets;b++) {
                if(thisDelay<=CoreToStub.bucket[b].bucketValue) {
                    CoreToStub.bucket[b].samples++;
                    break;
                }
            }
            if(thisDelay>CoreToStub.bucket[CoreToStub.nBuckets].bucketValue) {
                CoreToStub.bucket[CoreToStub.nBuckets].samples++;
            }

        }

    }


    // Third, we build the rest of the CoreToStub histogram (i.e. pcts and accumulated samples)

    for(int b=0; (total_samples>0) && (b<CoreToStub.nBuckets); b++) {
        CoreToStub.bucket[b].pct=((number)CoreToStub.bucket[b].samples)/(number)total_samples;
        CoreToStub.bucket[b].accSamples+=CoreToStub.bucket[b].samples;
        CoreToStub.bucket[b].accPct=((number)CoreToStub.bucket[b].accSamples)/(number)total_samples;
        CoreToStub.bucket[b+1].accSamples=CoreToStub.bucket[b].accSamples;
    }
    CoreToStub.bucket[CoreToStub.nBuckets].accSamples+=CoreToStub.bucket[CoreToStub.nBuckets].samples;
    CoreToStub.bucket[CoreToStub.nBuckets].pct=((number)CoreToStub.bucket[CoreToStub.nBuckets].samples)/(number)total_samples;
    CoreToStub.bucket[CoreToStub.nBuckets].accPct=((number)CoreToStub.bucket[CoreToStub.nBuckets].accSamples)/(number)total_samples;



    // Fourth, we build the TickToOrder histogram basics

    boolean bufferFull=false;
    if(memMap->AccountingUnit[auIndex].testTimeIndex < MAX_TEST_MEASURES-1)
        if(memMap->AccountingUnit[auIndex].testTime[memMap->AccountingUnit[auIndex].testTimeIndex+1].timestamp.sec!=0)
            bufferFull=true;

    if(bufferFull && (memMap->AccountingUnit[auIndex].testTimeIndex>=TICK_TO_ORDER_INITIAL_SAMPLES_TO_DISCARD)) initiallyDiscardedOVerwritten=true;

    int many_samples_to_iterate = (bufferFull ? MAX_TEST_MEASURES : memMap->AccountingUnit[auIndex].testTimeIndex + 1);
    int many_samples=0;

    for(int sample = 0; sample < many_samples_to_iterate; sample++) {

        if(!initiallyDiscardedOVerwritten && (sample<TICK_TO_ORDER_INITIAL_SAMPLES_TO_DISCARD)) continue;

        for(int b=0; b<TickToOrder.nBuckets;b++) {
            if(memMap->AccountingUnit[auIndex].testTime[sample].time<=TickToOrder.bucket[b].bucketValue) {
                TickToOrder.bucket[b].samples++;
                break;
            }
        }
        if(memMap->AccountingUnit[auIndex].testTime[sample].time>TickToOrder.bucket[TickToOrder.nBuckets].bucketValue) {
            TickToOrder.bucket[TickToOrder.nBuckets].samples++;
        }

        many_samples++;

    }

    // Fifth, we build the rest of the TickToOrder histogram (i.e. pcts and accumulated samples)

    if(many_samples>0){

        for(int b=0; b<TickToOrder.nBuckets; b++) {
            TickToOrder.bucket[b].pct=((number)TickToOrder.bucket[b].samples)/(number)many_samples;
            TickToOrder.bucket[b].accSamples+=TickToOrder.bucket[b].samples;
            TickToOrder.bucket[b].accPct=((number)TickToOrder.bucket[b].accSamples)/(number)many_samples;
            TickToOrder.bucket[b+1].accSamples=TickToOrder.bucket[b].accSamples;
        }
        TickToOrder.bucket[TickToOrder.nBuckets].accSamples+=TickToOrder.bucket[TickToOrder.nBuckets].samples;
        TickToOrder.bucket[TickToOrder.nBuckets].pct=((number)TickToOrder.bucket[TickToOrder.nBuckets].samples)/(number)many_samples;
        TickToOrder.bucket[TickToOrder.nBuckets].accPct=((number)TickToOrder.bucket[TickToOrder.nBuckets].accSamples)/(number)many_samples;

    }




    // Sixth, we build the total callback histogram basics

    many_samples = (callbackDelaysBufferFull ? MAX_PROFILING_SAMPLES : callbackDelaysPtr);

    for(int sample = 0; sample < many_samples; sample++) {

        for(int b=0; b<Callback.nBuckets;b++) {
            if(callbackDelays[sample]<=Callback.bucket[b].bucketValue) {
                Callback.bucket[b].samples++;
                break;
            }
        }
        if(callbackDelays[sample]>Callback.bucket[Callback.nBuckets].bucketValue) {
            Callback.bucket[Callback.nBuckets].samples++;
        }

    }

    // Seventh, we build the rest of the total callback histogram (i.e. pcts and accumulated samples)

    for(int b=0; (many_samples>0) && (b<Callback.nBuckets); b++) {
        Callback.bucket[b].pct=((number)Callback.bucket[b].samples)/(number)many_samples;
        Callback.bucket[b].accSamples+=Callback.bucket[b].samples;
        Callback.bucket[b].accPct=((number)Callback.bucket[b].accSamples)/(number)many_samples;
        Callback.bucket[b+1].accSamples=Callback.bucket[b].accSamples;
    }
    Callback.bucket[Callback.nBuckets].accSamples+=Callback.bucket[Callback.nBuckets].samples;
    Callback.bucket[Callback.nBuckets].pct=((number)Callback.bucket[Callback.nBuckets].samples)/(number)many_samples;
    Callback.bucket[Callback.nBuckets].accPct=((number)Callback.bucket[Callback.nBuckets].accSamples)/(number)many_samples;

}



void buildProfilingHistograms(sharedMemory_t* memMap) {

    int32 auIndex = memMap->strAuIndex;

    for(int h=0; h<=TEST_TICKTOTRADE_TIME_MESAURES; h++) clearHistogram(&profilingHistograms[h]);

    boolean bufferFull=false;
    if(memMap->AccountingUnit[auIndex].testTimeIndex < MAX_TEST_MEASURES-1)
        if(memMap->AccountingUnit[auIndex].testTime[memMap->AccountingUnit[auIndex].testTimeIndex+1].timestamp.sec!=0)
            bufferFull=true;

    if(bufferFull && (memMap->AccountingUnit[auIndex].testTimeIndex>=TICK_TO_ORDER_INITIAL_SAMPLES_TO_DISCARD)) initiallyDiscardedOVerwritten=true;

    int many_samples_to_iterate = (bufferFull ? MAX_TEST_MEASURES : memMap->AccountingUnit[auIndex].testTimeIndex + 1);
    int many_samples=0;


    for(int sample = 0; sample < many_samples_to_iterate; sample++) {

        if(!initiallyDiscardedOVerwritten && (sample<TICK_TO_ORDER_INITIAL_SAMPLES_TO_DISCARD)) continue;

        for(int b=0; b<profilingHistograms[0].nBuckets;b++) {
            if(memMap->AccountingUnit[auIndex].testTime[sample].time<=profilingHistograms[0].bucket[b].bucketValue) {
                profilingHistograms[0].bucket[b].samples++;
                break;
            }
        }
        if(memMap->AccountingUnit[auIndex].testTime[sample].time>profilingHistograms[0].bucket[profilingHistograms[0].nBuckets].bucketValue) {
            profilingHistograms[0].bucket[profilingHistograms[0].nBuckets].samples++;
        }

        for(int h=1; h< TEST_TICKTOTRADE_TIME_MESAURES; h++) {

            for(int b=0; b<profilingHistograms[h].nBuckets;b++) {
                if(memMap->AccountingUnit[auIndex].testTime[sample].profile[h]-memMap->AccountingUnit[auIndex].testTime[sample].profile[h-1]<=profilingHistograms[h].bucket[b].bucketValue) {
                    profilingHistograms[h].bucket[b].samples++;
                    break;
                }
            }
            if(memMap->AccountingUnit[auIndex].testTime[sample].profile[h]-memMap->AccountingUnit[auIndex].testTime[sample].profile[h-1]>profilingHistograms[h].bucket[profilingHistograms[h].nBuckets].bucketValue) {
                profilingHistograms[h].bucket[profilingHistograms[h].nBuckets].samples++;
            }

        } // Profiling samples array loop

        for(int b=0; b<profilingHistograms[TEST_TICKTOTRADE_TIME_MESAURES].nBuckets;b++) {
            if(memMap->AccountingUnit[auIndex].testTime[sample].time-memMap->AccountingUnit[auIndex].testTime[sample].profile[TEST_TICKTOTRADE_TIME_MESAURES-1]<=profilingHistograms[TEST_TICKTOTRADE_TIME_MESAURES].bucket[b].bucketValue) {
                profilingHistograms[TEST_TICKTOTRADE_TIME_MESAURES].bucket[b].samples++;
                break;
            }
        }
        if(memMap->AccountingUnit[auIndex].testTime[sample].time-memMap->AccountingUnit[auIndex].testTime[sample].profile[TEST_TICKTOTRADE_TIME_MESAURES-1]>profilingHistograms[TEST_TICKTOTRADE_TIME_MESAURES].bucket[profilingHistograms[TEST_TICKTOTRADE_TIME_MESAURES].nBuckets].bucketValue) {
            profilingHistograms[TEST_TICKTOTRADE_TIME_MESAURES].bucket[profilingHistograms[TEST_TICKTOTRADE_TIME_MESAURES].nBuckets].samples++;
        }

        many_samples++;

    }

    if(many_samples>0){

        for(int h=0; h<=TEST_TICKTOTRADE_TIME_MESAURES; h++) {

            for(int b=0; b<profilingHistograms[h].nBuckets; b++) {
                profilingHistograms[h].bucket[b].pct=((number)profilingHistograms[h].bucket[b].samples)/(number)many_samples;
                profilingHistograms[h].bucket[b].accSamples+=profilingHistograms[h].bucket[b].samples;
                profilingHistograms[h].bucket[b].accPct=((number)profilingHistograms[h].bucket[b].accSamples)/(number)many_samples;
                profilingHistograms[h].bucket[b+1].accSamples=profilingHistograms[h].bucket[b].accSamples;
            }
            profilingHistograms[h].bucket[profilingHistograms[h].nBuckets].accSamples+=profilingHistograms[h].bucket[profilingHistograms[h].nBuckets].samples;
            profilingHistograms[h].bucket[profilingHistograms[h].nBuckets].pct=((number)profilingHistograms[h].bucket[profilingHistograms[h].nBuckets].samples)/(number)many_samples;
            profilingHistograms[h].bucket[profilingHistograms[h].nBuckets].accPct=((number)profilingHistograms[h].bucket[profilingHistograms[h].nBuckets].accSamples)/(number)many_samples;

        }

    }

}


uint64 addExecTime(idtype execTimeType, timetype_t *execStart, timetype_t *execEnd, idtype ti) {

    uint64 delay = my_getTimeDif(execStart, execEnd);

    switch(execTimeType) {
        case EXEC_TIME_IMMEDIATE_LIMIT :
            execTimesImmediateLimitDelays[execTimesImmediateLimitDelaysPtr[ti]][ti] = delay;
            execTimesImmediateLimitDelaysPtr[ti]++;
            if(execTimesImmediateLimitDelaysPtr[ti] >= MAX_PROFILING_SAMPLES) {
                execTimesImmediateLimitDelaysPtr[ti] = 0;
                execTimesImmediateLimitDelaysBufferFull[ti] = true;
            }
            break;
        case EXEC_TIME_MARKET :
            execTimesMarketDelays[execTimesMarketDelaysPtr[ti]][ti] = delay;
            execTimesMarketDelaysPtr[ti]++;
            if(execTimesMarketDelaysPtr[ti] >= MAX_PROFILING_SAMPLES) {
                execTimesMarketDelaysPtr[ti] = 0;
                execTimesMarketDelaysBufferFull[ti] = true;
            }
            break;
        case EXEC_TIME_CANCEL :
            execTimesCancelDelays[execTimesCancelDelaysPtr[ti]][ti] = delay;
            execTimesCancelDelaysPtr[ti]++;
            if(execTimesCancelDelaysPtr[ti] >= MAX_PROFILING_SAMPLES) {
                execTimesCancelDelaysPtr[ti] = 0;
                execTimesCancelDelaysBufferFull[ti] = true;
            }
            break;
        case EXEC_TIME_REPLACE :
            execTimesReplaceDelays[execTimesReplaceDelaysPtr[ti]][ti] = delay;
            execTimesReplaceDelaysPtr[ti]++;
            if(execTimesReplaceDelaysPtr[ti] >= MAX_PROFILING_SAMPLES) {
                execTimesReplaceDelaysPtr[ti] = 0;
                execTimesReplaceDelaysBufferFull[ti] = true;
            }
            break;
    }

    return delay;

}



void buildExecTimeHistograms(sharedMemory_t* memMap) {

    for(int tiIndex=0; tiIndex<memMap->nTradingInterfaces; tiIndex++) {

        // First, we clear the histogram data structures
        clearHistogram(&ExecTimesImmediateLimit[tiIndex]);
        clearHistogram(&ExecTimesMarket[tiIndex]);
        clearHistogram(&ExecTimesCancel[tiIndex]);
        clearHistogram(&ExecTimesReplace[tiIndex]);

        // Second, we build the Immediate Limit Execution Times histograms
        // We start with the basics (i.e. just samples per bucket)

        int many_samples = (execTimesImmediateLimitDelaysBufferFull[tiIndex] ? n_samples_for_profiling : execTimesImmediateLimitDelaysPtr[tiIndex]);
        if(many_samples > MAX_PROFILING_SAMPLES) many_samples = MAX_PROFILING_SAMPLES; // This should not happen, but ...

        for(int sample = 0; sample < many_samples; sample++) {

            time_t thisDelay=execTimesImmediateLimitDelays[sample][tiIndex];

            for(int b=0; b<ExecTimesImmediateLimit[tiIndex].nBuckets;b++) {
                if(thisDelay<=ExecTimesImmediateLimit[tiIndex].bucket[b].bucketValue) {
                    ExecTimesImmediateLimit[tiIndex].bucket[b].samples++;
                    break;
                }
            }
            if(thisDelay>ExecTimesImmediateLimit[tiIndex].bucket[ExecTimesImmediateLimit[tiIndex].nBuckets].bucketValue) {
                ExecTimesImmediateLimit[tiIndex].bucket[ExecTimesImmediateLimit[tiIndex].nBuckets].samples++;
            }

        } // samples loop

        // And then we build the rest of the ExecTimesImmediateLimit histogram (i.e. pcts and accumulated samples)

        for(int b=0; (many_samples>0) && (b<ExecTimesImmediateLimit[tiIndex].nBuckets); b++) {
            ExecTimesImmediateLimit[tiIndex].bucket[b].pct=((number)ExecTimesImmediateLimit[tiIndex].bucket[b].samples)/(number)many_samples;
            ExecTimesImmediateLimit[tiIndex].bucket[b].accSamples+=ExecTimesImmediateLimit[tiIndex].bucket[b].samples;
            ExecTimesImmediateLimit[tiIndex].bucket[b].accPct=((number)ExecTimesImmediateLimit[tiIndex].bucket[b].accSamples)/(number)many_samples;
            ExecTimesImmediateLimit[tiIndex].bucket[b+1].accSamples=ExecTimesImmediateLimit[tiIndex].bucket[b].accSamples;
        }
        ExecTimesImmediateLimit[tiIndex].bucket[ExecTimesImmediateLimit[tiIndex].nBuckets].accSamples+=ExecTimesImmediateLimit[tiIndex].bucket[ExecTimesImmediateLimit[tiIndex].nBuckets].samples;
        ExecTimesImmediateLimit[tiIndex].bucket[ExecTimesImmediateLimit[tiIndex].nBuckets].pct=((number)ExecTimesImmediateLimit[tiIndex].bucket[ExecTimesImmediateLimit[tiIndex].nBuckets].samples)/(number)many_samples;
        ExecTimesImmediateLimit[tiIndex].bucket[ExecTimesImmediateLimit[tiIndex].nBuckets].accPct=((number)ExecTimesImmediateLimit[tiIndex].bucket[ExecTimesImmediateLimit[tiIndex].nBuckets].accSamples)/(number)many_samples;



        // Third, we build the Market Execution Times histograms
        // We start with the basics (i.e. just samples per bucket)

        many_samples = (execTimesMarketDelaysBufferFull[tiIndex] ? n_samples_for_profiling : execTimesMarketDelaysPtr[tiIndex]);
        if(many_samples > MAX_PROFILING_SAMPLES) many_samples = MAX_PROFILING_SAMPLES; // This should not happen, but ...

        for(int sample = 0; sample < many_samples; sample++) {

            time_t thisDelay=execTimesMarketDelays[sample][tiIndex];

            for(int b=0; b<ExecTimesMarket[tiIndex].nBuckets;b++) {
                if(thisDelay<=ExecTimesMarket[tiIndex].bucket[b].bucketValue) {
                    ExecTimesMarket[tiIndex].bucket[b].samples++;
                    break;
                }
            }
            if(thisDelay>ExecTimesMarket[tiIndex].bucket[ExecTimesMarket[tiIndex].nBuckets].bucketValue) {
                ExecTimesMarket[tiIndex].bucket[ExecTimesMarket[tiIndex].nBuckets].samples++;
            }

        } // samples loop

        // And then we build the rest of the ExecTimesMarket histogram (i.e. pcts and accumulated samples)

        for(int b=0; (many_samples>0) && (b<ExecTimesMarket[tiIndex].nBuckets); b++) {
            ExecTimesMarket[tiIndex].bucket[b].pct=((number)ExecTimesMarket[tiIndex].bucket[b].samples)/(number)many_samples;
            ExecTimesMarket[tiIndex].bucket[b].accSamples+=ExecTimesMarket[tiIndex].bucket[b].samples;
            ExecTimesMarket[tiIndex].bucket[b].accPct=((number)ExecTimesMarket[tiIndex].bucket[b].accSamples)/(number)many_samples;
            ExecTimesMarket[tiIndex].bucket[b+1].accSamples=ExecTimesMarket[tiIndex].bucket[b].accSamples;
        }
        ExecTimesMarket[tiIndex].bucket[ExecTimesMarket[tiIndex].nBuckets].accSamples+=ExecTimesMarket[tiIndex].bucket[ExecTimesMarket[tiIndex].nBuckets].samples;
        ExecTimesMarket[tiIndex].bucket[ExecTimesMarket[tiIndex].nBuckets].pct=((number)ExecTimesMarket[tiIndex].bucket[ExecTimesMarket[tiIndex].nBuckets].samples)/(number)many_samples;
        ExecTimesMarket[tiIndex].bucket[ExecTimesMarket[tiIndex].nBuckets].accPct=((number)ExecTimesMarket[tiIndex].bucket[ExecTimesMarket[tiIndex].nBuckets].accSamples)/(number)many_samples;



        // Fourth, we build the Cancel Execution Times histograms
        // We start with the basics (i.e. just samples per bucket)

        many_samples = (execTimesCancelDelaysBufferFull[tiIndex] ? n_samples_for_profiling : execTimesCancelDelaysPtr[tiIndex]);
        if(many_samples > MAX_PROFILING_SAMPLES) many_samples = MAX_PROFILING_SAMPLES; // This should not happen, but ...

        for(int sample = 0; sample < many_samples; sample++) {

            time_t thisDelay=execTimesCancelDelays[sample][tiIndex];

            for(int b=0; b<ExecTimesCancel[tiIndex].nBuckets;b++) {
                if(thisDelay<=ExecTimesCancel[tiIndex].bucket[b].bucketValue) {
                    ExecTimesCancel[tiIndex].bucket[b].samples++;
                    break;
                }
            }
            if(thisDelay>ExecTimesCancel[tiIndex].bucket[ExecTimesCancel[tiIndex].nBuckets].bucketValue) {
                ExecTimesCancel[tiIndex].bucket[ExecTimesCancel[tiIndex].nBuckets].samples++;
            }

        } // samples loop

        // And then we build the rest of the ExecTimesCancel histogram (i.e. pcts and accumulated samples)

        for(int b=0; (many_samples>0) && (b<ExecTimesCancel[tiIndex].nBuckets); b++) {
            ExecTimesCancel[tiIndex].bucket[b].pct=((number)ExecTimesCancel[tiIndex].bucket[b].samples)/(number)many_samples;
            ExecTimesCancel[tiIndex].bucket[b].accSamples+=ExecTimesCancel[tiIndex].bucket[b].samples;
            ExecTimesCancel[tiIndex].bucket[b].accPct=((number)ExecTimesCancel[tiIndex].bucket[b].accSamples)/(number)many_samples;
            ExecTimesCancel[tiIndex].bucket[b+1].accSamples=ExecTimesCancel[tiIndex].bucket[b].accSamples;
        }
        ExecTimesCancel[tiIndex].bucket[ExecTimesCancel[tiIndex].nBuckets].accSamples+=ExecTimesCancel[tiIndex].bucket[ExecTimesCancel[tiIndex].nBuckets].samples;
        ExecTimesCancel[tiIndex].bucket[ExecTimesCancel[tiIndex].nBuckets].pct=((number)ExecTimesCancel[tiIndex].bucket[ExecTimesCancel[tiIndex].nBuckets].samples)/(number)many_samples;
        ExecTimesCancel[tiIndex].bucket[ExecTimesCancel[tiIndex].nBuckets].accPct=((number)ExecTimesCancel[tiIndex].bucket[ExecTimesCancel[tiIndex].nBuckets].accSamples)/(number)many_samples;



        // Fifth, we build the Replace Execution Times histograms
        // We start with the basics (i.e. just samples per bucket)

        many_samples = (execTimesReplaceDelaysBufferFull[tiIndex] ? n_samples_for_profiling : execTimesReplaceDelaysPtr[tiIndex]);
        if(many_samples > MAX_PROFILING_SAMPLES) many_samples = MAX_PROFILING_SAMPLES; // This should not happen, but ...

        for(int sample = 0; sample < many_samples; sample++) {

            time_t thisDelay=execTimesReplaceDelays[sample][tiIndex];

            for(int b=0; b<ExecTimesReplace[tiIndex].nBuckets;b++) {
                if(thisDelay<=ExecTimesReplace[tiIndex].bucket[b].bucketValue) {
                    ExecTimesReplace[tiIndex].bucket[b].samples++;
                    break;
                }
            }
            if(thisDelay>ExecTimesReplace[tiIndex].bucket[ExecTimesReplace[tiIndex].nBuckets].bucketValue) {
                ExecTimesReplace[tiIndex].bucket[ExecTimesReplace[tiIndex].nBuckets].samples++;
            }

        } // samples loop

        // And then we build the rest of the ExecTimesReplace histogram (i.e. pcts and accumulated samples)

        for(int b=0; (many_samples>0) && (b<ExecTimesReplace[tiIndex].nBuckets); b++) {
            ExecTimesReplace[tiIndex].bucket[b].pct=((number)ExecTimesReplace[tiIndex].bucket[b].samples)/(number)many_samples;
            ExecTimesReplace[tiIndex].bucket[b].accSamples+=ExecTimesReplace[tiIndex].bucket[b].samples;
            ExecTimesReplace[tiIndex].bucket[b].accPct=((number)ExecTimesReplace[tiIndex].bucket[b].accSamples)/(number)many_samples;
            ExecTimesReplace[tiIndex].bucket[b+1].accSamples=ExecTimesReplace[tiIndex].bucket[b].accSamples;
        }
        ExecTimesReplace[tiIndex].bucket[ExecTimesReplace[tiIndex].nBuckets].accSamples+=ExecTimesReplace[tiIndex].bucket[ExecTimesReplace[tiIndex].nBuckets].samples;
        ExecTimesReplace[tiIndex].bucket[ExecTimesReplace[tiIndex].nBuckets].pct=((number)ExecTimesReplace[tiIndex].bucket[ExecTimesReplace[tiIndex].nBuckets].samples)/(number)many_samples;
        ExecTimesReplace[tiIndex].bucket[ExecTimesReplace[tiIndex].nBuckets].accPct=((number)ExecTimesReplace[tiIndex].bucket[ExecTimesReplace[tiIndex].nBuckets].accSamples)/(number)many_samples;


    } // for ti

}



void save_samples(sharedMemory_t* memMap) {

    int32 auIndex = memMap->strAuIndex;

// First, FixToCore samples

    for(int tiIndex=0; tiIndex<memMap->nTradingInterfaces; tiIndex++) {

        char filename[SLEN];
        sprintf(filename, "LOG/FixToCore_%d_%s.csv", tiIndex, memMap->tradingInterfaceName[tiIndex]);
        FILE *f = fopen(filename, "w");
        if(!f) {
            stubDbg(DEBUG_ERROR_LEVEL, "??? Error in save_samples(): failed to open file (%s)\n", filename);
            return;
        }

        int many_samples = (profilingBufferFull[tiIndex] ? n_samples_for_profiling : profilingPtr[tiIndex]);
        if(many_samples > MAX_PROFILING_SAMPLES) many_samples = MAX_PROFILING_SAMPLES; // This should not happen, but ...

        fprintf(f, "FIX time (Str),FIX ts secs,FIX ts usecs,Core ts secs,Core ts usecs,Stub ts secs,Stub ts usecs, Callback nr\n");

        for(int sample = 0; sample < many_samples; sample++) {

            timestruct_t fixtime_str;
            uint32 fixtime_ms;
            timetype_t fixtime;
            int args = sscanf(FixTimesStr[sample][tiIndex], "%4hu%2hu%2hu-%2hu:%2hu:%2hu.%3u",
                                                            &fixtime_str.year,
                                                            &fixtime_str.month,
                                                            &fixtime_str.day,
                                                            &fixtime_str.hour,
                                                            &fixtime_str.min,
                                                            &fixtime_str.sec,
                                                            &fixtime_ms
                             );
            if(args!=7) {
                fprintf(f, "Wrong FIX timestamp format: %s,NA,NA,", FixTimesStr[sample][tiIndex]);
            } else {
                dateTotimestamp (&fixtime_str, &(fixtime.sec));
                fixtime.usec=1000*fixtime_ms;
                fprintf(f, "%s,%d,%d,",
                           FixTimesStr[sample][tiIndex],
                           fixtime.sec,
                           fixtime.usec
                       );
            }

            fprintf(f, "%d,%d,%d,%d,%d\n",
                       CoreTimes[sample][tiIndex].sec,
                       CoreTimes[sample][tiIndex].usec,
                       StubTimes[sample][tiIndex].sec,
                       StubTimes[sample][tiIndex].usec,
                       callback_numbers[sample][tiIndex]
                   );

        } // for (samples)

        fclose(f);

    } // for (TIs)


// Second, tick-to-order samples

    boolean bufferFull=false;
    if(memMap->AccountingUnit[auIndex].testTimeIndex < MAX_TEST_MEASURES-1)
        if(memMap->AccountingUnit[auIndex].testTime[memMap->AccountingUnit[auIndex].testTimeIndex+1].timestamp.sec!=0)
            bufferFull=true;

    int many_samples = (bufferFull ? MAX_TEST_MEASURES : memMap->AccountingUnit[auIndex].testTimeIndex + 1);

    char filename[SLEN];
    sprintf(filename, "LOG/TickToOrder.csv");
    FILE *f = fopen(filename, "w");
    if(!f) {
        stubDbg(DEBUG_ERROR_LEVEL, "??? Error in save_samples(): failed to open file (%s)\n", filename);
        return;
    }

    fprintf(f, "Time str, Timestamp, total latency\n");

    for(int sample = 0; sample < many_samples; sample++) {

        timestruct_t timeStr;
        timestampToDate (memMap->AccountingUnit[auIndex].testTime[sample].timestamp.sec, &timeStr);
        fprintf(f, "%04d%02d%02d-%02d:%02d:%02d.%03d(%06d),%d,%d\n",
                   timeStr.year,
                   timeStr.month,
                   timeStr.day,
                   timeStr.hour,
                   timeStr.min,
                   timeStr.sec,
                   memMap->AccountingUnit[auIndex].testTime[sample].timestamp.usec/1000,
                   memMap->AccountingUnit[auIndex].testTime[sample].timestamp.usec,
                   memMap->AccountingUnit[auIndex].testTime[sample].timestamp.sec,
                   memMap->AccountingUnit[auIndex].testTime[sample].time
               );


    }

    fclose(f);


}



void saveAllIndicators(sharedMemory_t* memMap) {

    for(idtype pbIndex=0; pbIndex<memMap->nPrimeBrokers; pbIndex++) {

        tradeIndicators_t todayInd;
        buildTodayNonSavedTradeIndicatorsForPB(memMap, pbIndex, &todayInd);

        indicators[TIMEFRAME_TODAY][pbIndex].totalTrades+=todayInd.totalTrades;
        indicators[TIMEFRAME_TODAY][pbIndex].fullyFilledTrades+=todayInd.fullyFilledTrades;
        indicators[TIMEFRAME_TODAY][pbIndex].partiallyFilledTrades+=todayInd.partiallyFilledTrades;
        indicators[TIMEFRAME_TODAY][pbIndex].failedAndRejectedTrades+=todayInd.failedAndRejectedTrades;
        indicators[TIMEFRAME_TODAY][pbIndex].userCanceledTrades+=todayInd.userCanceledTrades;
        indicators[TIMEFRAME_TODAY][pbIndex].replacesDone+=todayInd.replacesDone;
        indicators[TIMEFRAME_TODAY][pbIndex].amountTraded+=todayInd.amountTraded;

        for(idtype tf=0; tf<NUM_TIMEFRAMES; tf++) {
/*
            indicators[tf][pbIndex].totalTrades += todayInd.totalTrades;
            indicators[tf][pbIndex].fullyFilledTrades += todayInd.fullyFilledTrades;
            indicators[tf][pbIndex].partiallyFilledTrades += todayInd.partiallyFilledTrades;
            indicators[tf][pbIndex].failedAndRejectedTrades += todayInd.failedAndRejectedTrades;
            indicators[tf][pbIndex].userCanceledTrades += todayInd.userCanceledTrades;
            indicators[tf][pbIndex].replacesDone += todayInd.replacesDone;
            indicators[tf][pbIndex].amountTraded += todayInd.amountTraded;
*/
            writeIndicators(pbIndex, tf, &(indicators[tf][pbIndex]), equityValue[tf][pbIndex]);

        } // for timeframes

    } // for pbs

}



boolean renderIndicators(sharedMemory_t* memMap, boolean allPBs, idtype PBindexIfNotForAll, uint *manyIndicators,
                                  char indicatorNames[MAX_INDICATORS][NUM_TIMEFRAMES][SLEN], char indicatorValues[MAX_INDICATORS][NUM_TIMEFRAMES][SLEN]) {

    int32 auIndex = memMap->strAuIndex;
    int32 epIndex = memMap->strEpIndex;

    if(!allPBs && ((PBindexIfNotForAll<0) || (PBindexIfNotForAll>=memMap->nPrimeBrokers))) return false;

    tradeIndicators_t todayNonSavedIndicators;
    if(allPBs)
        buildTodayNonSavedTradeIndicatorsForAllPBs(memMap, &todayNonSavedIndicators);
    else
        buildTodayNonSavedTradeIndicatorsForPB(memMap, PBindexIfNotForAll, &todayNonSavedIndicators);

    char amount[NLEN];
    char pct[NLEN];

    for(int tf=0; tf < NUM_TIMEFRAMES; tf++) {

        tradeIndicators_t indicatorsRec;
        int32 initialEquityVal;
        if(allPBs) {
            indicatorsRec.totalTrades=todayNonSavedIndicators.totalTrades;
            indicatorsRec.fullyFilledTrades=todayNonSavedIndicators.fullyFilledTrades;
            indicatorsRec.partiallyFilledTrades=todayNonSavedIndicators.partiallyFilledTrades;
            indicatorsRec.failedAndRejectedTrades=todayNonSavedIndicators.failedAndRejectedTrades;
            indicatorsRec.userCanceledTrades=todayNonSavedIndicators.userCanceledTrades;
            indicatorsRec.replacesDone=todayNonSavedIndicators.replacesDone;
            indicatorsRec.amountTraded=todayNonSavedIndicators.amountTraded;
            initialEquityVal=0;
            for(int pbIndex=0; pbIndex<memMap->nPrimeBrokers; pbIndex++) {
                indicatorsRec.totalTrades+=indicators[tf][pbIndex].totalTrades;
                indicatorsRec.fullyFilledTrades+=indicators[tf][pbIndex].fullyFilledTrades;
                indicatorsRec.partiallyFilledTrades+=indicators[tf][pbIndex].partiallyFilledTrades;
                indicatorsRec.failedAndRejectedTrades+=indicators[tf][pbIndex].failedAndRejectedTrades;
                indicatorsRec.userCanceledTrades+=indicators[tf][pbIndex].userCanceledTrades;
                indicatorsRec.replacesDone+=indicators[tf][pbIndex].replacesDone;
                indicatorsRec.amountTraded+=indicators[tf][pbIndex].amountTraded;
                initialEquityVal+=equityValue[tf][pbIndex];
            }
        } else {
            indicatorsRec.totalTrades=indicators[tf][PBindexIfNotForAll].totalTrades + todayNonSavedIndicators.totalTrades;
            indicatorsRec.fullyFilledTrades=indicators[tf][PBindexIfNotForAll].fullyFilledTrades + todayNonSavedIndicators.fullyFilledTrades;
            indicatorsRec.partiallyFilledTrades=indicators[tf][PBindexIfNotForAll].partiallyFilledTrades + todayNonSavedIndicators.partiallyFilledTrades;
            indicatorsRec.failedAndRejectedTrades=indicators[tf][PBindexIfNotForAll].failedAndRejectedTrades + todayNonSavedIndicators.failedAndRejectedTrades;
            indicatorsRec.userCanceledTrades=indicators[tf][PBindexIfNotForAll].userCanceledTrades + todayNonSavedIndicators.userCanceledTrades;
            indicatorsRec.replacesDone=indicators[tf][PBindexIfNotForAll].replacesDone + todayNonSavedIndicators.replacesDone;
            indicatorsRec.amountTraded=indicators[tf][PBindexIfNotForAll].amountTraded + todayNonSavedIndicators.amountTraded;
            initialEquityVal=equityValue[tf][PBindexIfNotForAll];
        }

        uint indicators = 0;

        sprintf(indicatorNames[indicators][tf], "Total trades");
        renderWithDots(amount, indicatorsRec.totalTrades, false);
        sprintf(indicatorValues[indicators][tf], "%s", amount);
        indicators++;

        sprintf(indicatorNames[indicators][tf], "Fully filled trades");
        renderWithDots(amount, indicatorsRec.totalTrades, false);
        if(indicatorsRec.totalTrades <= 0)
            sprintf(indicatorValues[indicators][tf], "%s", amount);
        else
            sprintf(indicatorValues[indicators][tf], "%s (%.1f%%)", amount, (100.0 * (number) indicatorsRec.fullyFilledTrades) / (number) indicatorsRec.totalTrades);
        indicators++;

        sprintf(indicatorNames[indicators][tf], "Partially filled trades");
        renderWithDots(amount, indicatorsRec.partiallyFilledTrades, false);
        if(indicatorsRec.totalTrades <= 0)
            sprintf(indicatorValues[indicators][tf], "%s", amount);
        else
            sprintf(indicatorValues[indicators][tf], "%s (%.1f%%)", amount, (100.0 * (number) indicatorsRec.partiallyFilledTrades) / (number) indicatorsRec.totalTrades);
        indicators++;

        sprintf(indicatorNames[indicators][tf], "Failed and rejected trades");
        renderWithDots(amount, indicatorsRec.failedAndRejectedTrades, false);
        if(indicatorsRec.totalTrades <= 0)
            sprintf(indicatorValues[indicators][tf], "%s", amount);
        else
            sprintf(indicatorValues[indicators][tf], "%s (%.1f%%)", amount, (100.0 * (number) indicatorsRec.failedAndRejectedTrades) / (number) indicatorsRec.totalTrades);
        indicators++;

        sprintf(indicatorNames[indicators][tf], "User canceled trades");
        renderWithDots(amount, indicatorsRec.userCanceledTrades, false);
        if(indicatorsRec.totalTrades <= 0)
            sprintf(indicatorValues[indicators][tf], "%s", amount);
        else
            sprintf(indicatorValues[indicators][tf], "%s (%.1f%%)", amount, (100.0 * (number) indicatorsRec.userCanceledTrades) / (number) indicatorsRec.totalTrades);
        indicators++;

        sprintf(indicatorNames[indicators][tf], "Replaced orders");
        renderWithDots(amount, indicatorsRec.replacesDone, false);
        sprintf(indicatorValues[indicators][tf], "%s", amount);
        indicators++;

        sprintf(indicatorNames[indicators][tf], "Amount traded");
        renderWithDots(amount, indicatorsRec.amountTraded, false);
        sprintf(indicatorValues[indicators][tf], "%s", amount);
        indicators++;

        int32 total_margin = 0;
        int32 total_free_margin = 0;
        int32 total_equity_EP = 0;
        int32 total_equity_STR = 0;
        if(allPBs) {
            for(idtype i = 0; i < memMap->nPrimeBrokers; i++) {
                total_margin += memMap->AccountingUnit[auIndex].UsedMargin_STR[i];
                total_free_margin += memMap->AccountingUnit[auIndex].FreeMargin_STR[i];
                total_equity_EP += memMap->EquityPool[epIndex].Equity_EP[i];
                total_equity_STR += memMap->AccountingUnit[auIndex].Equity_STR[i];
            }
        }

        sprintf(indicatorNames[indicators][tf], "Used margin (and %% vs EP)");
        if(allPBs) {
            renderWithDots(amount, total_margin, false);
            renderPct(pct, total_margin, total_equity_EP, false);
            sprintf(indicatorValues[indicators][tf], "%s (%s)", amount, pct);
        } else {
            renderWithDots(amount, memMap->AccountingUnit[auIndex].UsedMargin_STR[PBindexIfNotForAll], false);
            renderPct(pct, memMap->AccountingUnit[auIndex].UsedMargin_STR[PBindexIfNotForAll], memMap->EquityPool[epIndex].Equity_EP[PBindexIfNotForAll], false);
            sprintf(indicatorValues[indicators][tf], "%s (%s)", amount, pct);
        }
        indicators++;

        sprintf(indicatorNames[indicators][tf], "Margin still available (and %% vs EP)");
        if(allPBs) {
            renderWithDots(amount, total_free_margin, false);
            renderPct(pct, total_free_margin, total_equity_EP, false);
            sprintf(indicatorValues[indicators][tf], "%s (%s)", amount, pct);
        } else {
            renderWithDots(amount, memMap->AccountingUnit[auIndex].FreeMargin_STR[PBindexIfNotForAll], false);
            renderPct(pct, memMap->AccountingUnit[auIndex].FreeMargin_STR[PBindexIfNotForAll], memMap->EquityPool[epIndex].Equity_EP[PBindexIfNotForAll], false);
            sprintf(indicatorValues[indicators][tf], "%s (%s)", amount, pct);
        }
        indicators++;

        sprintf(indicatorNames[indicators][tf], "P&L");
        if(allPBs)
            renderWithDots(amount, total_equity_STR - initialEquityVal, true);
        else
            renderWithDots(amount, memMap->AccountingUnit[auIndex].Equity_STR[PBindexIfNotForAll] - initialEquityVal, true);
        sprintf(indicatorValues[indicators][tf], "%s", amount);
        indicators++;

        (*manyIndicators) = indicators;

    } // loop timeframes


    return true;

}



/************************************************************************************/
/* Market watch utilities                                                           */


void renderFB(
        // These are inputs:
        sharedMemory_t*  memMap,                                                        // memMap as usual
        idtype           sec,                                                           // Security to use
        boolean          renderOwnQuotes,                                               // Own quotes if true, full book quotes if false

        // And these are outputs:
        // Note: rows are not sorted by price!
        uint             *nColumns,                                                     // Number of columns for the table (# of TIs * 2 + 3)
        uint             *nRows,                                                        // Number of rows for the table
        uint32           prices[MAX_FB_RENDER_ROWS],                                    // Prices corresponding to each row (rows are not sorted!)
        uint32           *midPrice,                                                     // Price corresponding to the middle of the table (will be set to system price for this sec)
        char             headers[MAX_FB_RENDER_COLUMNS][NLEN],                          // Headers to display on top of the table
        char             toDisplay[MAX_FB_RENDER_ROWS][MAX_FB_RENDER_COLUMNS][NLEN],    // Message to display in each node
        idtype           colorToDisplay[MAX_FB_RENDER_ROWS][MAX_FB_RENDER_COLUMNS],     // Color code to display each node (codes defined above)
        char             expand[MAX_FB_RENDER_ROWS][MAX_FB_RENDER_COLUMNS][SLEN],       // Message to display when node is to be expanded (i.e. list of quotes)
        uint32           totalLiquidity[MAX_FB_RENDER_ROWS][MAX_FB_RENDER_COLUMNS],     // Sum of liquidity of all quotes in this price level
        uint             manyQuotes[MAX_FB_RENDER_ROWS][MAX_FB_RENDER_COLUMNS],         // Number of quotes in this price level
        boolean          *tooManyRows

) {

    int32 auIndex = memMap->strAuIndex;

    if(memMap->mapFBNumberofsortingBid[sec]+memMap->mapFBNumberofsortingAsk[sec]<=0) {
        *nColumns=0;
        *nRows=0;
        return;
    }

    uint nCol=0;

    for(int ti=memMap->nTradingInterfaces-1; ti>=0; ti--) {
        sprintf(headers[nCol], "%s", memMap->tradingInterfaceName[ti]);
        nCol++;
    }

    sprintf(headers[nCol], "Own BUYs");
    nCol++;
    sprintf(headers[nCol], "Prices");
    nCol++;
    sprintf(headers[nCol], "Own SELLs");
    nCol++;

    for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
        sprintf(headers[nCol], "%s", memMap->tradingInterfaceName[ti]);
        nCol++;
    }

    *nColumns = nCol; // This would be 2 * nTradingInterfaces + 3

    *midPrice = memMap->mapBookSp[sec].price;




    int nRow = -1;
    *tooManyRows = false;

    if(!renderOwnQuotes) {

        for(int r=0; r<MAX_FB_RENDER_ROWS; r++) {
            for(int c=0; c<*nColumns; c++) {
                manyQuotes[r][c]=0;
                totalLiquidity[r][c]=0;
            }
        }

        int overallToBbidRow = -1;
        int overallToBaskRow = -1;

        {
            // Ask quotes:

            for(int q=0; q<memMap->mapFBNumberofsortingAsk[sec]; q++) {
                uint16 thisQuoteIndex = memMap->mapFBSortingAsk[sec][q].quoteIndex;
                uint32 thisQuotePrice = memMap->mapFBSortingAsk[sec][q].price;
                uint32 thisTIindex = memMap->mapFBSortingAsk[sec][q].tiIndex;
                uint32 thisQuoteLiquidity = memMap->mapFBQuotesAsk[thisTIindex][sec][thisQuoteIndex].liquidity;
                uint32 thisQuoteReplaces = memMap->mapFBQuotesAsk[thisTIindex][sec][thisQuoteIndex].replaces;
                uint thisCol = memMap->nTradingInterfaces + 3 + thisTIindex;

                boolean rowFound = false;
                int whichRow;
                for(int i=0; i<=nRow; i++) {
                    if(prices[i]==thisQuotePrice) {
                        rowFound = true;
                        whichRow=i;
                        break;
                    }
                }
                if(!rowFound) {
                    nRow++;
                    whichRow=nRow;
                    for(int c=0; c<MAX_FB_RENDER_COLUMNS; c++) {
                        strcpy(toDisplay[whichRow][c], "");
                        if(c<memMap->nTradingInterfaces)
                            colorToDisplay[whichRow][c]=FB_RENDER_COLOR_EMPTY_BID;
                        else if((c==memMap->nTradingInterfaces) || (c==memMap->nTradingInterfaces+2))
                            colorToDisplay[whichRow][c]=FB_RENDER_COLOR_EMPTY;
                        else if(c==memMap->nTradingInterfaces+1)
                            colorToDisplay[whichRow][c]=FB_RENDER_COLOR_PRICE;
                        else if(c>memMap->nTradingInterfaces+2) // redundant
                            colorToDisplay[whichRow][c]=FB_RENDER_COLOR_EMPTY_ASK;
                        strcpy(expand[whichRow][c], "");
                    }
                    if(nRow>=MAX_FB_RENDER_ROWS) {nRow=MAX_FB_RENDER_ROWS-1; *tooManyRows=true; break;} // Row array is full!!
                    prices[nRow]=thisQuotePrice;
                }

                manyQuotes[whichRow][thisCol]++;
                totalLiquidity[whichRow][thisCol]+=thisQuoteLiquidity;
                char thisAm[NLEN];
                renderWithDots(thisAm, thisQuoteLiquidity, false);
                char thisLine[SLEN];
                sprintf(thisLine, "%s (%d replaces, liq provider is %s)\n",
                        thisAm, thisQuoteReplaces, memMap->mapFBQuotesProvidersAsk[thisTIindex][sec][thisQuoteIndex]);
                if(manyQuotes[whichRow][thisCol] == 1) {
                    strcpy(expand[whichRow][thisCol], thisLine);
                } else {
                    strcat(expand[whichRow][thisCol], thisLine);
                }
                if(thisQuotePrice == memMap->mapTiTbAsk[thisTIindex][sec].price) {
                    colorToDisplay[whichRow][thisCol]=FB_RENDER_COLOR_TI_TOB_ASK;
                } else {
                    colorToDisplay[whichRow][thisCol]=FB_RENDER_COLOR_QUOTE_ASK;
                }
                if(q==0) overallToBaskRow=whichRow; // This is to later mark all ask cells in this row as FB_RENDER_COLOR_OVERALL_TOB_ASK

            } // loop (quotes)

        } // ask quotes

        {
            // Bid quotes:

            for(int q=0; q<memMap->mapFBNumberofsortingBid[sec]; q++) {

                uint16 thisQuoteIndex = memMap->mapFBSortingBid[sec][q].quoteIndex;
                uint32 thisQuotePrice = memMap->mapFBSortingBid[sec][q].price;
                uint32 thisTIindex = memMap->mapFBSortingBid[sec][q].tiIndex;
                uint32 thisQuoteLiquidity = memMap->mapFBQuotesBid[thisTIindex][sec][thisQuoteIndex].liquidity;
                uint32 thisQuoteReplaces = memMap->mapFBQuotesBid[thisTIindex][sec][thisQuoteIndex].replaces;
                uint thisCol = memMap->nTradingInterfaces - thisTIindex - 1;

                boolean rowFound = false;
                int whichRow;
                for(int i=0; i<=nRow; i++) {
                    if(prices[i]==thisQuotePrice) {
                        rowFound = true;
                        whichRow=i;
                        break;
                    }
                }
                if(!rowFound) {
                    nRow++;
                    whichRow=nRow;
                    for(int c=0; c<MAX_FB_RENDER_COLUMNS; c++) {
                        strcpy(toDisplay[whichRow][c], "");
                        if(c<memMap->nTradingInterfaces)
                            colorToDisplay[whichRow][c]=FB_RENDER_COLOR_EMPTY_BID;
                        else if((c==memMap->nTradingInterfaces) || (c==memMap->nTradingInterfaces+2))
                            colorToDisplay[whichRow][c]=FB_RENDER_COLOR_EMPTY;
                        else if(c==memMap->nTradingInterfaces+1)
                            colorToDisplay[whichRow][c]=FB_RENDER_COLOR_PRICE;
                        else if(c>memMap->nTradingInterfaces+2) // redundant
                            colorToDisplay[whichRow][c]=FB_RENDER_COLOR_EMPTY_ASK;
                        strcpy(expand[whichRow][c], "");
                    }
                    if(nRow>=MAX_FB_RENDER_ROWS) {nRow=MAX_FB_RENDER_ROWS-1; *tooManyRows=true; break;} // Row array is full!!
                    prices[nRow]=thisQuotePrice;
                }

                manyQuotes[whichRow][thisCol]++;
                totalLiquidity[whichRow][thisCol]+=thisQuoteLiquidity;
                char thisAm[NLEN];
                renderWithDots(thisAm, thisQuoteLiquidity, false);
                char thisLine[SLEN];
                sprintf(thisLine, "%s (%d replaces, liq provider is %s)\n",
                        thisAm, thisQuoteReplaces, memMap->mapFBQuotesProvidersBid[thisTIindex][sec][thisQuoteIndex]);
                if(manyQuotes[whichRow][thisCol] == 1) {
                    strcpy(expand[whichRow][thisCol], thisLine);
                } else {
                    strcat(expand[whichRow][thisCol], thisLine);
                }
                if(thisQuotePrice == memMap->mapTiTbBid[thisTIindex][sec].price) {
                    colorToDisplay[whichRow][thisCol]=FB_RENDER_COLOR_TI_TOB_BID;
                } else {
                    colorToDisplay[whichRow][thisCol]=FB_RENDER_COLOR_QUOTE_BID;
                }
                if(q==0) overallToBbidRow=whichRow; // This is to later mark all bid cells in this row as FB_RENDER_COLOR_OVERALL_TOB_BID

            } // loop (quotes)

        } // bid quotes

        for(int r=0; r<=nRow; r++) {

            for(int c=0; c<*nColumns; c++) {

                if(manyQuotes[r][c] > 0) {

                    char amS[NLEN];
                    renderWithDots(amS, totalLiquidity[r][c], false);
                    sprintf(toDisplay[r][c], "%s [%d]", amS, manyQuotes[r][c]);

                }

            } // Loop (columns)

        } // Loop (rows so far)

        for(int c=0; c<memMap->nTradingInterfaces; c++) {
            if(overallToBbidRow >= 0) {
                colorToDisplay[overallToBbidRow][c]=FB_RENDER_COLOR_OVERALL_TOB_BID;
            }
        }

        for(int c=memMap->nTradingInterfaces + 3; c < 3 + 2 * memMap->nTradingInterfaces; c++) {
            if(overallToBaskRow >= 0) {
                colorToDisplay[overallToBaskRow][c]=FB_RENDER_COLOR_OVERALL_TOB_ASK;
            }
        }

    } // if !renderOwnQuotes

    else {
        // Now to consider own orders

        for(int r=0; r<MAX_FB_RENDER_ROWS; r++) {
            manyQuotes[r][memMap->nTradingInterfaces]=0;
            totalLiquidity[r][memMap->nTradingInterfaces]=0;
            manyQuotes[r][memMap->nTradingInterfaces+2]=0;
            totalLiquidity[r][memMap->nTradingInterfaces+2]=0;
        }

        uint32 tradesProcessed=0;
        for(uint32 t=0; (tradesProcessed<memMap->AccountingUnit[auIndex].numberOfAlives) && (t<MAX_TRADES_LIST_ALIVE); t++) {
            if(!memMap->AccountingUnit[auIndex].alive[t].alive) continue;
            tradesProcessed++;

            tradeElement_t *thisTrade=&(memMap->AccountingUnit[auIndex].alive[t]);
            tradeCommand_t *thisTradeParams=&(thisTrade->params);

            if(
                    (thisTradeParams->tradeType == TRADE_TYPE_LIMIT) &&
                    (thisTradeParams->security == sec)
              ) {

                uint32 thisPrice=thisTradeParams->limitPrice;

                boolean rowFound = false;
                int whichRow;
                for(int i=0; i<=nRow; i++) {
                    if(prices[i]==thisPrice) {
                        rowFound = true;
                        whichRow=i;
                        break;
                    }
                }
                if(!rowFound) {
                    nRow++;
                    whichRow=nRow;
                    for(int c=0; c<MAX_FB_RENDER_COLUMNS; c++) {
                        strcpy(toDisplay[whichRow][c], "");
                        if(c<memMap->nTradingInterfaces)
                            colorToDisplay[whichRow][c]=FB_RENDER_COLOR_EMPTY_BID;
                        else if((c==memMap->nTradingInterfaces) || (c==memMap->nTradingInterfaces+2))
                            colorToDisplay[whichRow][c]=FB_RENDER_COLOR_EMPTY;
                        else if(c==memMap->nTradingInterfaces+1)
                            colorToDisplay[whichRow][c]=FB_RENDER_COLOR_PRICE;
                        else if(c>memMap->nTradingInterfaces+2) // redundant
                            colorToDisplay[whichRow][c]=FB_RENDER_COLOR_EMPTY_ASK;
                        strcpy(expand[whichRow][c], "");
                    }
                    if(nRow>=MAX_FB_RENDER_ROWS) {nRow=MAX_FB_RENDER_ROWS-1; *tooManyRows=true; break;} // Row array is full!!
                    prices[nRow]=thisPrice;
                }

                if(thisTradeParams->side == TRADE_SIDE_SELL) {
                    manyQuotes[whichRow][memMap->nTradingInterfaces+2]++;
                    totalLiquidity[whichRow][memMap->nTradingInterfaces+2]+=thisTradeParams->quantity;
                    char thisAm[NLEN];
                    renderWithDots(thisAm, thisTradeParams->quantity, false);
                    char thisLine[SLEN];
                    sprintf(thisLine, "LIMIT SELL order, %s\n", thisAm);
                    if(manyQuotes[whichRow][memMap->nTradingInterfaces+2] == 1) {
                        strcpy(expand[whichRow][memMap->nTradingInterfaces+2], thisLine);
                    } else {
                        strcat(expand[whichRow][memMap->nTradingInterfaces+2], thisLine);
                    }
                    colorToDisplay[whichRow][memMap->nTradingInterfaces+2]=FB_RENDER_COLOR_OWN_SELL;
                } else {
                    manyQuotes[whichRow][memMap->nTradingInterfaces]++;
                    totalLiquidity[whichRow][memMap->nTradingInterfaces]+=thisTradeParams->quantity;
                    char thisAm[NLEN];
                    renderWithDots(thisAm, thisTradeParams->quantity, false);
                    char thisLine[SLEN];
                    sprintf(thisLine, "LIMIT BUY order, %s\n", thisAm);
                    if(manyQuotes[whichRow][memMap->nTradingInterfaces] == 1) {
                        strcpy(expand[whichRow][memMap->nTradingInterfaces], thisLine);
                    } else {
                        strcat(expand[whichRow][memMap->nTradingInterfaces], thisLine);
                    }
                    colorToDisplay[whichRow][memMap->nTradingInterfaces]=FB_RENDER_COLOR_OWN_BUY;
                } // SELL or BUY

            } // Type LIMIT and the appropriate security

        } // loop (trades alive)

        for(int r=0; r<=nRow; r++) {

            if(manyQuotes[r][memMap->nTradingInterfaces] > 0) {

                char amS[NLEN];
                renderWithDots(amS, totalLiquidity[r][memMap->nTradingInterfaces], false);
                sprintf(toDisplay[r][memMap->nTradingInterfaces], "%s [%d]", amS, manyQuotes[r][memMap->nTradingInterfaces]);

            }

            renderPrice(toDisplay[r][memMap->nTradingInterfaces+1], prices[r], getSecurityDecimals(sec));

            if(manyQuotes[r][memMap->nTradingInterfaces+2] > 0) {

                char amS[NLEN];
                renderWithDots(amS, totalLiquidity[r][memMap->nTradingInterfaces+2], false);
                sprintf(toDisplay[r][memMap->nTradingInterfaces+2], "%s [%d]", amS, manyQuotes[r][memMap->nTradingInterfaces+2]);

            }

        } // Loop (rows so far)


    } // if renderOwnOrders

    *nRows=nRow+1;

}



void ToBComparison(
        // These are inputs:
        sharedMemory_t*  memMap,                                                      // memMap as usual
        idtype           sec,                                                         // Security to use

        // And these are outputs:
        uint             *nColumns,                                                   // This will be set to memMap->nTradingInterfaces

        uint             manyQuotesBid[MAX_NUMBER_TI],                                // Number of quotes to display, including ToB (0 if no prices available)
        uint32           bidQuotesPrices[MAX_NUMBER_TI][MAX_QUOTES],                  // Prices values
                                                                                      // column within 0 ..*manyQuotesBid - 1
                                                                                      // bidQuotesPrices[col][0] is ToB if manyQuotesBid>0
        char             ToBbidNames[MAX_NUMBER_TI][NLEN],                            // TI names for ToB bids

        uint             manyQuotesAsk[MAX_NUMBER_TI],                                // Number of quotes to display, including ToB (0 if no prices available)
        uint32           askQuotesPrices[MAX_NUMBER_TI][MAX_QUOTES],                  // Prices values
                                                                                      // column within 0 ..*manyQuotesAsk - 1
                                                                                      // askQuotesPrices[col][0] is ToB if manyQuotesBid>0
        char             ToBaskNames[MAX_NUMBER_TI][NLEN]                             // TI names for ToB asks

) {

    for(int col=0; col < memMap->nTradingInterfaces; col++) {
        manyQuotesBid[col]=0;
        manyQuotesAsk[col]=0;
    }

    *nColumns = 0;
    if( (memMap->mapFBNumberofsortingBid[sec]>0) || (memMap->mapFBNumberofsortingAsk[sec]>0) )
        *nColumns = memMap->nTradingInterfaces;
    else
        return;

    {
        // Bids:
        boolean ToBbidsSet[MAX_NUMBER_TI] = {false};
        uint manyBidsSet=0;
        uint whichColumnIsThisTIBid[MAX_NUMBER_TI];

        for(int q=0; q<memMap->mapFBNumberofsortingBid[sec]; q++) {
            uint32 thisPrice = memMap->mapFBSortingBid[sec][q].price;
            idtype thisTIindex = memMap->mapFBSortingBid[sec][q].tiIndex;
            if(!ToBbidsSet[thisTIindex]) {
                uint thisColumn = manyBidsSet;
                manyQuotesBid[thisColumn]=1;
                bidQuotesPrices[thisColumn][0]=thisPrice;
                sprintf(ToBbidNames[thisColumn], "%s", memMap->tradingInterfaceName[thisTIindex]);
                whichColumnIsThisTIBid[thisTIindex] = thisColumn;
                ToBbidsSet[thisTIindex]=true;
                manyBidsSet++;
            } else {
                uint thisColumn = whichColumnIsThisTIBid[thisTIindex];
                bidQuotesPrices[thisColumn][manyQuotesBid[thisColumn]]=thisPrice;
                manyQuotesBid[thisColumn]++;
            }
        } // Loop: bid quotes
    }


    {
        // Asks:
        boolean ToBasksSet[MAX_NUMBER_TI] = {false};
        uint manyAsksSet=0;
        uint whichColumnIsThisTIAsk[MAX_NUMBER_TI];

        for(int q=0; q<memMap->mapFBNumberofsortingAsk[sec]; q++) {
            uint32 thisPrice = memMap->mapFBSortingAsk[sec][q].price;
            idtype thisTIindex = memMap->mapFBSortingAsk[sec][q].tiIndex;
            if(!ToBasksSet[thisTIindex]) {
                uint thisColumn = manyAsksSet;
                manyQuotesAsk[thisColumn]=1;
                askQuotesPrices[thisColumn][0]=thisPrice;
                sprintf(ToBaskNames[thisColumn], "%s", memMap->tradingInterfaceName[thisTIindex]);
                whichColumnIsThisTIAsk[thisTIindex] = thisColumn;
                ToBasksSet[thisTIindex]=true;
                manyAsksSet++;
            } else {
                uint thisColumn = whichColumnIsThisTIAsk[thisTIindex];
                askQuotesPrices[thisColumn][manyQuotesAsk[thisColumn]]=thisPrice;
                manyQuotesAsk[thisColumn]++;
            }
        } // Loop: ask quotes
    }

}


