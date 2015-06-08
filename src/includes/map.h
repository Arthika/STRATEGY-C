/*--------------------------------------------------------------------------------------------------
-- Project     :    CoreLib
-- Filename    :    name: map.h
--                  created_by: carlos
--                  date_created: May 27, 2013
--------------------------------------------------------------------------------------------------
-- File Purpose: defines and structs for shared memory map
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

#ifndef MAP_H_
#define MAP_H_

#include "hftUtils_types.h"
#include "megatick.h"
#include "asset.h"
#include "security.h"
#include "trade.h"
#include "mapMacros.h"
#include "params.h"

/********************************************************************************************************************/
/* CONSTANTS FOR STRATEGY USER (USUALLY MAX VALUES)                                                                 */
/********************************************************************************************************************/

/* TODO send this define information below at stub start-up */
#define MAX_NUMBER_TI            32
#define MAX_NUMBER_PB            16

/** Maximum number of disaggregated levels per liquidity provider */
#define MAX_LIQUID_LEVELS        64

/** Maximum number of liquidity providers */
#define MAX_NUMBER_LIQUID_PROV   8

/** Static number of trades list, to prevent memory allocs and reallocs */
#define MAX_TRADES_LIST_ALIVE    512
#define MAX_TRADES_LIST_HISTORIC 1024

/** Max size in bytes for names of venues, prime brokers or trading interfaces */
#define MAX_NAME_SIZE            32

/* MAX quotes for full book disagregated (per ti and security) */
#define MAX_QUOTES               256

#define MAX_NUMBER_FOR_IDTYPE    0xFFFF /* max possible number for an idtype */

// trading interface ok status
#define TI_STATUS_NOK         0 // Red
#define TI_STATUS_OK          1 // Green
#define TI_STATUS_OK_NO_PRICE 2 // Yellow (no price incoming but heartbeat)


#define MAX_TRADE_REASON_SIZE 256

#define MAX_TEST_MEASURES 1024

// Venue types (each venue has been identified by type to be processed by fsmt and fix machine)
#define venueID_TEST          0
#define venueID_CNX           1
#define venueID_FXALL         2
#define venueID_INTEGRAL      3
#define venueID_HOTSPOT       4
#define venueID_LIQUIDX       5
#define venueID_CITI          6
#define venueID_SAXO          7
#define venueID_EBS           8

// ASK OR BID
#define SIDE_BID 0
#define SIDE_ASK 1

#define MARGINOK			0
#define MARGINCALL			1
#define MARGINCALLRESERVE	2
#define MARGINLIMIT			3

#define AUMARGINLIMIT		1.5

/********************************************************************************************************************/
/* AUXILIARY STRUCTS FOR MAP (used inside map for trading data, not needed individually)                            */
/********************************************************************************************************************/

// Lists of trades (components)
typedef struct _tradeId_t_
{
    idtype accId;                                 // Accounting id identifier (this accounting)
    uint32 tempId;                                // Internal identifier for this trade (temporal identifier up to fixId is coming)
    char   fixId[MAX_TRADE_ID_STRING_SIZE];       // current fix identifier (string)
    char   staticFixId[MAX_TRADE_ID_STRING_SIZE]; // unique fix identifier (string)
}
tradeId_t;

typedef struct _tradeModInfo_t_
{
    boolean    requested; // request flag
    uint32     price;     // Trade replace (price content) requested
    int32      quantity;  // Trade replace (quantity content) requested
    boolean    rejected;  // Modify request has been rejected
    timetype_t timeRejected;
}
tradeModInfo_t;

typedef struct _tradeCancelInfo_t_
{
    boolean    requested;   // Cancel has been requested by strategy
    boolean    rejected;    // Cancel request has been rejected
    timetype_t timeRejected;
}
tradeCancelInfo_t;

typedef struct _tradeInfo_t_
{
    idtype            status;
    idtype            substatus;
    tradeModInfo_t    modify;   // Trade replace has been requested by strategy
    tradeCancelInfo_t cancel;   // Cancel has been requested by strategy

    /* trading time information */
    timetype_t        timeRxCore;       // First time at core when trade is received
    timetype_t        timeSentToVenue;  // Time when send to venue
    timetype_t        timeAckFromVenue; // Time when ack
    timetype_t        timeExecuted;     // Time when finished

    timetype_t        timeReplaceRequest;      // Time when modify was requested
    timetype_t        timeCancelRequest;       // Time when cancel was requested

    char              reason[MAX_TRADE_REASON_SIZE]; // reason text (used normally for rejects)
}
tradeInfo_t;

typedef struct _tradeElement_t_
{
    boolean           alive;     // flag to determine if this element is alive or not (when a trade is cleaned then only this flag is erased)
    tradeCommand_t    params;    // array of trade elements (not static array... dynamic list)
    tradeId_t         ids;       // System information about this trade
    tradeInfo_t       info;
}
tradeElement_t;

// megasorting structure (created at stub, not traveling with megatick)

typedef struct _sortingFB_t_
{
    uint32  price;
    idtype  tiIndex; /**< Trading interface index */
    uint16  quoteIndex;
}
sortingFB_t;

typedef struct _sortingFBbyTI_t_
{
    uint32  price;
    uint16  quoteIndex;
}
sortingFBbyTI_t;


typedef struct _exposureInt_t_
{
    uint32  price;
    int32  amount;
}
exposureInt_t;

typedef struct _tradeTestProfile_
{
    idtype     tiId;
    uint32     time;
    uint32     profile[TEST_TICKTOTRADE_TIME_MESAURES];
    timetype_t timestamp;
}
tradeTestProfile;

#define MAX_REPLACED_QUOTES_INFO 10000
#define MAX_REPLACED_QUOTES_LIST 32

typedef struct _replaceInfo_t_
{
    uint32     origEntryId;
    uint32     newEntryId;
    uint32     price;
    uint32     liquidity;
    uint32     replaces;
    timetype_t timestamp;
    boolean    finished;
}
replaceInfo_t;

typedef struct _replaceInfoList_t_
{
    idtype        security;
    idtype        tiIndex;
    idtype        tiId;
    replaceInfo_t ask[MAX_REPLACED_QUOTES_INFO];
    uint32        askIndex;
    replaceInfo_t bid[MAX_REPLACED_QUOTES_INFO];
    uint32        bidIndex;
}
replaceInfoList_t;


/********************************************************************************************************************/
/* MEMORY MAP STRUCT FOR EACH ACCOUNTING UNIT                                                                       */
/********************************************************************************************************************/

typedef struct _sharedAuMemory_t_
{
    /****************************************************************************************************************/
    /* Configuration section */
    /****************************************************************************************************************/

    /* Ids */
    idtype  auId;                              /**< Accounting Unit identifier for this map */
    idtype  epId;                              /**< Equity pool where this strategy must be running */

    /****************************************************************************************************************/
    /* Accounting section */
    /****************************************************************************************************************/

    number               mlea; /**< Prime broker maximum limited equity access */

    /* Only for this strategy per primer broker*/
    number               Equity_STR[MAX_NUMBER_PB];
    number               UsedMargin_STR[MAX_NUMBER_PB];
    number               FreeMargin_STR[MAX_NUMBER_PB];
    number               ResvMargin_STR[MAX_NUMBER_PB];
    number               FreeMarginResv_STR[MAX_NUMBER_PB];
    number               GlobalEquity_STR;

    /* Assets Exposures section */
    number               AssetExposure[MAX_NUMBER_PB][NUM_ASSETS];              // M1
    number               ReservedAssetExposure[MAX_NUMBER_PB][NUM_ASSETS];      // Reserved M1
    /* Security exposures section */
    mtSecurityExposure_t SecurityExposure[MAX_NUMBER_PB][NUM_SECURITIES];       // M2
    exposureInt_t        SecurityExposureInt[MAX_NUMBER_PB][NUM_SECURITIES];    // M2 integer, Amount and Price

    /* Total exposure */
    number               TotalExposure[MAX_NUMBER_PB][NUM_ASSETS];              // M4
    number               ReservedTotalExposure[MAX_NUMBER_PB][NUM_ASSETS];      // Reserved M4

    /* Total exposure multiplied per price */
    number               TotalExposureWprice[MAX_NUMBER_PB][NUM_ASSETS];        // M5
    number               TotalMarginWprice[MAX_NUMBER_PB][NUM_ASSETS];          // M6
    number               TotalReservedMarginWprice[MAX_NUMBER_PB][NUM_ASSETS];  // M6 reserve

    /* Global data accumulated for all PB by AU */
    number               GblAssetExposureAU[NUM_ASSETS];              // M1 Accounting unit accumulation (all prime brokers)
    mtSecurityExposure_t GblSecurityExposureAU[NUM_SECURITIES];       // M2 Accounting unit accumulation (all prime brokers)
    exposureInt_t        GblSecurityExposureAUInt[NUM_SECURITIES];    // M2 integer Price
    number               GblTotalExposureAU[NUM_ASSETS];              // M4 Accounting unit accumulation (all prime brokers)
    number               GblTotalExposureWpriceAU[NUM_ASSETS];        // M5 Accounting unit accumulation (all prime brokers)

    /****************************************************************************************************************/
    /* Trades section */
    /****************************************************************************************************************/

    tradeElement_t       alive[MAX_TRADES_LIST_ALIVE];        /**< trades list but only alive at core */
    uint32               numberOfAlives;                      /**< Total number of trades in this alive list */
    tradeElement_t*      listByPb[MAX_NUMBER_PB][MAX_TRADES_LIST_ALIVE];  /**< pointers array to list by prime broker account */
    tradeElement_t*      listByTi[MAX_NUMBER_TI][MAX_TRADES_LIST_ALIVE];  /**< pointers array to list by trading interface */

    tradeElement_t       historic[MAX_TRADES_LIST_HISTORIC];  /**< just after a trade has been resolved go to historic list (ring buffer) */
    uint32               indexOfHistoric;                     /**< Current index of historic list (index with next position) */

    /****************************************************************************************************************/
    /* Profile info */
    /****************************************************************************************************************/
    tradeTestProfile     testTime[MAX_TEST_MEASURES];         /**< Ring buffer to introduce tick to trade measures */
    uint32               testTimeIndex;
}
sharedAuMemory_t;

typedef struct _sharedEpMemory_t_
{
    idtype  epId;    /**< Equity pool identifier */

    number  mmum; /**< Prime broker maximum used margin */
    number  mgum; /**< Prime broker guaranteed used margin */

    /* Accounting of equity pool, where this strategy is running */
    number               Equity_EP[MAX_NUMBER_PB];
    number               UsedMargin_EP[MAX_NUMBER_PB];
    number               FreeMargin_EP[MAX_NUMBER_PB];
    number               ResvMargin_EP[MAX_NUMBER_PB];
    number               FreeMarginResv_EP[MAX_NUMBER_PB];
    number               GlobalEquity_EP;

    /* Assets Exposures section */
    number               AssetExposureEP[MAX_NUMBER_PB][NUM_ASSETS];             // M1 for Equity Pool
    number               AssetExposureReservedEP[MAX_NUMBER_PB][NUM_ASSETS];     // M1 Reserved for Equity Pool

    /* Security exposures section */
    mtSecurityExposure_t SecurityExposureEP[MAX_NUMBER_PB][NUM_SECURITIES];      // M2 for Equity Pool
    exposureInt_t        SecurityExposureEPInt[MAX_NUMBER_PB][NUM_SECURITIES];   // M2 integer, Amount and Price

    number               TotalExposureEP[MAX_NUMBER_PB][NUM_ASSETS];             // M4 for Equity Pool
    number               TotalExposureReservedEP[MAX_NUMBER_PB][NUM_ASSETS];     // M4 Reserved for Equity Pool

    /* Total exposure multiplied per price */
    number               TotalExposureWpriceEP[MAX_NUMBER_PB][NUM_ASSETS];       // M5
    number               TotalMarginWpriceEP[MAX_NUMBER_PB][NUM_ASSETS];         // M6
    number               TotalReservedMarginWpriceEP[MAX_NUMBER_PB][NUM_ASSETS]; // M6 reserve

    /* EXPOSURES */
    number               GblAssetExposureEP[NUM_ASSETS];            // M1 Equity pool accumulation
    mtSecurityExposure_t GblSecurityExposureEP[NUM_SECURITIES];     // M2 Equity pool accumulation
    exposureInt_t        GblSecurityExposureEPInt[NUM_SECURITIES];  // M2 integer Price
    number               GblTotalExposureEP[NUM_ASSETS];            // M4 Equity pool accumulation
    number               GblTotalExposureWpriceEP[NUM_ASSETS];            // M4 Equity pool accumulation
}
sharedEpMemory_t;

/********************************************************************************************************************/
/* MEMORY MAP STRUCT FOR BOOK                                                                                       */
/********************************************************************************************************************/

typedef struct _sharedMemory_t_
{
    uint64     megatickSequence; /**< Current megatick sequence number */
    timetype_t megatickTime;     /**< Last incoming megatick time */

    /****************************************************************************************************************/
    /* Book Configuration section */
    /****************************************************************************************************************/
    idtype  nAus;                              /**< Number of max accounting units allocated inside this map */
    idtype  nEps;                              /**< Number of max equity pools allocated inside this map */

    /* Ids */
    idtype  bookId;                            /**< Book Id associated to this accounting unit */
    uint32  bookMulticastAddress;              /**< Multicast address */
    uint16  bookMulticastPort;                 /**< Multicast address */
    idtype  nPrimeBrokers;                     /**< Number of available prime brokers in this book */
    idtype  primeBroker[MAX_NUMBER_PB];        /**< Prime broker identifier list */
    char    primeBrokerName[MAX_NUMBER_PB][MAX_NAME_SIZE]; /**< Name of prime broker */
    idtype  nTradingInterfaces;                /**< Number of available trading interfaces in this book*/
    idtype  tradingInterface[MAX_NUMBER_TI];   /**< Trading interface identifier list */
    char    tradingInterfaceName[MAX_NUMBER_TI][MAX_NAME_SIZE]; /**< Name of trading interface */
    byte    tradingInterfaceOk[MAX_NUMBER_TI]; /**< true if trading interface is ready to trade */
    idtype  whichPrimeBroker[MAX_NUMBER_TI];   /**< Prime broker Id to which trading interfaces correspond */
    idtype  whichVenue[MAX_NUMBER_TI];         /**< Venue Id to which trading interfaces correspond */
    idtype  whichVenueType[MAX_NUMBER_TI];     /**< Venue Type to which trading interfaces correspond */

    /* inverse arrays to get immediately index from identifier */
    uint16  tradingInterfaceIndex[MAX_NUMBER_FOR_IDTYPE];  /**< Put trading interface identifier as array element and get index used for other arrays */
    uint16  primeBrokerIndex[MAX_NUMBER_FOR_IDTYPE];       /**< Put prime broker identifier as array element and get index used for other arrays */

    /* Matrixes for trade simulation (from core configuration) */
    number  pbml[MAX_NUMBER_PB][NUM_ASSETS]; /**< Prime broker maximum leverage matrix */

    // user reserved data... next identifiers will be by the user to define quickly what accounting unit and equity pool id is used
    // by the main strategy user... not used by parser or library, this must be set and used by user
    idtype strAuId;
    int32  strAuIndex;
    idtype strEpId;
    int32  strEpIndex;
    char   strUser[MAX_USER_SIZE];
    sharedAuMemory_t*    strAu;
    sharedEpMemory_t*    strEp;

    /****************************************************************************************************************/
    /* Books section */
    /****************************************************************************************************************/

    /* Top of book */
    mtbookElement_t   mapTiTbAsk[MAX_NUMBER_TI][NUM_SECURITIES];
    mtbookElement_t   mapTiTbBid[MAX_NUMBER_TI][NUM_SECURITIES];

    /* Full book aggregated */
    mtbookElement_t   mapTiFBAAsk[MAX_NUMBER_TI][NUM_SECURITIES][MAX_QUOTES];
    uint16            mapTiFBAAskSorting[MAX_NUMBER_TI][NUM_SECURITIES][MAX_QUOTES];
    uint16            mapTiFBAAskLevels[MAX_NUMBER_TI][NUM_SECURITIES];
    mtbookElement_t   mapTiFBABid[MAX_NUMBER_TI][NUM_SECURITIES][MAX_QUOTES];
    uint16            mapTiFBABidSorting[MAX_NUMBER_TI][NUM_SECURITIES][MAX_QUOTES];
    uint16            mapTiFBABidLevels[MAX_NUMBER_TI][NUM_SECURITIES];

    /* Full book desaggregated (array of quotes not sorted) */
    mtbookElement_t   mapFBQuotesAsk[MAX_NUMBER_TI][NUM_SECURITIES][MAX_QUOTES]; /**< Quotes in full book */
    timetype_t        mapFBQuotesAskFirstTime[MAX_NUMBER_TI][NUM_SECURITIES][MAX_QUOTES]; /**< Quotes in full book */
    char              mapFBQuotesProvidersAsk[MAX_NUMBER_TI][NUM_SECURITIES][MAX_QUOTES][MAX_NAME_SIZE];
    mtbookElement_t   mapFBQuotesBid[MAX_NUMBER_TI][NUM_SECURITIES][MAX_QUOTES]; /**< Quotes in full book */
    timetype_t        mapFBQuotesBidFirstTime[MAX_NUMBER_TI][NUM_SECURITIES][MAX_QUOTES]; /**< Quotes in full book */
    char              mapFBQuotesProvidersBid[MAX_NUMBER_TI][NUM_SECURITIES][MAX_QUOTES][MAX_NAME_SIZE];

    sortingFB_t       mapFBSortingBid[NUM_SECURITIES][MAX_QUOTES*MAX_NUMBER_TI];
    uint16            mapFBNumberofsortingBid[NUM_SECURITIES];
    sortingFB_t       mapFBSortingAsk[NUM_SECURITIES][MAX_QUOTES*MAX_NUMBER_TI];
    uint16            mapFBNumberofsortingAsk[NUM_SECURITIES];

    sortingFBbyTI_t   mapFBSortingBidbyTI[MAX_NUMBER_TI][NUM_SECURITIES][MAX_QUOTES];
    uint16            mapFBNumberofsortingBidbyTI[MAX_NUMBER_TI][NUM_SECURITIES];
    sortingFBbyTI_t   mapFBSortingAskbyTI[MAX_NUMBER_TI][NUM_SECURITIES][MAX_QUOTES];
    uint16            mapFBNumberofsortingAskbyTI[MAX_NUMBER_TI][NUM_SECURITIES];


    /* Prime brokers prices for securities and assets to denomination currency */
    mtAssetPrice_t    mapPbAp[MAX_NUMBER_PB][NUM_ASSETS];
    mtSecurityPrice_t mapPbSpAsk[MAX_NUMBER_PB][NUM_SECURITIES];
    mtSecurityPrice_t mapPbSpBid[MAX_NUMBER_PB][NUM_SECURITIES];

    /* Sorting prices for trading interface at top of book */
    mtSorting_t       mapSortingAsk[NUM_SECURITIES][MAX_NUMBER_TI];
    uint16            mapSortAskNumberOfTi[NUM_SECURITIES];
    mtSorting_t       mapSortingBid[NUM_SECURITIES][MAX_NUMBER_TI];
    uint16            mapSortBidNumberOfTi[NUM_SECURITIES];

    mtAssetPrice_t    mapBookAp[NUM_ASSETS];
    uint32            mapBookApInt[NUM_ASSETS];
    mtSecurityPrice_t mapBookSpAsk[NUM_SECURITIES];
    uint32            mapBookSpAskInt[NUM_SECURITIES];
    mtSecurityPrice_t mapBookSpBid[NUM_SECURITIES];
    uint32            mapBookSpBidInt[NUM_SECURITIES];
    // average price for book (average for bid and ask and integer)
    mtSecurityPrice_t mapBookSp[NUM_SECURITIES];
    uint32            mapBookSpInt[NUM_SECURITIES];

    /* GLOBAL ACCOUNTING INFO */

    /* Accounting for prime broker for all strategies */
    number               Equity_PB[MAX_NUMBER_PB];
    number               UsedMargin_PB[MAX_NUMBER_PB];
    number               FreeMargin_PB[MAX_NUMBER_PB];
    number               ResvMargin_PB[MAX_NUMBER_PB];
    number               FreeMarginResv_PB[MAX_NUMBER_PB];
    number               GlobalEquity_PB;
    /* Assets and securities Exposures section */
    number               AssetExposurePB[MAX_NUMBER_PB][NUM_ASSETS];           // M1 for Prime Broker
    number               AssetExposureReservedPB[MAX_NUMBER_PB][NUM_ASSETS];   // M1 Reserved for Prime Broker
    mtSecurityExposure_t SecurityExposurePB[MAX_NUMBER_PB][NUM_SECURITIES];    // M2 for Prime Broker
    exposureInt_t        SecurityExposurePBInt[MAX_NUMBER_PB][NUM_SECURITIES]; // M2 integer, Amount and Price
    number               TotalExposurePB[MAX_NUMBER_PB][NUM_ASSETS];           // M4 for Prime Broker
    number               TotalExposureReservedPB[MAX_NUMBER_PB][NUM_ASSETS];   // M4 Reserved for Prime Broker
    /* Total exposure multiplied per price */
    number               TotalExposureWpricePB[MAX_NUMBER_PB][NUM_ASSETS];     // M5
    number               TotalMarginWpricePB[MAX_NUMBER_PB][NUM_ASSETS];
    number               TotalReservedMarginWpricePB[MAX_NUMBER_PB][NUM_ASSETS];
    /* EXPOSURES */
    number               GblAssetExposurePB[NUM_ASSETS];        // M1 Prime brokers accumulation
    mtSecurityExposure_t GblSecurityExposurePB[NUM_SECURITIES]; // M2 Prime brokers accumulation
    exposureInt_t        GblSecurityExposurePBInt[NUM_SECURITIES];    // M2 integer Price
    number               GblTotalExposurePB[NUM_ASSETS];        // M4 Prime brokers accumulation
    number               GblTotalExposureWpricePB[NUM_ASSETS];        // M4 Prime brokers accumulation

    number               sharedMarginPB[MAX_NUMBER_PB]; /**< Shared margin for this prime broker */
    number               sharedReservedMarginPB[MAX_NUMBER_PB]; /**< Shared margin for this prime broker */

    /****************************************************************************************************************/
    /* Replaced Quotes information (only activated by user for one security and one ti per element in list) */
    /****************************************************************************************************************/
    replaceInfoList_t*   replacedInfo[MAX_REPLACED_QUOTES_LIST];


    /****************************************************************************************************************/
    /* Acconting units and equity pools section  */
    /****************************************************************************************************************/

    /* Pointer to array with information about each accounting unit */
    /* allocated at memory at start */
    /* Size depends on number of accounting units to work */
    sharedAuMemory_t*    AccountingUnit;
    sharedEpMemory_t*    EquityPool;
}
sharedMemory_t;


/********************************************************************************************************************/
/* CHANGED FLAGS STRUCTURE                                                                                          */
/********************************************************************************************************************/

/* Flags change for any element of the memory map */
typedef struct _change_t_
{
    uint16 mtCode;   /**< megatick code of this change (let's know element changed) */
    uint16 first;    /**< first index of indirection in the case of array or matrix */
    uint16 second;   /**< second index of indirection in the case of array or matrix */
    uint16 third;    /**< Additional data used at full book matrixes */
    uint16 fourth;   /**< Additional data used at full book matrixes */
}
change_t;

/* Changes array */
typedef struct _flagsChanges_t_
{
    uint16   numberChanges;        /**< Number of changes inside this list */
    change_t changes[MAXNUMTICK];  /**< changes contents */
}
flagsChanges_t;



/********************************************************************************************************************/
/* FUNCTIONS                                                                                                        */
/********************************************************************************************************************/

/********************************************************************************************************************/
/* TRADING functions (this functions are defined here due to you will need access to memory map)                    */
/* you can find defines and struct needed at  trade.h header file.                                                  */
/********************************************************************************************************************/
extern int32 SendTradeToCore     (sharedMemory_t* memMap, char* user, tradeCommand_t* tradeCommand, uint32* tempTradeId);
extern int32 ModifyTradeToCore   (sharedMemory_t* memMap, char* user, char* fixTradeId, uint32 price, int32 quantity, boolean unconditional);
extern int32 CancelTradeToCore   (sharedMemory_t* memMap, char* user, char* fixTradeId);
extern int32 SimulateTrade       (sharedMemory_t* memMap, char* user, tradeCommand_t* tradeCommand, number* AUfreemargin,
                                   number* EPfreemargin, number* PBfreemargin, boolean* tradeResult);

/********************************************************************************************************************/
/* MEMORY MAP ACCESS: (Remember that strategy starts with memory map access called and locked and only need to call */
/* release map from strategy after finish or no more access needed. This functions could be useful when you need    */
/* asynchronous access (for instance from webserver, or trading out of the strategy)                                */
/* REMEMBER too release the map after finish                                                                        */
/********************************************************************************************************************/

/* Get memory map. This will activate the mutex to guaranty exclusive access and return a pointer to map */
extern sharedMemory_t* getMemoryMap (void);

/* release memory map unlocks the semaphore locked  at getMemoryMap */
extern int32 releaseMap (void);

/* Get memory map with list of changes and old values. Tthis will activate the mutex to guaranty exclusive access */
extern int32 getMemoryMapWithChanges (sharedMemory_t** memMap, flagsChanges_t** memChanges);

/* release memory map unlocks the semaphore and clean changes list and old map */
extern int32 releaseMapAndCleanChanges (void);

/* release map from strategy is the same than release and clean changed flags */
#define releaseMapFromStrategy releaseMapAndCleanChanges

/********************************************************************************************************************/
/* Other USER utilities                                                                                             */
/********************************************************************************************************************/

/* change timeout in millisecs to wait for data. After elapsed time strategy is awaked but without no changes */
extern int32 setNotifyTimeout (uint32 millisecs);

/* start or change test trade timeout... = 0 means deactivate */
extern int32 setTimeTestTrade (uint32 testTradeTime);

/* set main identaifer for strategy user and his equity pool, only used by user */
extern int32 setUserAsStrategyMainUser (sharedMemory_t* memMap, char* user);

/* Replaced quotes tools */
extern int32 setSecurityForQuotesReplacedInfo (sharedMemory_t* memMap, uint32 indexList, idtype security, idtype tiIndex);
extern int32 resetSecurityForQuotesReplacedInfo (sharedMemory_t* memMap, uint32 indexList);

#endif /* MAP_H_ */
