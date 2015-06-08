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
#include "ti_marks.h"
#include "main.h"

boolean goodForMarketExecution[MAX_NUMBER_TI];
boolean goodForLiquidityOffering[MAX_NUMBER_TI];
boolean goodForHedging[MAX_NUMBER_TI];

#define GOOD_FOR_MARKET_EXECUTION_CODE 1
#define GOOD_FOR_LIQUIDITY_OFFERING_CODE 2
#define GOOD_FOR_HEDGING_CODE 3

void print_ti_marks(sharedMemory_t* memMap, FILE *where) {

    for(int tiIndex=0; tiIndex<memMap->nTradingInterfaces; tiIndex++) {
        fprintf(where,"%d:%d:%s\n", GOOD_FOR_MARKET_EXECUTION_CODE, memMap->tradingInterface[tiIndex], (goodForMarketExecution[tiIndex] ? "true" : "false"));
        fprintf(where,"%d:%d:%s\n", GOOD_FOR_LIQUIDITY_OFFERING_CODE, memMap->tradingInterface[tiIndex], (goodForLiquidityOffering[tiIndex] ? "true" : "false"));
        fprintf(where,"%d:%d:%s\n", GOOD_FOR_HEDGING_CODE, memMap->tradingInterface[tiIndex], (goodForHedging[tiIndex] ? "true" : "false"));
    }

}


void save_ti_marks(sharedMemory_t* memMap) {

    mkdir(PARAMETERS_DIR, 0777);
    char filePath[MLEN];

    sprintf(filePath, "%s/ti_marks_%s_%s.txt", PARAMETERS_DIR, userName, execName);

    FILE *ti_marks_file=fopen(filePath,"wo");
    print_ti_marks(memMap, ti_marks_file);

    fprintf(ti_marks_file, "#done!\n");
    fclose(ti_marks_file);

}


void initialize_ti_marks(sharedMemory_t* memMap) {

    for(int tiIndex=0; tiIndex<memMap->nTradingInterfaces; tiIndex++) {
        goodForMarketExecution[tiIndex]=true;
        goodForLiquidityOffering[tiIndex]=true;
        goodForHedging[tiIndex]=true;
    }

}


void load_ti_marks(sharedMemory_t* memMap) {

    initialize_ti_marks(memMap);
    char filePath[MLEN];
    sprintf(filePath, "%s/ti_marks_%s_%s.txt", PARAMETERS_DIR, userName, execName);

    FILE *config_file;
    config_file=fopen(filePath,"r");

    if(!config_file) {

        stubDbg (DEBUG_INFO_LEVEL, "[Warning] No ti_marks file found - now loading default values (i.e. everything disabled)\n");


    } else {

        boolean error = false;
        boolean end_of_file = false;

        do {

            int what;
            int tiId;
            char valueStr[SLEN];

            int n_parse = fscanf(config_file, "%d:%d:%s", &what, &tiId, valueStr);

            if (n_parse == 3) {
                // This is a ti_mark

                boolean value;
                if(!strcmp(valueStr, "true"))
                    value = true;
                else if(!strcmp(valueStr, "false"))
                    value = false;
                else {
                    error = true;
                    break;
                }

                int tiIndex=-1;
                for(int t=0; t<memMap->nTradingInterfaces; t++) {
                    if(memMap->tradingInterface[t] == tiId) {
                        tiIndex = t;
                        break;
                    }
                }

                if(tiIndex<0) {
                    error=true;
                    break;
                }

                switch(what) {
                    case GOOD_FOR_MARKET_EXECUTION_CODE :
                        goodForMarketExecution[tiIndex] = value;
                        break;
                    case GOOD_FOR_LIQUIDITY_OFFERING_CODE :
                        goodForLiquidityOffering[tiIndex] = value;
                        break;
                    case GOOD_FOR_HEDGING_CODE :
                        goodForHedging[tiIndex] = value;
                        break;
                    default :
                        error=true;
                }

            } else {

                end_of_file = true;

            }

        } while(!error && !end_of_file);

        fclose(config_file);

        if(error) {

            stubDbg (DEBUG_INFO_LEVEL, "[Warning] Error found in ti_marks file found - now loading default values (i.e. everything disabled)\n");
            initialize_ti_marks(memMap);

        }

    } // config_file was opened

    save_ti_marks(memMap);

}


