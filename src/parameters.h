
/*--------------------------------------------------------------------------------------------------
-- Project     :    remoteStubJF
-- Filename    :    name: strategy.h
--                  created_by: carlos
--                  date_created: Nov 5, 2013
--------------------------------------------------------------------------------------------------
-- File Purpose: Extern variables defined at strategy.c
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

#ifndef PARAMETERS_H_
#define PARAMETERS_H_

#define MAX_PARAMS 100

#define PARAMETERS_DIR "PARAMETERS"

typedef struct {
    char name[SLEN];
    int32 value;
} params_t ;

extern params_t system_params[MAX_PARAMS];
extern int many_system_params;

extern params_t strategy_params[MAX_PARAMS];
extern int many_strategy_params;

extern int add_system_parameter(char *name, int32 def_value);
extern int add_strategy_parameter(char *name, int32 def_value);
extern int find_system_parameter(char *name);
extern int find_strategy_parameter(char *name);
extern void print_system_parameters(FILE *where);
extern void print_strategy_parameters(FILE *where);
extern void save_parameters(void);
extern void load_parameters(void);
extern void list_system_parameters(void);
extern void list_strategy_parameters(void);


extern void init_system_parameters(void);

// These tags need to be consistent with init_parameters() as defined in parameters.c !!!!

#define valuation_change_threshold system_params[0].value
#define disconnect_on_valuation_change_too_big system_params[1].value
#define drawdown_threshold system_params[2].value
#define disconnect_on_drawdown_too_big system_params[3].value
#define too_many_trades_threshold_1_sec system_params[4].value
#define too_many_trades_threshold_10_sec system_params[5].value
#define too_many_trades_threshold_1_min system_params[6].value
#define disconnect_on_too_many_trades system_params[7].value
#define minimum_order_size system_params[8].value
#define maximum_order_size system_params[9].value
#define maximum_hedge_size system_params[10].value
#define min_trade_step system_params[11].value
#define price_obsolescence_threshold system_params[12].value
#define connect_profiling system_params[13].value
#define n_samples_for_profiling system_params[14].value
#define security_for_profiling system_params[15].value
#define n_failed_immediate_limits_in_a_row_for_yellow_card system_params[16].value
#define n_rejects_in_a_row_for_yellow_card system_params[17].value
#define yellow_card_putout_time system_params[18].value
#define n_yellow_cards_for_red_card system_params[19].value
#define connect_yellow_cards system_params[20].value
#define n_successful_immediate_limits_to_remove_yellow_card system_params[21].value
#define n_successful_markets_to_remove_yellow_card system_params[22].value
#define n_macro_command_failures_to_disconnect system_params[23].value
#define weekly_start_time_weekday system_params[24].value // 0 is Sunday, 6 is Saturday
#define weekly_start_time_hour system_params[25].value
#define weekly_start_time_min system_params[26].value
#define weekly_end_time_weekday system_params[27].value // 0 is Sunday, 6 is Saturday
#define weekly_end_time_hour system_params[28].value
#define weekly_end_time_min system_params[29].value
#define daily_stop_1_start_hour system_params[30].value
#define daily_stop_1_start_min system_params[31].value
#define daily_stop_1_end_hour system_params[32].value
#define daily_stop_1_end_min system_params[33].value
#define daily_stop_2_start_hour system_params[34].value
#define daily_stop_2_start_min system_params[35].value
#define daily_stop_2_end_hour system_params[36].value
#define daily_stop_2_end_min system_params[37].value
#define daily_stop_3_start_hour system_params[38].value
#define daily_stop_3_start_min system_params[39].value
#define daily_stop_3_end_hour system_params[40].value
#define daily_stop_3_end_min system_params[41].value
#define send_cancel_all_after_operation_period system_params[42].value
#define send_close_all_after_operation_period system_params[43].value
#define fixToStubDelayThreshold_ms system_params[44].value
#define ignoreTIswDelayedPrices strategy_params[45].value
#define noTicksThreshold_ms system_params[46].value
#define ignoreTIswNoTicks strategy_params[47].value
#define reportDelaysAndNoTicks strategy_params[48].value

// Done


#endif /* PARAMETERS_H_ */
