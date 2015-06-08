/*
 * strategy_parameters.h
 *
 *  Created on: May 26, 2014
 *      Author: julio
 */

#ifndef STRATEGY_PARAMETERS_H_
#define STRATEGY_PARAMETERS_H_


extern void init_strategy_parameters(void);

// Add here specific strategy parameters
// Use #define clauses to refer to specific parameters
// These tags need to be consistent with init_strategy_parameters() as defined in strategy_parameters.c !!!!

// example:
// #define my_first_parameter strategy_params[0].value
// #define my_second_parameter strategy_params[1].value
// And so on

#define STR_MAX_ORDERS_ALIVE strategy_params[0].value
#define STR_TOTAL_ORDERS_TO_SEND strategy_params[1].value
#define STR_1_IN_N_TO_CANCEL_LIMIT strategy_params[2].value
#define STR_1_IN_N_TO_REPLACE_TO_MARKET_PRICE strategy_params[3].value
#define STR_1_IN_N_TO_CANCEL_RANDOMLY strategy_params[4].value
#define EVERY_NTH_MILISECOND_TO_TRADE strategy_params[5].value
#define STR_TIMEOUT_FOR_TRADE_RESOLUTION strategy_params[6].value

// Done


#endif /* STRATEGY_PARAMETERS_H_ */
