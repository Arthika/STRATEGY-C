/*
 * strategy_parameters.c
 *
 *  Created on: May 26, 2014
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
#include "parameters.h"
#include "strategy_parameters.h"

void init_strategy_parameters(void) {

    many_strategy_params = 0; // To reset the count

    // Add here specific strategy parameters
    // Remember to update the #define tags in strategy_parameters.h

    // example:
    // add_strategy_parameter("my_first_parameter", 23); // Where 23 is the default value
    // add_strategy_parameter("my_second_parameter", 32); // Same
    // And so on

    add_strategy_parameter("MAX ORDERS ALIVE", 30);
    add_strategy_parameter("TOTAL ORDERS TO SEND", 10000);
    add_strategy_parameter("STR_1_IN_N_TO_CANCEL_LIMIT", 8);
    add_strategy_parameter("STR_1_IN_N_TO_REPLACE_TO_MARKET_PRICE", 5);
    add_strategy_parameter("STR_1_IN_N_TO_CANCEL_RANDOMLY", 7);
    add_strategy_parameter("EVERY_NTH_MILISECOND_TO_TRADE", 10);
    add_strategy_parameter("STR_TIMEOUT_FOR_TRADE_RESOLUTION", 1000000);

    // Done


}


