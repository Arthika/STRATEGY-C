/*--------------------------------------------------------------------------------------------------
-- Project     :    CoreLib
-- Filename    :    name: megatick.h
--                  created_by: carlos
--                  date_created: May 27, 2013
--------------------------------------------------------------------------------------------------
-- File Purpose: Megatick communication from Core to Stub
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


#ifndef MEGATICK_H_
#define MEGATICK_H_

#include "hftUtils_types.h"

// version and revision of megatick protocol (x.xx)
// used to accept or not connections from stub or dblogger based on version
#define MEGATICK_VERSION  1
#define MEGATICK_REVISION 00


#define MAXHEADERSIZE 8
#define MAXTICKSIZE   1024
#define MAXNUMTICK    1024

#define MAXMTLENGHT   ((MAXHEADERSIZE + MAXTICKSIZE) * MAXNUMTICK)

#define MAX_PROFILING_LEVELS_MT 4  /* Profiling levels to get an address to save time difference for several points */

#define MAX_TRADE_ID_STRING_SIZE 32
#define MAX_FIX_TIME_STRING_SIZE 24

#define TEST_TICKTOTRADE_TIME_MESAURES 6 /* must be the same than fix_contants.h TEST_TRADE_PROFILE... */


/* Megatick header codes */

#define MTC_START               0x00FF
#define MTC_SNAPSHOT_START      0x00EF
#define MTC_SNAPSHOT_CONTINUE   0x00EE
#define MTC_SNAPSHOT_END        0x00ED


#define MTC_END                 0x00FE

#define MTC_TOPASK              0x0001
#define MTC_TOPBID              0x0002
#define MTC_FBA_ASK             0x0003  /** < Price for full book aggregated map ask (all levels information) */
#define MTC_FBA_BID             0x0004  /** < Price for full book aggregated map bid (all levels information) */
#define MTC_FBD_ASK             0x0005  /** < Price for full book disaggregated map ask (all levels information for one provider) */
#define MTC_FBD_BID             0x0006  /** < Price for full book disaggregated map bid (all levels information for one provider) */
#define MTC_FBD_CLEAN           0x0007
#define MTC_FBA_SORTING_ASK     0x0008
#define MTC_FBA_SORTING_BID     0x0009

#define MTC_FBD_INFO            0x000A  /**< Send information about quotes chains, about replaces or end chain */

#define MTC_PBSPASK             0x0010
#define MTC_PBSPBID             0x0011
#define MTC_PBAP                0x0012
#define MTC_BOOKSPASK           0x0014
#define MTC_BOOKSPBID           0x0015
#define MTC_BOOKAP              0x0016
#define MTC_BOOKSP              0x0017

#define MTC_SORTINGASK          0x0020
#define MTC_SORTINGBID          0x0021

#define MTC_AUACCOUNTING        0x0030
#define MTC_EPACCOUNTING        0x0031
#define MTC_PBACCOUNTING        0x0032

#define MTC_AUASSETEXPOSURE     0x0070
#define MTC_EPASSETEXPOSURE     0x0071
#define MTC_PBASSETEXPOSURE     0x0072
#define MTC_AUSECURITYEXPOSURE  0x0073
#define MTC_EPSECURITYEXPOSURE  0x0074
#define MTC_PBSECURITYEXPOSURE  0x0075
#define MTC_AUTOTALEXPOSURE     0x0076
#define MTC_EPTOTALEXPOSURE     0x0077
#define MTC_PBTOTALEXPOSURE     0x0078
#define MTC_AUEXPOSUREwPRICE    0x0079
#define MTC_EPEXPOSUREwPRICE    0x007A
#define MTC_PBEXPOSUREwPRICE    0x007B

#define MTC_GAUASSETEXPOSURE    0x0080
#define MTC_GPBASSETEXPOSURE    0x0081
#define MTC_GEPASSETEXPOSURE    0x0082
#define MTC_GAUSECURITYEXPOSURE 0x0083
#define MTC_GEPSECURITYEXPOSURE 0x0084
#define MTC_GPBSECURITYEXPOSURE 0x0085
#define MTC_GAUTOTALEXPOSURE    0x0086
#define MTC_GPBTOTALEXPOSURE    0x0087
#define MTC_GEPTOTALEXPOSURE    0x0088

#define MTC_AURSVDASSETEXPOSURE 0x0090
#define MTC_EPRSVDASSETEXPOSURE 0x0091
#define MTC_PBRSVDASSETEXPOSURE 0x0092
#define MTC_AURSVTOTALEXPOSURE  0x0093
#define MTC_EPRSVTOTALEXPOSURE  0x0094
#define MTC_PBRSVTOTALEXPOSURE  0x0095

#define MTC_EPSHAREDMARGIN      0x0042

#define MTC_TRADE_ADD           0x0050
#define MTC_TRADE_REJECTED      0x0051
#define MTC_TRADE_INFO          0x0052
#define MTC_TRADE_STATUS        0x0053
#define MTC_TRADE_DELETE        0x0054
#define MTC_TRADE_MODIFY_REJ    0x0055
#define MTC_TRADE_CANCEL_REJ    0x0056
#define MTC_TRADE_CHANGEID      0x0057
#define MTC_TRADE_TESTTIME      0x0058


#define MTC_ALARM_INFO          0x0060
#define MTC_STATUS_INFO_TI      0x0061


/* Megatick elements structs */

/**
 * @struct _mtHeader_t_
 * @brief   Network message header
 */
typedef struct _mtHeader_t_
{
    uint16  code;       /**< Fixed code to identify type of message */
    uint32  sequence;   /**< Sequence number of current megatick */
    idtype  id;         /**< Associated identifier to the code */
    idtype  idToIndex;  /**< Identifier to the affected element in the array */
    uint16  length;     /**< Size in bytes of megatick payload */
}
mtHeader_t;

/**
 * @struct _mt_t_
 * @brief  Network message element including header and payload
 */
typedef struct _mt_t_
{
    mtHeader_t header;                /**< Header element */
    byte       payload[MAXTICKSIZE];  /**< Payload of the message */
}
mt_t;

/**
 * @struct _mtSecurityPrice_t_
 * @brief   Security price element at payload
 */
typedef struct _mtSecurityPrice_t_
{
    number     price; /**< Price value */
    timetype_t timet; /**< Timestamp */
}
mtSecurityPrice_t;

/**
 * @struct _mtAssetPrice_t_
 * @brief   Asset price element at payload
 */
typedef struct _mtAssetPrice_t_
{
    number     price; /**< Price value */
    timetype_t timet; /**< Timestamp */
}
mtAssetPrice_t;

/**
 * @struct _mtAccounting_t_
 * @brief  Total equity for this accounting
 */
typedef struct _mtAccounting_t_
{
    number      equity;
    number      usedMargin;
    number      freeMargin;
    number      reservedMargin;
    number      reservedFreeMargin;
    number      globalEquity;
}
mtAccounting_t;

/**
 * @struct _mtAssetExposure_t_
 * @brief  Aggregated exposures from securities and assets
 */
typedef struct _mtAssetExposure_t_
{
    idtype      baseAsset;     /**< Base asset for this pair exposure */
    idtype      termAsset;     /**< Term asset for this pair exposure */
    number      exposureBase;  /**< Base exposure value */
    number      exposureTerm;  /**< Term exposure value */
    char        tradeId[MAX_TRADE_ID_STRING_SIZE]; /* tradeId related to this exposure */
}
mtAssetExposure_t;

/**
 * @struct _mtSecurityExposure_t_
 * @brief  Aggregated exposures from securities and assets
 */
typedef struct _mtSecurityExposure_t_
{
    idtype      security; /**< Security affected identifier */
    number      exposure; /**< Exposure amount */
    number      price;  /**< Exposure price */
    char        tradeId[MAX_TRADE_ID_STRING_SIZE]; /* tradeId related to this exposure */
}
mtSecurityExposure_t;

/**
 * @struct _mtTrade_t_
 * @brief  Trade list element (check tradeCommand_t at trade.h or tradeBridge.h)
 */
typedef struct _mtTrade_t_
{
    // Next is exactly the same than tradeCommant_t at begining... test flags not included for user
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

    /* test flags for tick to trade measure */
    boolean       testFlag;        // flag for testing trade to support time measures
    timetype_t    testFixTime;     // time asociated to fix incoming price for test purposes
    uint32        testProfile[TEST_TICKTOTRADE_TIME_MESAURES];
}
mtTrade_t;

/**
 * @struct mtSimulation_t
 * @brief  Simulation trade matrix element
 */
typedef struct _mtSimulation_t_
{
    number      M5;
    number      M6;
    number      reservedM6;
    idtype      asset; /**< Affected asset code */
}
mtSimulation_t;

typedef struct _mtShared_t_
{
    number      ep;    /**< Equity pool accumulation */
    number      reserved;
}
mtShared_t;


/**
 * @struct _mtSorting_t_
 * @brief  Sorting price element
 */
typedef struct _mtSorting_t_
{
    idtype  tiIndex; /**< Trading interface index in this book */
}
mtSorting_t;

/**
 * @struct _bookElement_t_
 * @brief   Minimal element for each book entry composed by price, liquidity and timestamp
 */
typedef struct _mtbookElement_t_
{
    uint32     price;         /**< Price as integer multiplied by decimal positions (as incoming) */
    uint32     liquidity;     /**< Liquidity for this price value (integer multiplied as incoming */
    uint32     entryIndex;    /**< Identifier for this price entry, needed for allocate at quotes list in full desaggregated */
    uint32     providerIndex; /**< Provider identifier, for disaggregated full books */
    uint32     replaces;      /**< Number of replaces in this quote */
    timetype_t timestamp;     /**< First time stamp at fix input */
    char       fixTime[MAX_FIX_TIME_STRING_SIZE]; /**< Time stamp received (as text) at fix message */
    uint32     profile[MAX_PROFILING_LEVELS_MT]; /**< Profiling information in microseconds at different points */
}
mtbookElement_t;


/**
 * @struct _mtQuoteChainInfo_t_
 * @brief   Quote mchains information when a new chain starts or changes or finish
 */
typedef struct _mtQuoteChainInfo_t_
{
    uint32  newEntryIndex;
    uint32  origEntryIndex;
    boolean new;
    boolean finish;
    uint8   side;
    uint8   reserved;
}
mtQuoteChainInfo_t;

/**
 * @struct _mtAlarm_t_
 * @brief   Alarm information distributed by megatick (only alarms needed at remote stubs
 */
typedef struct _mtAlarm_t_
{
    idtype     code;              /**< Alarm code */
    uint16     dataSize;          /**< Size in number of bytes included before, for auxiliary information */
    byte       data[MAXTICKSIZE]; /**< Data content depending on each code */
}
mtAlarm_t;

/**
 * @struct _mtStatusTi_t_
 * @brief   Trading interface status
 */
typedef struct _mtStatusTi_t_
{
    idtype     tiId;      /**< Alarm code */
    boolean    connected; /**< Connection status of this trading interface */
}
mtStatusTi_t;

/**
 * @struct _mtTradeTest_t_
 * @brief   Tick to trade measures struct
 */
typedef struct _mtTradeTest_t_
{
    uint32     time;
    uint32     profile[TEST_TICKTOTRADE_TIME_MESAURES];      /**< measures */
}
mtTradeTest_t;

/**
 * @struct _megatick_t_
 * @brief  Basic megatick message
 */
typedef struct _megatick_t_
{
    idtype bookId;                  /**< Associated book identifier for this megatick */
    uint16 tickposition;            /**< Current position in megaticks array */
    uint16 mtcrc;                   /**< Current crc to be accumulated for very send */
    uint32 mtsequence;              /**< Current sequence number to be send */
    mt_t   mtick[MAXNUMTICK];       /**< Megatick array */
    byte   mtickSend[MAXMTLENGHT];  /**< Output buffer of several megaticks to be send */

}
megatick_t;

#endif /* MEGATICK_H_ */
