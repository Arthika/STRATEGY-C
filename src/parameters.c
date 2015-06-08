/*--------------------------------------------------------------------------------------------------
-- Project     :    remoteStub
-- Filename    :    name: strathread.c
--                  created_by: carlos
--                  date_created: Jul 2, 2013
--------------------------------------------------------------------------------------------------
-- File Purpose: Strategy
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
-- TODO
--
--------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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
#include "parameters.h"
#include "monitoring.h"
#include "strategy_parameters.h"
#include "main.h"

params_t system_params[MAX_PARAMS];
int many_system_params = 0;

params_t strategy_params[MAX_PARAMS];
int many_strategy_params = 0;

// Change this to define system parameters!!

void init_system_parameters(void) {

    many_system_params = 0; // To reset the count

    // Add here parameters
    // Remember to update the #define tags in parameters.h

    add_system_parameter("valuation_chage_threshold", 5000); // This is valuation change threshold upon which the strategy will disconnect itself
    add_system_parameter("disconnect_on_valuation_change_too_big", 0); // 1 = Strategy will disconnected if valuation change larger than threshold, 0 otherwise
    add_system_parameter("drawdown_threshold", 500); // This is drawdown threshold upon which the strategy will disconnect itself
    add_system_parameter("disconnect_on_drawdown_too_big", 1); // 1 = Strategy will disconnected if drawdown excessive, 0 otherwise
    add_system_parameter("too_many_trades_threshold_1_sec", 25); // This is the maximum number of trades in a given second before the strategy disconnects itself
    add_system_parameter("too_many_trades_threshold_10_sec", 150); // This is the maximum number of trades in a given 10 seconds period before the strategy disconnects itself
    add_system_parameter("too_many_trades_threshold_1_min", 500); // This is the maximum number of trades in a given 1 minute period before the strategy disconnects itself
    add_system_parameter("disconnect_on_too_many_trades", 0); // 1 = Strategy will disconnected if too many trades, 0 otherwise
    add_system_parameter("minimum_order_size", 50000); // Minimum order size in base curr to reject trade
    add_system_parameter("maximum_order_size", 500000); // Maximum order size in base curr to reject trade
    add_system_parameter("maximum_hedge_size", 250000); // Maximum order size in base curr to reject trade
    add_system_parameter("min_trade_step", 10000); // Trade step to round odd size trades
    add_system_parameter("price_obsolescence_threshold", 10); // Number of seconds to consider a price as obsolete
    add_system_parameter("connect_profiling", 1); // 1 = profiling will be done and reported
    add_system_parameter("n_samples_for_profiling", MAX_PROFILING_SAMPLES); // Number of samples for profiling (max is
    add_system_parameter("security_for_profiling", 41); // Security to profile ticks (default is EUR_USD)
    add_system_parameter("n_failed_immediate_limits_in_a_row_for_yellow_card", 3);
    add_system_parameter("n_rejects_in_a_row_for_yellow_card", 3);
    add_system_parameter("yellow_card_putout_time", 300);
    add_system_parameter("n_yellow_cards_for_red_card", 3);
    add_system_parameter("connect_yellow_cards", 0);
    add_system_parameter("n_successful_immediate_limits_to_remove_yellow_card", 3);
    add_system_parameter("n_successful_markets_to_remove_yellow_card", 3);
    add_system_parameter("n_macro_command_failures_to_disconnect", 3);
    add_system_parameter("weekly_start_time_weekday", 0); // Sunday
    add_system_parameter("weekly_start_time_hour", 23);
    add_system_parameter("weekly_start_time_min", 0);
    add_system_parameter("weekly_end_time_weekday", 5); // Friday
    add_system_parameter("weekly_end_time_hour", 18);
    add_system_parameter("weekly_end_time_min", 0);
    add_system_parameter("daily_stop_1_start_hour", 20);
    add_system_parameter("daily_stop_1_start_min", 45);
    add_system_parameter("daily_stop_1_end", 21);
    add_system_parameter("daily_stop_1_end_min", 20);
    add_system_parameter("daily_stop_2_start_hour", 0);
    add_system_parameter("daily_stop_2_start_min", 0);
    add_system_parameter("daily_stop_2_end", 0);
    add_system_parameter("daily_stop_2_end_min", 0);
    add_system_parameter("daily_stop_3_start_hour", 0);
    add_system_parameter("daily_stop_3_start_min", 0);
    add_system_parameter("daily_stop_3_end", 0);
    add_system_parameter("daily_stop_3_end_min", 0);
    add_system_parameter("send_cancel_all_after_operation_period", 1);
    add_system_parameter("send_close_all_after_operation_period", 1);
    add_system_parameter("fixToStubDelayThreshold_ms", 10);
    add_system_parameter("ignoreTIswDelayedPrices", 1);
    add_system_parameter("noTicksThreshold_ms",500);
    add_system_parameter("ignoreTIswNoTicks", 1);
    add_system_parameter("reportDelaysAndNoTicks", 0);

    // Done

}



int add_system_parameter(char *name, int32 def_value) {
    // To do: check that name only contains letters, numbers and underscores - or at least no spaces!!
    if(many_system_params >= MAX_PARAMS) return -1;
    strcpy(system_params[many_system_params].name, name);
    system_params[many_system_params].value = def_value;
    many_system_params++;
    return many_system_params;
}


int add_strategy_parameter(char *name, int32 def_value) {
    // To do: check that name only contains letters, numbers and underscores - or at least no spaces!!
    if(many_strategy_params >= MAX_PARAMS) return -1;
    strcpy(strategy_params[many_strategy_params].name, name);
    strategy_params[many_strategy_params].value = def_value;
    many_strategy_params++;
    return many_strategy_params;
}


int find_system_parameter(char *name) {

    for(int i = 0; i < many_system_params; i++)
        if(!strcmp(name, system_params[i].name)) return i;

    return -1;

}


int find_strategy_parameter(char *name) {

    for(int i = 0; i < many_strategy_params; i++)
        if(!strcmp(name, strategy_params[i].name)) return i;

    return -1;

}


void print_system_parameters(FILE *where) {
    for(int i = 0; i < many_system_params; i++)
        fprintf(where, "%s = %d\n", system_params[i].name, system_params[i].value);
}



void print_strategy_parameters(FILE *where) {
    for(int i = 0; i < many_strategy_params; i++)
        fprintf(where, "%s = %d\n", strategy_params[i].name, strategy_params[i].value);
}


void save_parameters(void) {

    mkdir(PARAMETERS_DIR, 0777);
    char filePath[MLEN];

    sprintf(filePath, "%s/system_parameters_%s_%s.txt", PARAMETERS_DIR, userName, execName);
    FILE *config_system_file=fopen(filePath,"wo");
    print_system_parameters(config_system_file);

    fprintf(config_system_file, "#done!\n");
    fclose(config_system_file);

    sprintf(filePath, "%s/strategy_parameters_%s_%s.txt", PARAMETERS_DIR, userName, execName);
    FILE *config_strategy_file=fopen(filePath,"wo");
    print_strategy_parameters(config_strategy_file);

    fprintf(config_strategy_file, "#done!\n");
    fclose(config_strategy_file);

}


void list_system_parameters(void) {

    stubDbg(DEBUG_INFO_LEVEL, ">> System parameters\n");
    for(int i = 0; i < many_system_params; i++) {
        char thisParam[SLEN];
        sprintf(thisParam, "    %s = %d\n", system_params[i].name, system_params[i].value);
        stubDbg(DEBUG_INFO_LEVEL, thisParam);
    }

}


void list_strategy_parameters(void) {

    stubDbg(DEBUG_INFO_LEVEL, ">> Strategy parameters\n");
    for(int i = 0; i < many_strategy_params; i++) {
        char thisParam[SLEN];
        sprintf(thisParam, "    %s = %d\n", strategy_params[i].name, strategy_params[i].value);
        stubDbg(DEBUG_INFO_LEVEL, thisParam);
    }

}


void load_specific_parameters(params_t *params) {

    FILE *config_file;
    char what[64];
    char filePath[MLEN];

    if(params==system_params) {
        sprintf(filePath, "%s/system_parameters_%s_%s.txt", PARAMETERS_DIR, userName, execName);
        config_file=fopen(filePath,"r");
        sprintf(what, "system");
    } else {
        sprintf(filePath, "%s/strategy_parameters_%s_%s.txt", PARAMETERS_DIR, userName, execName);
        config_file=fopen(filePath,"r");
        sprintf(what, "strategy");
    }
    char errorS[SLEN];

    if(!config_file) {
        stubDbg (DEBUG_INFO_LEVEL, "[Warning] No %s configuration file found - now loading default values\n", what);
        return;
    } else {
        boolean error = false;
        boolean end_of_file = false;

        do {

            char name[SLEN];
            int32 value;

            int n_parse = fscanf(config_file, "%s = %d", name, & value);

            if (n_parse == 2) {
                // This is a parameter
//                stubDbg (DEBUG_INFO_LEVEL, "[Info] (%s) Parameter loaded: %s = %d\n", what, name, value);
                int param;
                if(params==system_params) {
                    param = find_system_parameter(name);
                } else {
                    param = find_strategy_parameter(name);
                }
                if(param < 0) {
                    error = true;
                    sprintf(errorS, "Unrecognized %s parameter: %s", what, name);
                } else {
                    if(params==system_params) {
                        strcpy(system_params[param].name, name);
                        system_params[param].value = value;
                    } else {
                        strcpy(strategy_params[param].name, name);
                        strategy_params[param].value = value;
                    }
                }
            } else {
                end_of_file = true;
            }

        } while(!error && !end_of_file);

        fclose(config_file);

        if(error) {
            char ErrMsg[SLEN];
            sprintf(ErrMsg, "[Error] Wrong %s config file format [%s]- aborting and reverting to default values!\n", what, errorS);

            stubDbg (DEBUG_INFO_LEVEL, ErrMsg);

            if(params==system_params) {
                init_system_parameters();
            } else {
                init_strategy_parameters();
            }

        }
    }

}


void load_parameters(void) {

    init_system_parameters();
    load_specific_parameters(system_params);
    init_strategy_parameters();
    load_specific_parameters(strategy_params);
    save_parameters();

}


