/*--------------------------------------------------------------------------------------------------
-- Project     :    CoreLib
-- Filename    :    name: security.h
--                  created_by: carlos
--                  date_created: Mar 8, 2013
--------------------------------------------------------------------------------------------------
-- File Purpose: Security list
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


#ifndef SECURITY_H_
#define SECURITY_H_

/* Security fixed code numbers (currently based on currenex) */

#define AUD_CAD    1
#define AUD_CHF    2
#define AUD_JPY    3
#define AUD_NZD    4
#define AUD_SGD    5
#define AUD_USD    6
#define CAD_CHF    7
#define CAD_JPY    8
#define CAD_SGD    9
#define CHF_JPY   10
#define CHF_NOK   11
#define CHF_PLN   12
#define CHF_SEK   13
#define CHF_SGD   14
#define CZK_JPY   15
#define EUR_AED   16
#define EUR_AUD   17
#define EUR_BHD   18
#define EUR_CAD   19
#define EUR_CHF   20
#define EUR_CZK   21
#define EUR_DKK   22
#define EUR_GBP   23
#define EUR_HKD   24
#define EUR_HUF   25
#define EUR_ILS   26
#define EUR_INR   27
#define EUR_JPY   28
#define EUR_KWD   29
#define EUR_MXN   30
#define EUR_NOK   31
#define EUR_NZD   32
#define EUR_OMR   33
#define EUR_PLN   34
#define EUR_SAR   35
#define EUR_SEK   36
#define EUR_SGD   37
#define EUR_SKK   38
#define EUR_THB   39
#define EUR_TRY   40
#define EUR_USD   41
#define EUR_ZAR   42
#define GBP_AUD   43
#define GBP_CAD   44
#define GBP_CHF   45
#define GBP_DKK   46
#define GBP_HKD   47
#define GBP_HUF   48
#define GBP_ILS   49
#define GBP_JPY   50
#define GBP_NOK   51
#define GBP_NZD   52
#define GBP_PLN   53
#define GBP_SEK   54
#define GBP_SGD   55
#define GBP_USD   56
#define HKD_JPY   57
#define HUF_JPY   58
#define MXN_JPY   59
#define NOK_JPY   60
#define NOK_SEK   61
#define NZD_CAD   62
#define NZD_CHF   63
#define NZD_DKK   64
#define NZD_JPY   65
#define NZD_SEK   66
#define NZD_SGD   67
#define NZD_USD   68
#define PLN_JPY   69
#define SEK_JPY   70
#define SGD_JPY   71
#define TRY_JPY   72
#define USD_AED   73
#define USD_BHD   74
#define USD_CAD   75
#define USD_CHF   76
#define USD_CLP   77
#define USD_CZK   78
#define USD_DKK   79
#define USD_HKD   80
#define USD_HUF   81
#define USD_IDR   82
#define USD_ILS   83
#define USD_INR   84
#define USD_JPY   85
#define USD_KWD   86
#define USD_MXN   87
#define USD_NOK   88
#define USD_OMR   89
#define USD_PLN   90
#define USD_RUB   91
#define USD_SAR   92
#define USD_SEK   93
#define USD_SGD   94
#define USD_SKK   95
#define USD_THB   96
#define USD_TRY   97
#define USD_ZAR   98
#define XAG_EUR   99
#define XAG_JPY  100
#define XAG_USD  101
#define XAU_EUR  102
#define XAU_USD  103
#define XAU_JPY  104
#define ZAR_JPY  105

#define NUM_SECURITIES 106 /**< Total number of defined securities */



/* STRUCTURES */

/**
 * @struct _secType_t_
 * @brief Security structure containing base and term asset
 */
typedef struct _secType_t_
{
    idtype id;
    idtype base;
    idtype term;
}
secType_t;


/**
 * @struct _atosElement_t_
 * @brief   Minimal element for each ATOS entry composed by security, number of TIs and flag
 */
typedef struct _atosElement_t_
{
    idtype  security;           /**< Security from base asset in row and term asset in column */
    uint32  numberOftradingIfs; /**< Security trading interfaces available */
    boolean    flag;               /**< Invert flag*/
}
atosElement_t;


#endif /* SECURITY_H_ */
