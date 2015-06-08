
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

#ifndef TI_MARKS_H_
#define TI_MARKS_H_

#include "hftUtils_types.h"
#include "map.h"

extern boolean goodForMarketExecution[MAX_NUMBER_TI];
extern boolean goodForLiquidityOffering[MAX_NUMBER_TI];
extern boolean goodForHedging[MAX_NUMBER_TI];

extern void print_ti_marks(sharedMemory_t* memMap, FILE *where);
extern void save_ti_marks(sharedMemory_t* memMap);
extern void load_ti_marks(sharedMemory_t* memMap);


#endif /* TI_MARKS_H_ */
