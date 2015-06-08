// To add to util.h

#ifndef MYUTIL_H_
#define MYUTIL_H_

#include "hftUtils_types.h"
#include "map.h"

#define NLEN 256 // For rendering numbers as strings with dots
#define SLEN 1024 // For identifiers (e.g. prime brokers, venues, etc.)
#define MLEN 4096 // For messages

/************************************************************************************/
/* Assets and securities utilities                                                  */

extern char securityName[NUM_SECURITIES][NLEN];
extern char assetName[NUM_ASSETS][NLEN];
extern boolean isToBPriceObsolete(sharedMemory_t* memMap, idtype tradingInterface, idtype secId, uint8 side);
extern void print_prices(sharedMemory_t *memMap, idtype sec, boolean printAsks, boolean printBids);
extern void print_prices_full_book(sharedMemory_t *memMap, idtype sec);
extern void print_exposures(sharedMemory_t *memMap, idtype sec);
extern void print_exposures_au(sharedMemory_t *memMap, int32 auIndex, idtype sec);
extern boolean areAllPosisClosed(sharedMemory_t *memMap);
extern boolean areAllPosisClosed_au(sharedMemory_t *memMap, int32 auIndex);

/************************************************************************************/
/* Trade utilities                                                                   */

extern int32 getTradeTypeString (idtype tradeType, char *ptr);
extern int32 getTimeInForceString (char timeInForce, char *ptr);
extern int32 getTradeStatusString (idtype tradeStatus, idtype tradeSubStatus, char *ptr);

extern void renderWithDots(char *where, int64 what, boolean with_sign);
extern void renderPct(char *where, int64 numerator, int64 denominator, boolean with_sign);
extern void renderPrice(char *where, int32 price, int nDecimals);

extern boolean isTradeAlive(sharedMemory_t* memMap, idtype tradingInterfaceIndex, uint8 side, uint32 reserved_MIN, uint32 reserved_MAX, int32 *tradeNumber);
extern boolean isTradeAlive_au(sharedMemory_t* memMap, int32 auIndex, idtype tradingInterfaceIndex, uint8 side, uint32 reserved_MIN, uint32 reserved_MAX, int32 *tradeNumber);
extern boolean enoughMarginToTrade(sharedMemory_t* memMap, char* user, idtype tradingInterfaceIndex, int32 amount, idtype security, uint8 side);

extern void printHistoricalTradeReport(sharedMemory_t* memMap, uint32 whichTrade, char *where);
extern void printHistoricalTradeReport_au(sharedMemory_t* memMap, int32 auIndex, uint32 whichTrade, char *where);
extern void printLiveOrderReport(sharedMemory_t* memMap, uint32 whichTrade, char *where);
extern void printLiveOrderReport_au(sharedMemory_t* memMap, int32 auIndex, uint32 whichTrade, char *where);

extern int32 willThisTradeWork(sharedMemory_t* memMap, tradeCommand_t *tc);
extern int32 willThisTradeWork_au(sharedMemory_t* memMap, int32 auIndex, tradeCommand_t *tc);
//Exit codes:
#define __everythingOK 0
#define __notEnoughLiquidityInToB 1
#define __isPriceObsolete 2
#define __notEnoughMargin 3
#define __tradingInterfaceNotOK 4

// extern boolean sendBestHedgingTradeLimit(sharedMemory_t* memMap, int32 amount, idtype security, boolean market, int32 purpose, idtype *tradingInterfaceIndex);

extern int32 roundTrade(int32 amountToRound);

// These two just wrappers to add too many trades control:
extern int32 mySendTradeToCore     (sharedMemory_t* memMap, char* user, tradeCommand_t* tradeCommand, uint32* tempTradeId, boolean *succeeded);
extern int32 myModifyTradeToCore   (sharedMemory_t* memMap, char* user, char* fixTradeId, idtype tiIndex, uint32 price, int32 quantity, boolean unconditional);
extern int32 myCancelTradeToCore   (sharedMemory_t* memMap, char* user, char* fixTradeId, idtype tiIndex);


/************************************************************************************/
/* TI and PB utilities                                                              */

#define MAX_IDS 65536
extern int32 getTIStatusString (idtype TIStatus, char *ptr);

#define VENUE_TYPE_CNX 102
#define VENUE_TYPE_FXALL 104
#define VENUE_TYPE_LIQUID 108
#define VENUE_TYPE_HOTSPOT 105

/************************************************************************************/
/* Strategy utilities                                                               */

#define MAX_MESSAGES 25
#define STR_MSG_CRITICAL 1 // Use "???" as msg header
#define STR_MSG_ERROR 2 // Use "!!!" as msg header
#define STR_MSG_ALARM 3 // Use "###" as msg header
#define STR_MSG_TRADING_INFO 4 // Use ">>>" as msg header, or "@@@" in macro commands
#define STR_MSG_SYSTEM_INFO 5 // Use "---" as msg header
#define STR_MSG_DEBUG 6

typedef struct msg_queue_t_ {
    char msg[SLEN];
    int type;
    timestruct_t strtime;
    timetype_t systime;
} msg_queue_t;

extern int many_messages;
extern void add_message(char *msg, int type);
extern void get_message(int msg_index, msg_queue_t *msg);
extern void clear_all_messages(void);
extern void render_message(int msg_index, char *where);
extern void render_message_HTML(int msg_index, char *where);


/************************************************************************************/
/* Books utilities                                                               */

extern void removeOwnOrdersFromQuoteList(

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

            );

extern void topOfBooksExOwnOrders(

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

            );

/************************************************************************************/
/* Misc utilities                                                                   */

// These constants to interpret dayofweek field in timestruct_t arrays
// (e.g. when using getStructTimeXXX functions in util.h)

#define SUNDAY 0
#define MONDAY 1
#define TUESDAY 2
#define WEDNESDAY 3
#define THURSDAY 4
#define FRIDAY 5
#define SATURDAY 6

extern void printTime(timetype_t *t, char *ptr);
extern int64  my_getTimeDifSigned (timetype_t* systimeStart, timetype_t* systimeEnd);
extern uint64 my_getTimeDif (timetype_t* systimeStart, timetype_t* systimeEnd);



/************************************************************************************/
/* Operating hours utilities                                                        */
extern boolean checkWithinOperatingHours(void); // Checks weekly operating hours, weekly maintenance and (coming soon) news

/************************************************************************************/
/* Init utilities                                                                  */
extern void init_myUtils(sharedMemory_t *memMap);


#endif /* MYUTIL_H_ */
