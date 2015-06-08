/*--------------------------------------------------------------------------------------------------
-- Project     :    CoreLib
-- Filename    :    name: assest.h
--                  created_by: carlos
--                  date_created: Mar 8, 2013
--------------------------------------------------------------------------------------------------
-- File Purpose: Assest code defintion... will be the same for all modules
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


#ifndef ASSET_H_
#define ASSET_H_

#include "hftUtils_types.h"

/* CODES FOR ASSETS */

#define AED    1   /**< Emirates Dirham       */
#define AUD    2   /**< Australia Dollar      */
#define BHD    3    /**< Bahrain Dinar        */
#define CAD    4   /**< Canada Dollar         */
#define CHF    5   /**< Switzerland Franc     */
#define CLP    6   /**< Chile Peso            */
#define CZK    7   /**< Czech Republic Koruna */
#define DKK    8   /**< Denmark Krone         */
#define EUR    9   /**< Euro Member Countries */
#define GBP   10   /**< United Kingdom Pound  */
#define HKD   11   /**< Hong Kong Dollar      */
#define HUF   12   /**< Hungary Forint        */
#define IDR   13   /**< Indonesia Rupiah      */
#define ILS   14   /**< Israel Shekel         */
#define INR   15   /**< India Rupee           */
#define JPY   16   /**< Japan Yen             */
#define KWD   17   /**< Kuwaiti Dinar         */
#define MXN   18   /**< Mexico Peso           */
#define NOK   19   /**< Norway Krone          */
#define NZD   20   /**< New Zealand Dollar    */
#define OMR   21   /**< Oman Rial             */
#define PLN   22   /**< Poland Zloty          */
#define RUB   23   /**< Russia Ruble          */
#define SAR   24   /**< Saudi Arabia Riyal    */
#define SEK   25   /**< Sweden Krona          */
#define SGD   26   /**< Singapore Dollar      */
#define SKK   27   /**< Slovak Koruna         */
#define THB   28   /**< Thailand Baht         */
#define TRY   29   /**< Turkey Lira           */
#define USD   30   /**< United States Dollar  */
#define XAG   31   /**< Silver                */
#define XAU   32   /**< Gold                  */
#define ZAR   33   /**< South Africa Rand     */

#define NUM_ASSETS 34 /**< Total number of defined assets (including blank column with index 0)*/


/* STRINGS FOR ASSETS */

#define AED_S  "AED"   /**< Emirates Dirham       */
#define AUD_S  "AUD"   /**< Australia Dollar      */
#define BHD_S  "BHD"   /**< Canada Dollar         */
#define CAD_S  "CAD"   /**< Chile Peso            */
#define CHF_S  "CHF"   /**< Czech Republic Koruna */
#define CLP_S  "CLP"   /**< Denmark Krone         */
#define CZK_S  "CZK"   /**< Euro Member Countries */
#define DKK_S  "DKK"   /**< Hong Kong Dollar      */
#define EUR_S  "EUR"   /**< Hungary Forint        */
#define GBP_S  "GBP"   /**< India Rupee           */
#define HKD_S  "HKD"   /**< Indonesia Rupiah      */
#define HUF_S  "HUF"   /**< Israel Shekel         */
#define IDR_S  "IDR"   /**< Japan Yen             */
#define ILS_S  "ILS"   /**< Mexico Peso           */
#define INR_S  "INR"   /**< New Zealand Dollar    */
#define JPY_S  "JPY"   /**< Norway Krone          */
#define KWD_S  "KWD"   /**< Oman Rial             */
#define MXN_S  "MXN"   /**< Poland Zloty          */
#define NOK_S  "NOK"   /**< Russia Ruble          */
#define NZD_S  "NZD"   /**< Saudi Arabia Riyal    */
#define OMR_S  "OMR"   /**< Singapore Dollar      */
#define PLN_S  "PLN"   /**< South Africa Rand     */
#define RUB_S  "RUB"   /**< Sweden Krona          */
#define SAR_S  "SAR"   /**< Switzerland Franc     */
#define SEK_S  "SEK"   /**< Thailand Baht         */
#define SGD_S  "SGD"   /**< Turkey Lira           */
#define SKK_S  "SKK"   /**< United Kingdom Pound  */
#define THB_S  "THB"   /**< United States Dollar  */
#define TRY_S  "TRY"   /**< Turkey Lira           */
#define USD_S  "USD"   /**< United States Dollar  */
#define XAG_S  "XAG"   /**< Silver                */
#define XAU_S  "XAU"   /**< Gold                  */
#define ZAR_S  "ZAR"   /**< South Africa Rand     */



/**** STRUCTURES ***/

/**
 * @struct _assetType_t_
 * @brief Asset structure containing each code and string formed by 3 chars
 */
typedef struct _assetType_t_
{
    idtype id;
    char   name[4];
}
assetType_t;

#endif /* ASSET_H_ */
