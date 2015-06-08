/*
 * macro_commands.h
 *
 *  Created on: May 21, 2014
 *      Author: julio
 */

#ifndef MACRO_COMMANDS_H_
#define MACRO_COMMANDS_H_

//
// Command codes (possible values for macro_command_type global variable):
//

// "no command" code:
#define MACRO_COMMAND_NO_COMMAND                 0

// Codes for trading functions:
#define MACRO_COMMAND_HEDGE_SECURITY_AU          1
#define MACRO_COMMAND_HEDGE_ALL_SECURITIES_AU    2
#define MACRO_COMMAND_HEDGE_SECURITY_EP          3
#define MACRO_COMMAND_HEDGE_ALL_SECURITIES_EP    4
#define MACRO_COMMAND_CANCEL_ALL_PENDING_IN_AU   5
#define MACRO_COMMAND_CLOSE_ALL_POSIS_IN_AU      6

// Codes for backoffice functions: (not to use from real strategies!!!)
#define MACRO_COMMAND_WIPE_PB_FROM_AU            16

//
// Done
//


// Potential status values for macro_command_status global variable:
#define MACRO_COMMAND_STATUS_NO_STATUS           0
#define MACRO_COMMAND_STATUS_IN_PROGRESS         1
#define MACRO_COMMAND_STATUS_SUCCEEDED           2
#define MACRO_COMMAND_STATUS_FAILED              3


// Reserved data codes for macro commands
// Codes range between 0xFFFFFFF0 and 0xFFFFFFFF are used by the macro commands processor to identify trades involved in macro commands
// Therefore, this value should NOT be used by user strategies when sending trades!!!!
// Immediate limit orders with codes within this range will be monitored (yellow cards will apply).
#define MACRO_COMMAND_RESERVED_DATA_MIN_RANGE          0xFFFFFFF0
#define MACRO_COMMAND_RESERVED_DATA_MAX_RANGE          0xFFFFFFFF

#define MACRO_COMMAND_MARKET_HEDGING_RESERVED_DATA_ID  0xFFFFFFFF
#define MACRO_COMMAND_LIMIT_HEDGING_RESERVED_DATA_ID   0xFFFFFFFE
#define MACRO_COMMAND_CLOSE_ALL_RESERVED_DATA_ID       0xFFFFFFFD


// Other internal defines:
#define MACRO_COMMAND_TIMEOUT_THRESHOLD            10000000
#define MACRO_COMMAND_TOO_MANY_CANCEL_TRIES        50
#define MACRO_COMMAND_MAX_WAIT_TIME_FOR_QUOTES     1000000
#define MACRO_COMMAND_MAX_WAIT_TIME_FOR_CANCELS    300000


// Data architecture to mark non-tradeable quotes (or quotes with last look provisions that failed)
typedef struct _quoteTradingHistory_t_
{
//    timetype_t initialQuoteTimestamp;                        // Timestamp of the initial quote (before the first replace) - proxy for liq provider
    timetype_t currentQuoteTimestamp;                        // Timestamp of the current replace to be saved in the moment of an trade
//    int        tradeAttempts;                                // Counter for number of trade attempts on this quote stream
//    int        failedAttempts;                               // Counter of failed attempts on this quote stream
    boolean    currentQuoteFailed;                           // to be set when a trade on the current quote (with the current quote timestamp) fails
}
quoteTradingHistory_t;

// And these are the quotes quality data repositories:
extern quoteTradingHistory_t quotesTradingHistoryAsk[MAX_NUMBER_TI][NUM_SECURITIES][MAX_QUOTES];
extern quoteTradingHistory_t quotesTradingHistoryBid[MAX_NUMBER_TI][NUM_SECURITIES][MAX_QUOTES];



// API:

/*
 * To use macro commands:
 *   - Call the appropriate function (one of the below)
 *   - The function returns false if a previous command is already running - only one macro command can be in progress at a time
 *   - The function returns true if the command is successfully launched, sets macro_command_type to the appropriate value,
 *     sets macro_command_in_progress to true, and sets macro_command_status to MACRO_COMMAND_STATUS_IN_PROGRESS
 *   - Once the command is finished, macro_command_in_progress is set to false, macro_command_status is set to
 *     MACRO_COMMAND_STATUS_SUCCEEDED (if the command succeeded) or MACRO_COMMAND_STATUS_FAILED (otherwise), and an output
 *     message is written in macro_command_output_msg
 *   - Typically several callbacks (megaticks) are needed for a macro command to complete. However, it is possible that the
 *     command is completed (successfully or not) right after the call to the macro command function
 */

extern boolean macro_command_in_progress;
extern int macro_command_type;
extern char macro_command_output_msg[MLEN];
extern int macro_command_status;

// Trading functions
extern boolean macro_command_hedge_security_in_AU(sharedMemory_t* memMap, idtype security, int32 finalIntendedOffset, boolean useMarketOrders, int32 additionalLeewayToLimit);
extern boolean macro_command_hedge_all_securities_in_AU(sharedMemory_t* memMap);
extern boolean macro_command_hedge_security_in_EP(sharedMemory_t* memMap, idtype security, int32 finalIntendedOffset, boolean useMarketOrders, int32 additionalLeewayToLimit);
extern boolean macro_command_hedge_all_securities_in_EP(sharedMemory_t* memMap);
extern boolean macro_command_cancel_all_pending_orders_in_AU (sharedMemory_t* memMap);
extern boolean macro_command_close_all_posis_in_AU (sharedMemory_t* memMap);

// Backoffice manipulation functions (seriously!! don't use this!!)
extern boolean macro_command_wipe_PB_from_AU(sharedMemory_t* memMap, idtype pbIndex);



//This last function is only to be (automatically) called from the strategy callback - just to process macro commands
extern boolean process_macro_commands(sharedMemory_t* memMap);

// This function to initialize the whole thing - typically to be called from strategyInit()
extern void init_macro_commands(void);


#endif /* MACRO_COMMANDS_H_ */
