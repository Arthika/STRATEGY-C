/*
 * monitoring.h
 *
 *  Created on: May 21, 2014
 *      Author: julio
 */

#ifndef MONITORING_H_
#define MONITORING_H_

#include "map.h"


#define _MATERIALITY_THRES_ 100.0 // For maximum equity recording

extern boolean      ValuationChangeAlarm;
extern boolean      DrawdownAlarm;
extern boolean      TooManyTradesAlarm;
extern boolean      MalfunctionAlarm;
extern char         MalfunctionAlarmText[4096];
extern boolean      withinOperatingHours;

extern uint32       nTradesThisSecond;
extern uint32       nTradesThis10Seconds;
extern uint32       nTradesThisMinute;

extern boolean      pricesDelayed[MAX_NUMBER_TI];
extern uint64       totalPricesDelayedDowntime_ms[MAX_NUMBER_TI];
extern uint         totalPricesDelayedOccurrences[MAX_NUMBER_TI];

extern boolean      noTicks[MAX_NUMBER_TI];
extern uint64       totalNoTicksDowntime_ms[MAX_NUMBER_TI];
extern uint         totalNoTicksOccurrences[MAX_NUMBER_TI];


/************************************************************************************/
/* Performance and operation utilities                                              */

typedef struct _ti_performance_t_ {
// Statistics about completed orders
    // Immediate Limit trades
    uint32      fullyFilledImmediateLimitTradesToday;
    uint32      partiallyFilledImmediateLimitTradesToday;
    uint32      failedAndRejectedImmediateLimitTradesToday;
    uint32      userCanceledImmediateLimitTradesToday;
    // Market trades
    uint32      fullyFilledMarketTradesToday;
    uint32      partiallyFilledMarketTradesToday;
    uint32      failedAndRejectedMarketTradesToday;
    uint32      userCanceledMarketTradesToday;
    // Other trades
    uint32      fullyFilledOtherTradesToday;
    uint32      partiallyFilledOtherTradesToday;
    uint32      failedAndRejectedOtherTradesToday;
    uint32      userCanceledOtherTradesToday;
// Statistics about replaces
    uint32      replacesDoneToday;
// Statistics about amounts traded
    number      amountTradedToday;
} ti_performance_t;

extern ti_performance_t tiPerformanceInfo[MAX_NUMBER_TI];


typedef struct _ti_operation_t_ {
// "In a row" counters to avoid infinite repetitions
    uint32      rejectsInARow;
    uint32      failedImmediateLimitTradesInaRow;
    uint32      successfulImmediateLimitTradesInaRow;
    uint32      successfulMarketTradesInaRow;
// Traded quotes through limit hedging
    uint32      goodQuotesToday;
    uint32      badQuotesToday;
// Yellow and red cards
    uint32      yellowCardCounter;
    uint32      yellowCardTimestamp;
    boolean     yellowCard;
    boolean     redCard;
} ti_operation_t;

extern ti_operation_t tiOperation[MAX_NUMBER_TI];


/************************************************************************************/
/* Indicators utilities                                                             */

typedef struct _trade_indicators_t_ {
    uint32      totalTrades;
    uint32      fullyFilledTrades;
    uint32      partiallyFilledTrades;
    uint32      failedAndRejectedTrades;
    uint32      userCanceledTrades;
    uint32      replacesDone;
    number      amountTraded;
} tradeIndicators_t;


#define INDICATORS_DIR             "INDICATORS"

#define TIMEFRAME_TODAY            0
#define TIMEFRAME_THIS_WEEK        1
#define TIMEFRAME_THIS_MONTH       2
#define TIMEFRAME_SINCE_INCEPTION  3
#define NUM_TIMEFRAMES    4

extern void saveAllIndicators(sharedMemory_t* memMap);
// extern void writeIndicators(idtype PBindex, idtype timeframe, tradeIndicators_t *ind, int32 eq);
// extern void readIndicators(sharedMemory_t* memMap, idtype PBindex, idtype timeframe, tradeIndicators_t *indResult, int32 *eqResult);

// The rendering function return rendered indicators. Names are returned in indicatorNames[0 .. *manyIndicators-1], and
// values in indicatorValues[0 .. *manyIndicators-1]
// Set allPBs to true to get indicators aggregating all PBs
// The function returns false only if PBindex invalid andallPbs is false

#define MAX_INDICATORS    64

extern boolean renderIndicators(sharedMemory_t* memMap, boolean allPBs, idtype PBindexIfNotForAll, uint *manyIndicators,
                                  char indicatorNames[MAX_INDICATORS][NUM_TIMEFRAMES][SLEN], char indicatorValues[MAX_INDICATORS][NUM_TIMEFRAMES][SLEN]);


extern idtype       many_securities;
extern idtype       which_securities[NUM_SECURITIES];
extern idtype       many_assets;
extern idtype       which_assets[NUM_ASSETS];


/************************************************************************************/
/* Quote chains                                                                     */

#define MAX_CHAINS 1024
#define MAX_CHAIN_SAMPLES 2048

typedef struct _quoteChainElement_t {
    timetype_t  fixtime;
    timetype_t  stubtime;
    uint32      price;
    uint32      liquidity;
} quoteChainElement_t;

extern boolean  chainsEnabled;                      // Set to true if to be enabled - please through resetChains()
extern idtype   chainSecurity;                      // Read only please, set through resetChains(). Chains only available for one security at a time

extern uint16   whichChainBid[MAX_QUOTES];          // This gives you the chain index corresponding to each quote
extern uint16   whichChainAsk[MAX_QUOTES];
extern uint16   nQuotesInChainBid[MAX_CHAINS];      // This gives you the number of samples available in a particular chain
extern uint16   nQuotesInChainAsk[MAX_CHAINS];
extern uint16   lastQuoteInChainBid[MAX_CHAINS];    // This gives you the last (most current) sample in the chain (which is a circular buffer)
extern uint16   lastQuoteInChainAsk[MAX_CHAINS];

extern quoteChainElement_t quoteChains[MAX_CHAINS][MAX_CHAIN_SAMPLES]; // This is the actual repository of quote chains

extern void     resetChains(boolean enable, idtype whichSecurity);  // This has to be called in strategyInit. It sets chainScurity to whichSecuity
extern void     saveChainsToFile(char *filename);   // Saved as a .csv file



/************************************************************************************/
/* Profiling utilities                                                              */

#define MAX_HISTOGRAM_BUCKETS 64

typedef struct _bucket_t_ {
    int         bucketValue; // Upper limit for elements in this bucket (less than or equal)
    char        bucketName[SLEN]; // For rendering purposes
    uint32      samples; // Number of samples in this bucket
    number      pct; // % of samples in this bucket
    uint32      accSamples; // Cumulative number of samples up to this bucket
    number      accPct; // Cumulative % of samples up to this bucket
} bucket_t;

typedef struct histogram_t_ {
    int        nBuckets; // Number of buckets in this histogram
    bucket_t   bucket[MAX_HISTOGRAM_BUCKETS]; // bucket[nBuckets] is the last buckets with all elements remaining
                                            // (i.e. with samples bigger than bucket[nBuckets-1].bucketValue)
} histogram_t;

#define MAX_PROFILING_SAMPLES 1024 // In general we would use the same value of MAX_TEST_MEASURES (defined in map.h)
#define TICK_TO_ORDER_INITIAL_SAMPLES_TO_DISCARD 5 // Samples to be discarded just in TickToOrder histograms

extern histogram_t         FixToCore[MAX_NUMBER_TI]; // TI index!
extern histogram_t         FixToStub[MAX_NUMBER_TI]; // TI index!
extern void                buildFixToUsHistograms(sharedMemory_t* memMap); // Returns histograms in FixToCore[] and FixToStub[] (removes ping bias!!)

extern histogram_t         CoreToStub; // One common histogram for all TIs (all samples accounted for)
extern histogram_t         TickToOrder; // One common histogram, delays measured automatically and taken from map
extern histogram_t         Callback; // One common histogram, delays measured from actual callbacks
extern void                addCallbackDelay(timetype_t *callbackStart, timetype_t *callbackEnd);
extern void                buildCoreHistograms(sharedMemory_t* memMap); // Important: assumes core and stub with the same time base

extern histogram_t         profilingHistograms[TEST_TICKTOTRADE_TIME_MESAURES+1]; // Histograms for tick-to-trade latency measurements breakdown
extern void                buildProfilingHistograms(sharedMemory_t* memMap); // This builds everything

extern histogram_t         ExecTimesImmediateLimit[MAX_NUMBER_TI]; // TI index!
extern histogram_t         ExecTimesMarket[MAX_NUMBER_TI]; // TI index!
extern histogram_t         ExecTimesCancel[MAX_NUMBER_TI]; // TI index!
extern histogram_t         ExecTimesReplace[MAX_NUMBER_TI]; // TI index!
extern uint64              addExecTime(idtype execTimeType, timetype_t *execStart, timetype_t *execEnd, idtype ti); // Returns exec time in us
#define EXEC_TIME_IMMEDIATE_LIMIT 1
#define EXEC_TIME_MARKET 2
#define EXEC_TIME_CANCEL 3
#define EXEC_TIME_REPLACE 4
extern void                buildExecTimeHistograms(sharedMemory_t* memMap);

extern void                save_samples(sharedMemory_t* memMap); // This shows all histograms samples to files



/************************************************************************************/
/* Market watch utilities                                                           */

#define MAX_FB_RENDER_ROWS 512
#define MAX_FB_RENDER_COLUMNS 2*MAX_NUMBER_TI+3

extern void renderFB(
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

);

#define FB_RENDER_COLOR_EMPTY 0

#define FB_RENDER_COLOR_EMPTY_BID 1
#define FB_RENDER_COLOR_QUOTE_BID 2
#define FB_RENDER_COLOR_TI_TOB_BID 3
#define FB_RENDER_COLOR_OVERALL_TOB_BID 4

#define FB_RENDER_COLOR_EMPTY_ASK 5
#define FB_RENDER_COLOR_QUOTE_ASK 6
#define FB_RENDER_COLOR_TI_TOB_ASK 7
#define FB_RENDER_COLOR_OVERALL_TOB_ASK 8

#define FB_RENDER_COLOR_OWN_BUY 9
#define FB_RENDER_COLOR_OWN_SELL 10

#define FB_RENDER_COLOR_PRICE 11


extern void ToBComparison(
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

);

// That's it


// This is for automatic management:

extern boolean      auto_management_connected;
extern boolean      strategy_connected;

// Done


extern int32        init_monitoring(sharedMemory_t* memMap, flagsChanges_t* changesMap);
extern int32        do_trade_reporting (sharedMemory_t* memMap, flagsChanges_t* changesMap); // To be called *before* strategy logic
extern int32        do_yellow_cards(sharedMemory_t* memMap, flagsChanges_t* changesMap); // To be called *before* strategy logic
extern int32        do_monitoring (sharedMemory_t* memMap, flagsChanges_t* changesMap); // To be called *after* strategy logic


#endif /* MONITORING_H_ */
