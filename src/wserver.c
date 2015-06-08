/*--------------------------------------------------------------------------------------------------
-- Project     :    remoteStub
-- Filename    :    name: wserver.c
--                  created_by: carlos
--                  date_created: Sep 24, 2013
--------------------------------------------------------------------------------------------------
-- File Purpose: TODO
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


#include <sys/types.h>
#ifndef _WIN32
#include <sys/select.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif
#include <string.h>
#include <microhttpd.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>


#include "platform.h"
#include "microhttpd.h"
#include "myUtil.h"
#include "stubDbg.h"
#include "asset.h"
#include "map.h"
#include "megatick.h"
#include "trade.h"
#include "util.h"
#include "wserver.h"
#include "strategy.h"
#include "monitoring.h"
#include "macro_commands.h"
#include "parameters.h"
#include "strategy_parameters.h"
#include "ti_marks.h"
#include "changeContext.h"
#include "main.h"

char _WEB_VERSION[10] = "v0.006";

struct post_data_t
{
    char key[MAXKEYDATASIZE];
    char value[MAXKEYDATASIZE];
};

static struct post_data_t post_data[MAXKEYS];
static int post_data_index;

struct connection_info_struct
{
    int connectiontype;
    char *pageString;
    struct MHD_PostProcessor *postprocessor;
};


static int iterate_post (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
                            const char *filename, const char *content_type,
                            const char *transfer_encoding, const char *data, uint64_t off,
                            size_t size)
{
    if(post_data_index >= MAXKEYS ) return MHD_NO;

    if(0 != strcmp(key, "")) {

        strcpy(post_data[post_data_index].key, key);
        strcpy(post_data[post_data_index].value, data);
        post_data_index++;
        return MHD_YES;

    } else {

        return MHD_NO;

    }
}

static void request_completed (void *cls, struct MHD_Connection *connection,
                    void **con_cls, enum MHD_RequestTerminationCode toe)
{
    struct connection_info_struct *con_info = *con_cls;

    if (NULL == con_info)
        return;

    if (con_info->connectiontype == POST)
    {
        MHD_destroy_post_processor (con_info->postprocessor);

        if (con_info->pageString)
            free (con_info->pageString);
    }

    free (con_info);
    *con_cls = NULL;
}



/***************************************************************************************************
* LOCAL DEFINES AND TYPES                                                                          *
***************************************************************************************************/


/***************************************************************************************************
* STATIC DATA                                                                                      *
***************************************************************************************************/

static struct MHD_Daemon *daemonWS = NULL;


int currentCurrency=EUR_USD; // This is for reporting purposes only

typedef struct _authentication_t_
{
    time_t                timestamp;                        // Timestamp of authentication of current connection (seconds)
    void                  *tls_session;                     // Authenticated session
    char                  sa_data[14];                      // Socket address data
}
authentication_t;

#define MAX_AUTH_CONNECTIONS 64
authentication_t auth_data[MAX_AUTH_CONNECTIONS];


/***************************************************************************************************
* STATIC PROTOTYPES                                                                                *
***************************************************************************************************/

static int answer_to_connection (void *cls, struct MHD_Connection *connection,
                                  const char *url, const char *method,
                                  const char *version, const char *upload_data,
                                  size_t *upload_data_size, void **con_cls);


/***************************************************************************************************
* DEFINITIONS                                                                                      *
***************************************************************************************************/

int32 webServerStart (uint16 portNumber)
{
    int32 result = OK;
    uint16 port = 0;


//    daemonWS = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
//                               &answer_to_connection, NULL, MHD_OPTION_END);

    if (portNumber > 0)
    {
        port = portNumber;
    }
    else
    {
        port = PORT;
    }

    daemonWS = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, port, NULL, NULL,
                                &answer_to_connection, NULL,
                                MHD_OPTION_NOTIFY_COMPLETED, request_completed,
                                NULL, MHD_OPTION_END);

    return (result);
}


int32 webServerStop (void)
{
    int32 result = OK;

    MHD_stop_daemon (daemonWS);

    return (result);
}




/***************************************************************************************************
* STATIC DEFINITIONS                                                                               *
***************************************************************************************************/

static int answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{


    // First of all we get keys and values from POST requests

    if (NULL == *con_cls)
    {

        post_data_index = 0;
        struct connection_info_struct *con_info;
        con_info = malloc (sizeof (struct connection_info_struct));

        if (NULL == con_info)
            return MHD_NO;

        con_info->pageString = NULL;

        if (0 == strcmp (method, "POST"))
        {
            con_info->postprocessor = MHD_create_post_processor (connection, POSTBUFFERSIZE,
                                                                 iterate_post, (void *) con_info);
            if (NULL == con_info->postprocessor)
            {
                free (con_info);
                return MHD_NO;
            }

            con_info->connectiontype = POST;
        }
        else
            con_info->connectiontype = GET;

        *con_cls = (void *) con_info;

        return MHD_YES;
    }

    struct connection_info_struct *con_info = *con_cls;

    if (0 == strcmp (method, "GET"))
    {
        // Do nothing
    }

    if (0 == strcmp (method, "POST"))
    {

        if (*upload_data_size != 0)
        {
            MHD_post_process (con_info->postprocessor, upload_data, *upload_data_size);
            *upload_data_size = 0;

            return MHD_YES;
        }
        else if (NULL != con_info->pageString)
        {
            // Do Nothing
        }

    }



  struct MHD_Response *response;
  int ret;

  /*
   int32 initial_equity = (find_strategy_parameter("initial_equity") >= 0) ?
          strategy_params[find_strategy_parameter("initial_equity")].value : 0;
  */


  ///////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////


  char buffer[512*8192]; // This is the buffer to send the HTML response
  strcpy(buffer, "");
  char line[128*8192]; // Utility buffer to sprint and then concat

  sharedMemory_t* memMap = getMemoryMap();

  // Common utility variables for the whole of answer_to_connection
  int32 total_equity_STR, total_equity_EP, total_margin, total_free_margin;
  char total_equity_STR_S[NLEN],
       total_equity_EP_S[NLEN],
       pctGainS[NLEN],
       total_marginS[NLEN],
       pctMarginUsedS[NLEN],
       free_marginS[SLEN];
  char auto_management_connectedS[NLEN], strategy_connectedS[NLEN];
  char auto_management_toggle_buttonS[NLEN], strategy_toggle_buttonS[NLEN];
  char withinOperatingHoursS[SLEN];


  int32 auIndex = memMap->strAuIndex;
  int32 epIndex = memMap->strEpIndex;
  ///////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////


  // HTML responses, encapsulated

  void HTML_jump_to(char *where, int delay) {
      sprintf (buffer, "<html><meta http-equiv=\"REFRESH\" content=\"%d;%s\" ></html>\n", delay, where);
      ret = 0;
  }



  void HTML_update_currency_shower(char *whereToGo) {

      sprintf(line, "<form action=\"do_change_currency\" method=\"post\"><input type=\"hidden\" name=\"where_to_go\" value=\"%s\">\n", whereToGo);
      strcat(buffer, line);
      strcat(buffer, "<table><TR><TD>Now showing:</TD><TD>\n<select name=\"security\">\n");

      for(int i = 0; i < many_securities; i++) {
          sprintf(line, "<option value=\"%d\"", which_securities[i]);
          strcat(buffer, line);
          if(which_securities[i]==currentCurrency) {
              strcat(buffer, " selected");
          }
          sprintf(line, ">%s (%d)</option>\n", securityName[which_securities[i]], which_securities[i]);
          strcat(buffer, line);
      }

      strcat(buffer, "</select></TD><TD>\n<input type=\"submit\" value=\"Change\"></TD></TR>\n</table>\n</form><p>\n");

  }



  void HTML_do_change_current_currency(void) {

      char where_to_go[NLEN]={0};
      int security=0;

      boolean where_to_go_set = false;
      boolean security_set = false;

      // Now parsing trade parameters

      for(int i = 0; i < post_data_index; i++) {

          if(!strcmp("where_to_go", post_data[i].key)) {
              where_to_go_set = true;
              strcpy(where_to_go, post_data[i].value);
          }

          if(!strcmp("security", post_data[i].key)) {
              security_set = true;
              security = atoi(post_data[i].value);
          }

      }


      if(where_to_go_set && security_set) {

          currentCurrency=security;

          // Now to refresh trade window rather than go to a confirmation page

          HTML_jump_to(where_to_go, 0);

      } else {

          sprintf(buffer,
                        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
                        "<html><head></head><body>\n"
                        "<FONT FACE=\"arial\">"
                        "A very strange error was detected (in HTML_do_change_current_currency) - some parameters were not set (?)<p>\n"
                );

          strcat(buffer, "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n");

          strcat(buffer, "</FONT>\n</body></html>\n");

      }


  }




  void HTML_update_strategy_currency_shower(char *whereToGo) {

      sprintf(line, "<form action=\"do_change_strategy_currency\" method=\"post\"><input type=\"hidden\" name=\"where_to_go\" value=\"%s\">\n", whereToGo);
      strcat(buffer, line);
      strcat(buffer, "<table><TR><TD><b>Strategy currency pair is</b>:</TD><TD>\n<select name=\"security\">\n");

      for(int i = 0; i < many_securities; i++) {
          sprintf(line, "<option value=\"%d\"", which_securities[i]);
          strcat(buffer, line);
          if(which_securities[i]==STR_WHICH_SECURITY) {
              strcat(buffer, " selected");
          }
          sprintf(line, ">%s (%d)</option>\n", securityName[which_securities[i]], which_securities[i]);
          strcat(buffer, line);
      }

      strcat(buffer, "</select></TD><TD>\n<input type=\"submit\" value=\"Change strategy currency\"></TD></TR>\n</table>\n</form><p>\n");

  }



  void HTML_do_change_strategy_currency(void) {

      char where_to_go[NLEN]={0};
      int security=0;

      boolean where_to_go_set = false;
      boolean security_set = false;

      // Now parsing trade parameters

      for(int i = 0; i < post_data_index; i++) {

          if(!strcmp("where_to_go", post_data[i].key)) {
              where_to_go_set = true;
              strcpy(where_to_go, post_data[i].value);
          }

          if(!strcmp("security", post_data[i].key)) {
              security_set = true;
              security = atoi(post_data[i].value);
          }

      }


//      if(!strategy_connected && where_to_go_set && security_set) {
      if(where_to_go_set && security_set) {

          STR_WHICH_SECURITY=security;

          // Now to refresh trade window rather than go to a confirmation page

          HTML_jump_to(where_to_go, 0);
/*
      } else if(strategy_connected) {

          sprintf(buffer,
                        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
                        "<html><head></head><body>\n"
                        "<FONT FACE=\"arial\">"
                        "Whoa!! Are you crazy??? You should not attempt to change the strategy currency while the strategy is connected!!<p>\n"
                        "Pls disconnect the strategy before attempting to change the strategy currency - you fool!<p>\n"
                );

          strcat(buffer, "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n");

          strcat(buffer, "</FONT>\n</body></html>\n");
*/
      } else {

          sprintf(buffer,
                        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
                        "<html><head></head><body>\n"
                        "<FONT FACE=\"arial\">"
                        "A very strange error was detected (in HTML_do_change_current_currency) - some parameters were not set (?)<p>\n"
                );

          strcat(buffer, "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n");

          strcat(buffer, "</FONT>\n</body></html>\n");

      }


  }




  void HTML_show_exposures(void) {

      // Now reporting equities, margins and exposures to assets for this AU:

      // First the header row:

            strcat(buffer,
                    "<b>Exposures data</b>:\n"
                    "<table border=0>\n"
                    "<TR bgcolor=#151540>\n"
                    "<TH><font color=#FFFFFF>Nr</TH>\n"
                    "<TH><font color=#FFFFFF>PB</TH>\n"
                    "<TH><font color=#FFFFFF>EP Equity</TH>\n"
                    "<TH><font color=#FFFFFF>STR Equity</TH>\n"
                    "<TH><font color=#FFFFFF>Used margin (STR)</TH>\n"
                    "<TH><font color=#FFFFFF>% Used margin (STR)</TH>\n"
                    "<TH><font color=#FFFFFF>free margin (STR)</TH>\n"
                    "<TH><font color=#FFFFFF>leverage (EUR)</TH>\n"
            );

            for(idtype s = 0; s < many_securities; s++) {
                sprintf(line, "<TH><font color=#FFFFFF>%s</TH>\n", securityName[which_securities[s]]);
                strcat(buffer, line);
            }

            for(idtype a = 0; a < many_assets; a++) {
                sprintf(line, "<TH><font color=#FFFFFF>%s</TH>\n", assetName[which_assets[a]]);
                strcat(buffer, line);
            }

            strcat(buffer,"</TR>\n");

      // Then broker by broker:

            for(idtype i = 0; i < memMap->nPrimeBrokers; i++) {
                char eq_EP[SLEN], eq_STR[SLEN], marg[SLEN], pct[SLEN], freemarg[SLEN];
                renderWithDots(eq_EP, memMap->EquityPool[epIndex].Equity_EP[i], false);
                renderWithDots(eq_STR, memMap->AccountingUnit[auIndex].Equity_STR[i], false);
                renderWithDots(marg, memMap->AccountingUnit[auIndex].UsedMargin_STR[i], false);
                renderPct(pct, memMap->AccountingUnit[auIndex].UsedMargin_STR[i], memMap->EquityPool[epIndex].Equity_EP[i], false);
                renderWithDots(freemarg, memMap->AccountingUnit[auIndex].FreeMargin_STR[i], false);

                sprintf(line,
                        "<TR bgcolor=#FFFFA0>\n"
                        "<TD align=\"right\"><b>#%d</b></TD>\n"
                        "<TD align=\"center\"><b>%s</b></TD>\n"
                        "<TD align=\"right\">%s</TD>\n"
                        "<TD align=\"right\">%s</TD>\n"
                        "<TD align=\"right\">%s</TD>\n"
                        "<TD align=\"center\">%s</TD>\n"
                        "<TD align=\"center\">%s</TD>\n"
                        "<TD align=\"center\">%.1f</TD>\n",
                        i,
                        memMap->primeBrokerName[i],
                        eq_EP,
                        eq_STR,
                        marg,
                        pct,
                        freemarg,
                        memMap->pbml[i][EUR]
                );

                strcat(buffer, line);

                for(idtype s = 0; s < many_securities; s++) {
                    idtype sec=which_securities[s];
                    if(memMap->AccountingUnit[auIndex].SecurityExposureInt[i][sec].amount!=0) {
                        char amt[NLEN];
                        char avgPrice[NLEN];
                        renderWithDots(amt, memMap->AccountingUnit[auIndex].SecurityExposureInt[i][sec].amount,false);
                        renderPrice(avgPrice,memMap->AccountingUnit[auIndex].SecurityExposureInt[i][sec].price, getSecurityDecimals(sec));
                        sprintf(line, "<TD align=\"right\">%s @%s</TD>\n", amt, avgPrice);
                    } else {
                        sprintf(line, "<TD></TD>\n");
                    }
                    strcat(buffer, line);

                }

                for(idtype a = 0; a < many_assets; a++) {
                    idtype asset=which_assets[a];
                    if(memMap->AccountingUnit[auIndex].AssetExposure[i][asset]!=0) {
                        char amt[NLEN];
                        renderWithDots(amt, memMap->AccountingUnit[auIndex].AssetExposure[i][asset],false);
                        sprintf(line, "<TD align=\"right\">%s</TD>\n", amt);
                    } else {
                        sprintf(line, "<TD align=\"right\"></TD>\n");
                    }
                    strcat(buffer, line);
                }

                strcat(buffer,"</TR>\n");

            }


      // Finally the last row with totals:

            sprintf(line,
                    "<TR bgcolor=#6060C0>\n"
                    "<TH><font color=#FFFFFF>All</TH>\n"
                    "<TH><font color=#FFFFFF>All</TH>\n"
                    "<TH><font color=#FFFFFF>%s</TH>\n"
                    "<TH><font color=#FFFFFF>%s</TH>\n"
                    "<TH><font color=#FFFFFF>%s</TH>\n"
                    "<TH><font color=#FFFFFF>%s</TH>\n"
                    "<TH><font color=#FFFFFF>%s</TH>\n"
                    "<TH><font color=#FFFFFF></TH>\n",
                    total_equity_EP_S,
                    total_equity_STR_S,
                    total_marginS,
                    pctMarginUsedS,
                    free_marginS
            );

            strcat(buffer, line);

            for(int s = 0; s < many_securities; s++) {
                idtype sec=which_securities[s];
                if(memMap->AccountingUnit[auIndex].GblSecurityExposureAUInt[sec].amount!=0) {
                    char amt[NLEN];
                    char avgPrice[NLEN];
                    renderWithDots(amt, memMap->AccountingUnit[auIndex].GblSecurityExposureAUInt[sec].amount, false);
                    renderPrice(avgPrice, memMap->AccountingUnit[auIndex].GblSecurityExposureAUInt[sec].price, getSecurityDecimals(sec));
                    sprintf(line, "<TH><font color=#FFFFFF>%s @%s</TH>\n", amt, avgPrice);
                } else {
                    sprintf(line, "<TH><font color=#FFFFFF></TH>\n");
                }
                strcat(buffer, line);
            }

            for(int a = 0; a < many_assets; a++) {
                idtype asset=which_assets[a];
                if((int32) memMap->AccountingUnit[auIndex].GblAssetExposureAU[asset]!=0) {
                    char amt[NLEN];
                    renderWithDots(amt, (int32) memMap->AccountingUnit[auIndex].GblAssetExposureAU[asset], false);
                    sprintf(line, "<TH><font color=#FFFFFF>%s</TH>\n", amt);
                } else {
                    sprintf(line, "<TH><font color=#FFFFFF></TH>\n");
                }
                strcat(buffer, line);
            }

            strcat(buffer, "</TR>\n");

            // And finally mark-to-markets

            sprintf(line,
                    "<TR bgcolor=#6060C0>\n"
                    "<TH><font color=#FFFFFF>All</TH>\n"
                    "<TH><font color=#FFFFFF>(in Eurs)</TH>\n"
                    "<TH><font color=#FFFFFF></TH>\n"
                    "<TH><font color=#FFFFFF></TH>\n"
                    "<TH><font color=#FFFFFF></TH>\n"
                    "<TH><font color=#FFFFFF></TH>\n"
                    "<TH><font color=#FFFFFF></TH>\n"
                    "<TH><font color=#FFFFFF></TH>\n"
            );

            strcat(buffer, line);

            for(int s = 0; s < many_securities; s++) {
                idtype sec=which_securities[s];
                idtype base, term;
                getBaseAndTerm(sec, &base, &term);

                if(memMap->AccountingUnit[auIndex].GblSecurityExposureAUInt[sec].amount!=0) {
                    if(
                            (memMap->mapBookSp[sec].price!=0) &&
                            (memMap->mapBookAp[base].price!=0) &&
                            (memMap->mapBookAp[term].price!=0)
                       ) {
                        number baseDen=
                                memMap->AccountingUnit[auIndex].GblSecurityExposureAU[sec].exposure *
                                memMap->mapBookAp[base].price;
                        number termDen=
                                memMap->AccountingUnit[auIndex].GblSecurityExposureAU[sec].price *
                                memMap->AccountingUnit[auIndex].GblSecurityExposureAU[sec].exposure *
                                memMap->mapBookAp[term].price;
/*
fprintf(stderr,"sec is %d, base=%d, term=%.1f, baseDen=%.1f, termDen=%.1f, apBase=%.5f, apTerm=%.5f, secP=%.5f\n",
        sec,
        memMapPrivate->GblSecurityExposureAUInt[sec].amount,
        memMap->mapBookSp[sec].price * memMapPrivate->GblSecurityExposureAUInt[sec].amount,
        baseDen,
        termDen,
        memMap->mapBookAp[base].price,
        memMap->mapBookAp[term].price,
        memMap->mapBookSp[sec].price);
fflush(stderr);
*/
                        char M2M[NLEN];
                        renderWithDots(M2M, (int32) (baseDen-termDen), false);
                        sprintf(line, "<TH><font color=#FFFFFF>%s</TH>\n", M2M);
                    } else {
                        sprintf(line, "<TH><font color=#FFFFFF>No book prices!</TH>\n");
                    }
                } else {
                    sprintf(line, "<TH><font color=#FFFFFF></TH>\n");
                }
                strcat(buffer, line);
            }

            for(int a = 0; a < many_assets; a++) {
                idtype asset=which_assets[a];
                if((int32) memMap->AccountingUnit[auIndex].GblAssetExposureAU[asset]!=0) {
                    if(memMap->mapBookAp[asset].price!=0) {
                        char M2M[NLEN];
                        renderWithDots(M2M, (int32) (memMap->mapBookAp[asset].price * memMap->AccountingUnit[auIndex].GblAssetExposureAU[asset]), false);
                        sprintf(line, "<TH><font color=#FFFFFF>%s</TH>\n", M2M);
                    } else {
                        sprintf(line, "<TH><font color=#FFFFFF>No book prices!</TH>\n");
                    }
                } else {
                    sprintf(line, "<TH><font color=#FFFFFF></TH>\n");
                }
                strcat(buffer, line);
            }


            strcat(buffer, "</TR>\n");

            strcat(buffer, "</table>\n");

      // Done!!


  }



  void HTML_show_trading_interfaces(void) {

      // Now reporting trading interfaces status, plus securities prices:

      // First the header row:

            sprintf(line,
                    "<b>Trading interfaces</b>:\n"
                    "<table border=0>\n"
                    "<TR bgcolor=#151540>\n"
                    "<TH><font color=#FFFFFF>TI</TH>\n"
                    "<TH><font color=#FFFFFF>PB</TH>\n"
                    "<TH><font color=#FFFFFF>OK?</TH>\n"
                    "<TH colspan=6><font color=#FFFFFF>Trades sent today (of which failed)</TH>\n"
                    "<TH><font color=#FFFFFF>Yellow card? (Nr)</TH>\n"
                    "<TH><font color=#FFFFFF>Red card?</TH>\n"
                    "<TH><font color=#FFFFFF>prices ToB %s</TH>\n"
                    "</TR>",
                    securityName[currentCurrency]
            );

            strcat(buffer, line);

            strcat(buffer,
                    "<TR bgcolor=#151540>\n"
                    "<TH></TH>\n" // TI
                    "<TH></TH>\n" // PB
                    "<TH></TH>\n" // Ok
                    "<TH><font color=#FFFFFF>Market</TH>\n"
                    "<TH><font color=#FFFFFF>Immed limit</TH>\n"
                    "<TH><font color=#FFFFFF>Other</TH>\n"
                    "<TH><font color=#FFFFFF>Replaces</TH>\n"
                    "<TH><font color=#FFFFFF>Cancels</TH>\n"
                    "<TH><font color=#FFFFFF>Bad quotes</TH>\n"
                    "<TH></TH>\n" // Yellow card
                    "<TH></TH>\n" // Red card
                    "<TH></TH>\n" // Prices
                    "</TR>"
            );


      // Then TI by TI:
            uint32 marketTradesSentTodayAllTIs=0;
            uint32 failedMarketTradesTodayAllTIs=0;
            uint32 immediateLimitTradesSentTodayAllTIs=0;
            uint32 failedLimitTradesTodayAllTIs=0;
            uint32 otherTradesSentTodayAllTIs=0;
            uint32 failedOtherTradesTodayAllTIs=0;
            uint32 replacesTodayAllTIs=0;
            uint32 cancelsTodayAllTIs=0;
            uint32 badQuotesTodayAllTIs=0;

            for(idtype i = 0; i < memMap->nTradingInterfaces; i++) {

                char TIst[SLEN];
                switch(memMap->tradingInterfaceOk[i]) {
                    case TI_STATUS_NOK : sprintf(TIst, "<TD align=\"center\" style=\"background-color: red\"><b>No</b>"); break;
                    case TI_STATUS_OK : sprintf(TIst, "<TD align=\"center\" style=\"background-color: lime\"><b>Yes</b>"); break;
                    case TI_STATUS_OK_NO_PRICE : sprintf(TIst, "<TD align=\"center\" style=\"background-color: yellow\"><b>No prices</b>"); break;
                    default : sprintf(TIst, "<TD align=\"center\" style=\"background-color: black\"><b>Unknown!!</b>");
                }
                if(pricesDelayed[i]) strcat(TIst, "[prices delayed!]");
                if(noTicks[i]) strcat(TIst, "[no ticks!]");
                strcat(TIst, "</TD>");

                char PriceBidS[SLEN], PriceAskS[SLEN];
                renderPrice(PriceBidS, memMap->mapTiTbBid[i][currentCurrency].price, getSecurityDecimals(currentCurrency));
                renderPrice(PriceAskS, memMap->mapTiTbAsk[i][currentCurrency].price, getSecurityDecimals(currentCurrency));

                char YellowCardStr[SLEN];
                if(tiOperation[i].yellowCard)
                    sprintf(YellowCardStr,"<TD align=\"center\" style=\"background-color: yellow\"><b>Yellow card!! (%d)</b></TD>",
                            tiOperation[i].yellowCardCounter);
                else
                    sprintf(YellowCardStr, "<TD></TD>");

                uint32 immediateLimitTradesToday=
                        tiPerformanceInfo[i].fullyFilledImmediateLimitTradesToday
                        + tiPerformanceInfo[i].partiallyFilledImmediateLimitTradesToday
                        + tiPerformanceInfo[i].failedAndRejectedImmediateLimitTradesToday
//                        + tiPerformanceInfo[i].userCanceledImmediateLimitTradesToday
                        ;

                uint32 marketTradesToday=
                        tiPerformanceInfo[i].fullyFilledMarketTradesToday
                        + tiPerformanceInfo[i].partiallyFilledMarketTradesToday
                        + tiPerformanceInfo[i].failedAndRejectedMarketTradesToday
//                        + tiPerformanceInfo[i].userCanceledMarketTradesToday
                        ;

                uint32 otherTradesToday=
                        tiPerformanceInfo[i].fullyFilledOtherTradesToday
                        + tiPerformanceInfo[i].partiallyFilledOtherTradesToday
                        + tiPerformanceInfo[i].failedAndRejectedOtherTradesToday
//                        + tiPerformanceInfo[i].userCanceledOtherTradesToday
                        ;

                sprintf(line,
                        "<TR bgcolor=#FFFFA0>\n"
                        "<TD align=\"left\"><b>#%d - %s (%d)</b></TD>\n"
                        "<TD align=\"left\">#%d - %s</TD>\n"
                        "%s\n"
                        "<TD align=\"center\">%d (%d)</TD>\n"
                        "<TD align=\"center\">%d (%d)</TD>\n"
                        "<TD align=\"center\">%d (%d)</TD>\n"
                        "<TD align=\"center\">%d</TD>\n"
                        "<TD align=\"center\">%d</TD>\n"
                        "<TD align=\"center\">%d</TD>\n"
                        "%s\n"
                        "%s\n"
                        "<TD align=\"center\">[%s|%s]</TD>\n"
                        "</TR>\n",
                        i,
                        memMap->tradingInterfaceName[i],
                        memMap->whichVenue[i],
                        memMap->primeBrokerIndex[memMap->whichPrimeBroker[i]],
                        memMap->primeBrokerName[memMap->primeBrokerIndex[memMap->whichPrimeBroker[i]]],
                        TIst,
                        marketTradesToday,
                        tiPerformanceInfo[i].failedAndRejectedMarketTradesToday,
                        immediateLimitTradesToday,
                        tiPerformanceInfo[i].failedAndRejectedImmediateLimitTradesToday,
                        otherTradesToday,
                        tiPerformanceInfo[i].failedAndRejectedOtherTradesToday,
                        tiPerformanceInfo[i].replacesDoneToday,
                        tiPerformanceInfo[i].userCanceledImmediateLimitTradesToday +
                        tiPerformanceInfo[i].userCanceledMarketTradesToday +
                        tiPerformanceInfo[i].userCanceledOtherTradesToday,
                        tiOperation[i].badQuotesToday,
                        YellowCardStr,
                        (tiOperation[i].redCard ?
                                "<TD align=\"center\" style=\"background-color: red\"><b>Red card!!</b></TD>" :
                                "<TD></TD>"),
                        PriceBidS,
                        PriceAskS
                );

                strcat(buffer, line);

                // Now compiled stats for all TIs
                marketTradesSentTodayAllTIs+=marketTradesToday;
                failedMarketTradesTodayAllTIs+=tiPerformanceInfo[i].failedAndRejectedMarketTradesToday;
                immediateLimitTradesSentTodayAllTIs+=immediateLimitTradesToday;
                failedLimitTradesTodayAllTIs+=tiPerformanceInfo[i].failedAndRejectedImmediateLimitTradesToday;
                otherTradesSentTodayAllTIs+=otherTradesToday;
                failedOtherTradesTodayAllTIs+=tiPerformanceInfo[i].failedAndRejectedOtherTradesToday;
                replacesTodayAllTIs+=tiPerformanceInfo[i].replacesDoneToday;
                cancelsTodayAllTIs+=tiPerformanceInfo[i].userCanceledImmediateLimitTradesToday +
                                    tiPerformanceInfo[i].userCanceledMarketTradesToday +
                                    tiPerformanceInfo[i].userCanceledOtherTradesToday;
                badQuotesTodayAllTIs+=tiOperation[i].badQuotesToday;

            } // for (trading interfaces)

            char SystemPriceS[NLEN];
            renderPrice(SystemPriceS, memMap->mapBookSpInt[currentCurrency], getSecurityDecimals(currentCurrency));

            sprintf(line,
                    "<TR bgcolor=#151540>\n"
                    "<TH><font color=#FFFFFF>All</TH>\n"
                    "<TH><font color=#FFFFFF>All</TH>\n"
                    "<TH><font color=#FFFFFF></TH>\n"
                    "<TH><font color=#FFFFFF>%d (%d)</TH>\n"
                    "<TH><font color=#FFFFFF>%d (%d)</TH>\n"
                    "<TH><font color=#FFFFFF>%d (%d)</TH>\n"
                    "<TH><font color=#FFFFFF>%d</TH>\n"
                    "<TH><font color=#FFFFFF>%d</TH>\n"
                    "<TH><font color=#FFFFFF>%d</TH>\n"
                    "<TH><font color=#FFFFFF></TH>\n"
                    "<TH><font color=#FFFFFF></TH>\n"
                    "<TH><font color=#FFFFFF>%s</TH>\n"
                    "</TR>",
                    marketTradesSentTodayAllTIs,
                    failedMarketTradesTodayAllTIs,
                    immediateLimitTradesSentTodayAllTIs,
                    failedLimitTradesTodayAllTIs,
                    otherTradesSentTodayAllTIs,
                    failedOtherTradesTodayAllTIs,
                    replacesTodayAllTIs,
                    cancelsTodayAllTIs,
                    badQuotesTodayAllTIs,
                    SystemPriceS
            );

            strcat(buffer, line);

            strcat(buffer, "</table><p>\n");

      // Done!!

  }



  void HTML_show_sorted_prices(idtype sec) {

      sprintf(line,
              "<b>Prices available for %s</b>:\n"
              "<table border=0>\n",
              securityName[sec]
             );

      strcat(buffer, line);


      for(int i=memMap->mapSortAskNumberOfTi[sec]-1; i>=0; i--) {

          idtype ti=memMap->mapSortingAsk[sec][i].tiIndex;

          char priceS[SLEN];
          char liquidityS[SLEN];

          renderPrice(priceS, memMap->mapTiTbAsk[ti][sec].price, getSecurityDecimals(sec));
//          renderWithDots(liquidityS, memMap->mapTiTbAsk[ti][sec].liquidity, false);
          sprintf(liquidityS, "%.1fM", 0.1 * (double) (memMap->mapTiTbAsk[ti][sec].liquidity) / 100000);

          sprintf(line,
                  "<TR>\n"
                  "<TD align=\"right\" bgcolor=#BBFFFF></TD>\n"
                  "<TD align=\"right\" bgcolor=#BBFFFF></TD>\n"
                  "<TD align=\"right\" bgcolor=#BBFFFF></TD>\n"
                  "<TD align=\"right\" bgcolor=#FFFFBB><b>%s</b></TD>\n" // Price
                  "<TD align=\"right\" bgcolor=#FFFFBB>%s</TD>\n" // Liquidity
                  "<TD align=\"right\" bgcolor=#FFFFBB>%s</TD>\n" // TI
                  "</TR>\n",
                  priceS,
                  liquidityS,
                  memMap->tradingInterfaceName[ti]
                 );

          strcat(buffer, line);

      }


      sprintf(line,
              "<TR>\n"
              "<TD align=\"center\" bgcolor=#BBFFFF></TD>\n"
              "<TD align=\"center\" bgcolor=#BBFFFF></TD>\n"
              "<TD align=\"center\" bgcolor=#BBFFFF><b></b></TD>\n"
              "<TD align=\"center\" bgcolor=#FFFFBB><b></b></TD>\n"
              "<TD align=\"center\" bgcolor=#FFFFBB></TD>\n"
              "<TD align=\"center\" bgcolor=#FFFFBB></TD>\n"
              "</TR>\n"
             );

      strcat(buffer, line);


      for(int i=0; i<memMap->mapSortBidNumberOfTi[sec]; i++) {

          idtype ti=memMap->mapSortingBid[sec][i].tiIndex;

          char priceS[SLEN];
          char liquidityS[SLEN];

          renderPrice(priceS, memMap->mapTiTbBid[ti][sec].price, getSecurityDecimals(sec));
//          renderWithDots(liquidityS, memMap->mapTiTbBid[ti][sec].liquidity, false);
          sprintf(liquidityS, "%.1fM", 0.1 * (double) (memMap->mapTiTbBid[ti][sec].liquidity) / 100000);

          sprintf(line,
                  "<TR>\n"
                  "<TD align=\"left\" bgcolor=#BBFFFF>%s</TD>\n" // TI
                  "<TD align=\"left\" bgcolor=#BBFFFF>%s</TD>\n" // Liquidity
                  "<TD align=\"left\" bgcolor=#BBFFFF><b>%s</b></TD>\n" // Price
                  "<TD align=\"left\" bgcolor=#FFFFBB><b></b></TD>\n"
                  "<TD align=\"left\" bgcolor=#FFFFBB></TD>\n"
                  "<TD align=\"left\" bgcolor=#FFFFBB></TD>\n"
                  "</TR>\n",
                  memMap->tradingInterfaceName[ti],
                  liquidityS,
                  priceS
                 );

          strcat(buffer, line);

      }


      strcat(buffer, "\n</table><p>\n");


  }




  void HTML_show_sorted_prices_ex_own_orders(idtype sec) {

      int nSortedBids;
      uint32 sortedBidPrices[MAX_NUMBER_TI];
      idtype sortedBidTiIndexes[MAX_NUMBER_TI];
      uint32 nonSortedBidPrices[MAX_NUMBER_TI];
      boolean tiBidInSortArray[MAX_NUMBER_TI];

      topOfBooksExOwnOrders(memMap, STR_WHICH_SECURITY, true /* Bid */,
                            &nSortedBids, sortedBidPrices, sortedBidTiIndexes, nonSortedBidPrices, tiBidInSortArray);

      int nSortedAsks;
      uint32 sortedAskPrices[MAX_NUMBER_TI];
      idtype sortedAskTiIndexes[MAX_NUMBER_TI];
      uint32 nonSortedAskPrices[MAX_NUMBER_TI];
      boolean tiAskInSortArray[MAX_NUMBER_TI];

      topOfBooksExOwnOrders(memMap, STR_WHICH_SECURITY, false /* Ask */,
                            &nSortedAsks, sortedAskPrices, sortedAskTiIndexes, nonSortedAskPrices, tiAskInSortArray);


      sprintf(line,
              "<b>Prices available for %s (top of book prices ex own orders)</b>:\n"
              "<table border=0>\n",
              securityName[sec]
             );

      strcat(buffer, line);


      for(int i=nSortedAsks-1; i>=0; i--) {

          idtype ti=sortedAskTiIndexes[i];

          char priceS[SLEN];
          renderPrice(priceS, sortedAskPrices[i], getSecurityDecimals(sec));

          sprintf(line,
                  "<TR>\n"
                  "<TD align=\"right\" bgcolor=#BBFFFF></TD>\n"
                  "<TD align=\"right\" bgcolor=#BBFFFF></TD>\n"
                  "<TD align=\"right\" bgcolor=#FFFFBB><b>%s</b></TD>\n" // Price
                  "<TD align=\"right\" bgcolor=#FFFFBB>%s</TD>\n" // TI
                  "</TR>\n",
                  priceS,
                  memMap->tradingInterfaceName[ti]
                 );

          strcat(buffer, line);

      }


      sprintf(line,
              "<TR>\n"
              "<TD align=\"center\" bgcolor=#BBFFFF></TD>\n"
              "<TD align=\"center\" bgcolor=#BBFFFF><b></b></TD>\n"
              "<TD align=\"center\" bgcolor=#FFFFBB><b></b></TD>\n"
              "<TD align=\"center\" bgcolor=#FFFFBB></TD>\n"
              "</TR>\n"
             );

      strcat(buffer, line);


      for(int i=0; i<nSortedBids; i++) {

          idtype ti=sortedBidTiIndexes[i];

          char priceS[SLEN];
          renderPrice(priceS, sortedBidPrices[i], getSecurityDecimals(sec));

          sprintf(line,
                  "<TR>\n"
                  "<TD align=\"left\" bgcolor=#BBFFFF>%s</TD>\n" // TI
                  "<TD align=\"left\" bgcolor=#BBFFFF><b>%s</b></TD>\n" // Price
                  "<TD align=\"left\" bgcolor=#FFFFBB><b></b></TD>\n"
                  "<TD align=\"left\" bgcolor=#FFFFBB></TD>\n"
                  "</TR>\n",
                  memMap->tradingInterfaceName[ti],
                  priceS
                 );

          strcat(buffer, line);

      }


      strcat(buffer, "\n</table><p>\n");


  }




  void HTML_show_trades_historical(void) {

      if(memMap->AccountingUnit[auIndex].indexOfHistoric <= 0) {

          strcat(buffer, "(No historical trades on record)<br>\n");
          return;

      }

      // First the header row:

      sprintf(line,
              "<b>Historical trades - showing %d (out of %d in total)</b>:\n"
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
              "<TH><font color=#FFFFFF>ID(core)</TH>\n"
              "<TH><font color=#FFFFFF>Time sent</TH>\n"
              "<TH><font color=#FFFFFF>Time finished</TH>\n"
              "<TH><font color=#FFFFFF>TI</TH>\n"
              "<TH><font color=#FFFFFF>PB</TH>\n"
              "<TH><font color=#FFFFFF>Sec</TH>\n"
              "<TH><font color=#FFFFFF>Amount</TH>\n"
              "<TH><font color=#FFFFFF>Side</TH>\n"
              "<TH><font color=#FFFFFF>Type</TH>\n"
              "<TH><font color=#FFFFFF>Limit</TH>\n"
              "<TH><font color=#FFFFFF>TimeInForce</TH>\n"
              "<TH><font color=#FFFFFF>Status</TH>\n"
              "<TH><font color=#FFFFFF>Amount filled</TH>\n"
              "<TH><font color=#FFFFFF>Avg price</TH>\n"
              "<TH><font color=#FFFFFF>Ref price</TH>\n"
              "<TH><font color=#FFFFFF>Diff (slippage)</TH>\n"
              "<TH><font color=#FFFFFF>Commissions</TH>\n"
              "<TH><font color=#FFFFFF>Reserved</TH>\n"
              "<TH><font color=#FFFFFF>Reason</TH>\n"
              "</TR>",
              (memMap->AccountingUnit[auIndex].indexOfHistoric > _MAX_HIST_TRADES_TO_SHOW_ ? _MAX_HIST_TRADES_TO_SHOW_ : memMap->AccountingUnit[auIndex].indexOfHistoric),
              memMap->AccountingUnit[auIndex].indexOfHistoric
      );

      strcat(buffer, line);

// Then we show the trades:

      for(int i=memMap->AccountingUnit[auIndex].indexOfHistoric - 1; (i>=0) && (memMap->AccountingUnit[auIndex].indexOfHistoric -i < _MAX_HIST_TRADES_TO_SHOW_); i--) {

          char statusS[SLEN];
          char amountS[SLEN];
          char typeS[SLEN];
          char limitPriceS[SLEN];
          char timeInForceS[SLEN];
          char amountFilledS[SLEN];
          char avgPriceS[SLEN];
          char refPriceS[SLEN];
          char initTime[SLEN];
          char endTime[SLEN];
          char _millisecs[SLEN];

          renderWithDots(amountS, memMap->AccountingUnit[auIndex].historic[i].params.quantity, false);
          renderWithDots(amountFilledS, memMap->AccountingUnit[auIndex].historic[i].params.finishedQuantity, false);
          getTradeTypeString(memMap->AccountingUnit[auIndex].historic[i].params.tradeType, typeS);
          getTimeInForceString(memMap->AccountingUnit[auIndex].historic[i].params.timeInForce, timeInForceS);
          getTradeStatusString(memMap->AccountingUnit[auIndex].historic[i].info.status, memMap->AccountingUnit[auIndex].historic[i].info.substatus, statusS);
          renderPrice(limitPriceS, memMap->AccountingUnit[auIndex].historic[i].params.limitPrice, getSecurityDecimals(memMap->AccountingUnit[auIndex].historic[i].params.security));
          renderPrice(avgPriceS, memMap->AccountingUnit[auIndex].historic[i].params.finishedPrice, getSecurityDecimals(memMap->AccountingUnit[auIndex].historic[i].params.security));
          renderPrice(refPriceS, memMap->AccountingUnit[auIndex].historic[i].params.priceAtStart, getSecurityDecimals(memMap->AccountingUnit[auIndex].historic[i].params.security));

          timestampToDateStr (memMap->AccountingUnit[auIndex].historic[i].info.timeRxCore.sec, initTime);
          sprintf(_millisecs, ".%d", memMap->AccountingUnit[auIndex].historic[i].info.timeRxCore.usec / 1000);
          strcat(initTime, _millisecs);

          timestampToDateStr (memMap->AccountingUnit[auIndex].historic[i].info.timeExecuted.sec, endTime);
          sprintf(_millisecs, ".%d", memMap->AccountingUnit[auIndex].historic[i].info.timeExecuted.usec / 1000);
          strcat(endTime, _millisecs);

          int32 slippage = memMap->AccountingUnit[auIndex].historic[i].params.side == TRADE_SIDE_BUY ?
                  memMap->AccountingUnit[auIndex].historic[i].params.finishedPrice - memMap->AccountingUnit[auIndex].historic[i].params.priceAtStart :
                  memMap->AccountingUnit[auIndex].historic[i].params.priceAtStart - memMap->AccountingUnit[auIndex].historic[i].params.finishedPrice;

          sprintf(line,
                  "<TR bgcolor=#FFFFA0>\n"
                  "<TD align=\"center\"><b>%s</b></TD>\n"    // Trade ID (core)
                  "<TD align=\"center\"><b>%s</b></TD>\n"    // Time sent
                  "<TD align=\"center\"><b>%s</b></TD>\n"    // Time finished
                  "<TD align=\"center\"><b>%s</b></TD>\n"    // TI name
                  "<TD align=\"center\"><b>%s</b></TD>\n"    // PB name
                  "<TD align=\"center\"><b>%s</b></TD>\n"    // Security
                  "<TD align=\"center\"><b>%s</b></TD>\n"    // Amount
                  "<TD align=\"center\"><b>%s</b></TD>\n"    // Side
                  "<TD align=\"center\"><b>%s</b></TD>\n"    // Trade type
                  "<TD align=\"center\"><b>%s</b></TD>\n"    // Limit
                  "<TD align=\"center\"><b>%s</b></TD>\n"    // TimeInForce
                  "<TD align=\"center\"><b>%s</b></TD>\n"    // Status
                  "<TD align=\"center\"><b>%s</b></TD>\n"    // Amount filled
                  "<TD align=\"center\"><b>%s</b></TD>\n"    // Avg price
                  "<TD align=\"center\"><b>%s</b></TD>\n"    // Ref price
                  "<TD align=\"center\"><b>%d</b></TD>\n"    // Diff (slippage)
                  "<TD align=\"center\"><b>%.2f</b></TD>\n"  // Commissions
                  "<TD align=\"center\"><b>%u</b></TD>\n"   // Reserved data
                  "<TD align=\"center\"><b>%s</b></TD>\n"    // Reason
                  "</TR>\n",
                  memMap->AccountingUnit[auIndex].historic[i].ids.fixId, // Trade ID (core)
                  initTime, // Time sent
                  endTime, // Time finished
                  memMap->tradingInterfaceName[memMap->tradingInterfaceIndex[memMap->AccountingUnit[auIndex].historic[i].params.tiId]], // TI name
                  memMap->primeBrokerName[memMap->primeBrokerIndex[memMap->whichPrimeBroker[memMap->tradingInterfaceIndex[memMap->AccountingUnit[auIndex].historic[i].params.tiId]]]], // PB name
                  securityName[memMap->AccountingUnit[auIndex].historic[i].params.security], // Security
                  amountS, // Amount
                  (memMap->AccountingUnit[auIndex].historic[i].params.side == TRADE_SIDE_BUY ? "BUY" : "SELL"), // Side
                  typeS, // Trade type
                  limitPriceS, // Limit price (for limit orders)
                  timeInForceS, // TimeInForce
                  statusS, // Status
                  amountFilledS, // Amount filled
                  avgPriceS, // Avg price
                  refPriceS, // Ref price
                  slippage, // Diff (slippage)
                  memMap->AccountingUnit[auIndex].historic[i].params.commission, // Commissions
                  memMap->AccountingUnit[auIndex].historic[i].params.reservedData,
                  memMap->AccountingUnit[auIndex].historic[i].info.reason
          );

          strcat(buffer, line);

      } // for

      strcat(buffer, "\n</table><p>\n");

// Done!!

  }




  void HTML_show_trades_alive(void) {

      if(memMap->AccountingUnit[auIndex].numberOfAlives <= 0) {

          strcat(buffer, "(No trades alive)<br>\n");
          return;

      }

      // First the header row:

      strcat(buffer,
              "<b>Live trades</b>:\n"
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
              "<TH><font color=#FFFFFF>ID(core)</TH>\n"
              "<TH><font color=#FFFFFF>Time start</TH>\n"
              "<TH><font color=#FFFFFF>Time ack from venue</TH>\n"
              "<TH><font color=#FFFFFF>TI</TH>\n"
              "<TH><font color=#FFFFFF>PB</TH>\n"
              "<TH><font color=#FFFFFF>Sec</TH>\n"
              "<TH><font color=#FFFFFF>Amount</TH>\n"
              "<TH><font color=#FFFFFF>Side</TH>\n"
              "<TH><font color=#FFFFFF>Type</TH>\n"
              "<TH><font color=#FFFFFF>Limit</TH>\n"
              "<TH><font color=#FFFFFF>TimeInForce</TH>\n"
              "<TH><font color=#FFFFFF>RefPrice</TH>\n"
              "<TH><font color=#FFFFFF>Reserved data</TH>\n"
              "<TH><font color=#FFFFFF>Status</TH>\n"
              "<TH><font color=#FFFFFF>Cancel?</TH>\n"
              "<TH><font color=#FFFFFF>Replace?</TH>\n"
              "</TR>"
      );

// Then we show the trades:

      int manyAlives=0;

      for(int i=MAX_TRADES_LIST_ALIVE - 1; (i>=0) && (manyAlives < memMap->AccountingUnit[auIndex].numberOfAlives); i--) {

          if(true == memMap->AccountingUnit[auIndex].alive[i].alive) {

              manyAlives++;

              char statusS[SLEN];
              char amountS[SLEN];
              char typeS[SLEN];
              char timeInForceS[SLEN];
              char cancelS[SLEN];
              char tradeIdCore[SLEN];
              char replaceS[SLEN];
              char limitS[SLEN];
              char refPriceS[SLEN];
              char timeStarted[SLEN];
              char timeAck[SLEN];
              char _millisecs[SLEN];

              renderWithDots(amountS, memMap->AccountingUnit[auIndex].alive[i].params.quantity, false);
              getTradeTypeString(memMap->AccountingUnit[auIndex].alive[i].params.tradeType, typeS);
              getTimeInForceString(memMap->AccountingUnit[auIndex].alive[i].params.timeInForce, timeInForceS);
              getTradeStatusString(memMap->AccountingUnit[auIndex].alive[i].info.status, memMap->AccountingUnit[auIndex].alive[i].info.substatus, statusS);
              renderPrice(limitS, memMap->AccountingUnit[auIndex].alive[i].params.limitPrice, getSecurityDecimals(memMap->AccountingUnit[auIndex].alive[i].params.security));
              renderPrice(refPriceS, memMap->AccountingUnit[auIndex].alive[i].params.priceAtStart, getSecurityDecimals(memMap->AccountingUnit[auIndex].alive[i].params.security));

              timestampToDateStr (memMap->AccountingUnit[auIndex].alive[i].info.timeRxCore.sec, timeStarted);
              sprintf(_millisecs, ".%d", memMap->AccountingUnit[auIndex].alive[i].info.timeRxCore.usec / 1000);
              strcat(timeStarted, _millisecs);

              timestampToDateStr (memMap->AccountingUnit[auIndex].alive[i].info.timeAckFromVenue.sec, timeAck);
              sprintf(_millisecs, ".%d", memMap->AccountingUnit[auIndex].alive[i].info.timeAckFromVenue.usec / 1000);
              strcat(timeAck, _millisecs);

              if(memMap->AccountingUnit[auIndex].alive[i].info.cancel.requested && memMap->AccountingUnit[auIndex].alive[i].info.cancel.rejected)
                  sprintf(cancelS, "Cancel rejected!");
              else if(memMap->AccountingUnit[auIndex].alive[i].info.cancel.requested)
                  sprintf(cancelS, "Cancel sent!");
              else
                  strcpy(cancelS, "");

              sprintf(tradeIdCore, "%s", memMap->AccountingUnit[auIndex].alive[i].ids.fixId);

              strcat(cancelS, "<form action=\"do_cancel\" method=\"post\"><input type=\"hidden\" name=\"which_trade\" value=\"");
              strcat(cancelS, tradeIdCore);
              strcat(cancelS, "\"><input type=\"hidden\" name=\"ti_index\" value=\"");
              sprintf(line, "%d", memMap->tradingInterfaceIndex[memMap->AccountingUnit[auIndex].alive[i].params.tiId]);
              strcat(cancelS,line);
              strcat(cancelS, "\"><input type=\"submit\" value=\"Cancel\"></form>\n");

              if(memMap->AccountingUnit[auIndex].alive[i].info.modify.requested && memMap->AccountingUnit[auIndex].alive[i].info.modify.rejected)
                  sprintf(replaceS, "Replace rejected!");
              else if(memMap->AccountingUnit[auIndex].alive[i].info.modify.requested) {
                  char quantS[SLEN];
                  char prS[SLEN];
                  renderWithDots(quantS, memMap->AccountingUnit[auIndex].alive[i].info.modify.quantity, false);
                  renderPrice(prS, memMap->AccountingUnit[auIndex].alive[i].info.modify.price, getSecurityDecimals(memMap->AccountingUnit[auIndex].alive[i].params.security));
                  sprintf(replaceS, "Replace sent (q=%s, limit=%s)", quantS, prS);
              } else {
                  strcpy(replaceS, "");
              }

              strcat(replaceS, "<form action=\"do_replace\" method=\"post\"><input type=\"hidden\" name=\"which_trade\" value=\"");
              strcat(replaceS, tradeIdCore);
              strcat(replaceS, "\"><input type=\"hidden\" name=\"ti_index\" value=\"");
              sprintf(line, "%d", memMap->tradingInterfaceIndex[memMap->AccountingUnit[auIndex].alive[i].params.tiId]);
              strcat(replaceS,line);
              strcat(replaceS, "\">New Q:<input type=\"text\" name=\"new_quantity\" value=\"0\" size=\"10\">"
                               "New P:<input type=\"text\" name=\"new_price\" value=\"0\" size=\"8\">"
                               "<input type=\"submit\" value=\"Replace\"></form>");

              if(memMap->AccountingUnit[auIndex].alive[i].info.status==TRADE_STATE_INDETERMINED) {
                  strcat(statusS, "\n<form action=\"do_resolve\" method=\"post\">\n"
                                  "<input type=\"hidden\" name=\"which_trade\" value=\"");
                  strcat(statusS, tradeIdCore);
                  strcat(statusS, "\"><input type=\"text\" name=\"quantity\" value=\"0\">\n"
                                  "<input type=\"text\" name=\"price\" value=\"");
                  char priceS[64];
                  sprintf(priceS, "%d", memMap->AccountingUnit[auIndex].alive[i].params.priceAtStart);
                  strcat(statusS, priceS);
                  strcat(statusS, "\">\n<input type=\"submit\" value=\"Resolve\"></form>\n"
                         );
              }

              sprintf(line,
                      "<TR bgcolor=#FFFFA0>\n"
                      "<TD align=\"center\"><b>%s</b></TD>\n"  // Trade ID (core)
                      "<TD align=\"center\"><b>%s</b></TD>\n"  // TI name
                      "<TD align=\"center\"><b>%s</b></TD>\n"  // Time start
                      "<TD align=\"center\"><b>%s</b></TD>\n"  // Time ack form venue
                      "<TD align=\"center\"><b>%s</b></TD>\n"  // PB name
                      "<TD align=\"center\"><b>%s</b></TD>\n"  // Security
                      "<TD align=\"center\"><b>%s</b></TD>\n"  // Amount
                      "<TD align=\"center\"><b>%s</b></TD>\n"  // Side
                      "<TD align=\"center\"><b>%s</b></TD>\n"  // Trade type
                      "<TD align=\"center\"><b>%s</b></TD>\n"  // Limit
                      "<TD align=\"center\"><b>%s</b></TD>\n"  // TimeInForce
                      "<TD align=\"center\"><b>%s</b></TD>\n"  // ref Price
                      "<TD align=\"center\"><b>%u</b></TD>\n"  // Reserved data
                      "<TD align=\"center\"><b>%s</b></TD>\n"  // Status
                      "<TD align=\"center\"><b>%s</b></TD>\n"  // Cancel
                      "<TD align=\"center\"><b>%s</b></TD>\n"  // Replace
                      "</TR>\n",
                      memMap->AccountingUnit[auIndex].alive[i].ids.fixId, // Trade ID (core)
                      timeStarted, // Time start
                      timeAck, // Time ack from venue
                      memMap->tradingInterfaceName[memMap->tradingInterfaceIndex[memMap->AccountingUnit[auIndex].alive[i].params.tiId]], // TI name
                      memMap->primeBrokerName[memMap->primeBrokerIndex[memMap->whichPrimeBroker[memMap->tradingInterfaceIndex[memMap->AccountingUnit[auIndex].alive[i].params.tiId]]]], // PB name
                      securityName[memMap->AccountingUnit[auIndex].alive[i].params.security], // Security
                      amountS, // Amount
                      (memMap->AccountingUnit[auIndex].alive[i].params.side == TRADE_SIDE_BUY ? "BUY" : "SELL"), // Side
                      typeS, // Trade type
                      limitS, // Limit price (for limit orders)
                      timeInForceS, // TimeInForce
                      refPriceS, // Ref price
                      memMap->AccountingUnit[auIndex].alive[i].params.reservedData,
                      statusS,
                      cancelS,
                      replaceS
              );

              strcat(buffer, line);

          } // alive[i].alive is true

      } // for

      strcat(buffer, "\n</table><br>\n");

      strcat(buffer, "<form action=\"cancel_all\" method=\"get\"><input type=\"submit\" value=\"Cancel all trades alive\"></form>\n");



// Done!!

  }




  void HTML_show_report(void) {

      if(auto_management_connected) {

          sprintf(auto_management_connectedS, "<strong style=\"background-color: lime\">Connected</strong>");
          sprintf(auto_management_toggle_buttonS, "Disconnect");

      } else {

          sprintf(auto_management_connectedS, "<strong style=\"background-color: red\">Disconnected</strong>");
          sprintf(auto_management_toggle_buttonS, "Connect");

      }


      if(strategy_connected) {

          sprintf(strategy_connectedS, "<strong style=\"background-color: lime\">Connected</strong>");
          sprintf(strategy_toggle_buttonS, "Disconnect");

      } else {

          sprintf(strategy_connectedS, "<strong style=\"background-color: red\">Disconnected</strong>");
          sprintf(strategy_toggle_buttonS, "Connect");

      }

      if(withinOperatingHours) {

          sprintf(withinOperatingHoursS, "<strong style=\"background-color: lime\">Within operating hours</strong>");

      } else {

          sprintf(withinOperatingHoursS, "<strong style=\"background-color: red\">Outside operating hours (strategy not called)</strong>");

      }
      char timeNow[SLEN];
      getStringTimeNowUTC(timeNow);

      char callbackNrStr[SLEN];
      renderWithDots(callbackNrStr, callback_number, false);

      char timeUp[SLEN] = "";
      uint32 secondsDiff = CALLBACK_systime.sec - INITIAL_systime.sec;
      uint32 daysUp = secondsDiff / (60 * 60 * 24);
      uint32 hoursUp = (secondsDiff % (60 * 60 * 24)) / (60 * 60);
      uint32 minsUp =  (secondsDiff % (60 * 60)) / (60);
      uint32 secondsUp = secondsDiff % 60;
      char moreStr[SLEN];
      boolean somethingAlready=false;

      if(daysUp>0) {
          sprintf(moreStr, "%d days", daysUp);
          strcat(timeUp, moreStr);
          somethingAlready = true;
      }

      if(somethingAlready || (hoursUp>0)) {
          if(somethingAlready) strcat(timeUp, ", ");
          sprintf(moreStr, "%d hours", hoursUp);
          strcat(timeUp, moreStr);
          somethingAlready = true;
      }

      if(somethingAlready || (minsUp>0)) {
          if(somethingAlready) strcat(timeUp, ", ");
          sprintf(moreStr, "%d minutes", minsUp);
          strcat(timeUp, moreStr);
          somethingAlready = true;
      }

      if(somethingAlready) strcat(timeUp, ", and ");
      sprintf(moreStr, "%d seconds", secondsUp);
      strcat(timeUp, moreStr);

      sprintf(buffer,
              "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
              "<html><head></head><body>\n"
              "<FONT FACE=\"arial\">"
              "Hello from <b>Enthalpy Web Server %s </b>(%s)<p>\n"
              "Current (UTC) time is %s, week day is %d<p>"
              "Callback number %s - and counting! (MT sequence is %lu); Stub on since %s (%s)<p>"
              "Acc unit = %d<br>\n"
              "User is %s<br>\n"
              "Strategy executable file is %s<p>\n"
              "<form action=\"toggle_auto_management\" method=\"get\">(Overall) auto management is %s <input type=\"submit\" value=\"%s\"></form>\n"
              "<form action=\"toggle_strategy\" method=\"get\">Strategy is %s <input type=\"submit\" value=\"%s\"%s></form><br>\n"
              "</TH>\n"
              "<table><TR>"
              "<TH><form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Refresh\"></form></TH>\n"
              "<TH><form action=\"config\" method=\"get\"><input type=\"submit\" value=\"Config\"></form></TH>\n"
              "<TH><form action=\"trade\" method=\"get\"><input type=\"submit\" value=\"Trade\"></form></TH>\n"
              "<TH><form action=\"show_full_book\" method=\"get\"><input type=\"submit\" value=\"Full Book\"></form></TH>\n"
              "<TH><form action=\"backoffice\" method=\"get\"><input type=\"submit\" value=\"Backoffice\"></form></TH>\n"
              "<TH><form action=\"debug\" method=\"get\"><input type=\"submit\" value=\"Debug\"></form></TH>\n"
              "<TH><form action=\"indicators\" method=\"get\"><input type=\"submit\" value=\"Indicators\"></form></TH>\n"
              "<TH><form action=\"latency_histograms\" method=\"get\"><input type=\"submit\" value=\"Latency\"></form></TH>\n"
              "</TR></table>\n"
              "<p><b>Current (EP) equity is %s</b> jules<br>\n"
              ">> Gain (STR equity): %s (%s)<p>\n"
              "Current used margin is %s jules<br>\n"
              "Current margin utilization is %s<br>\n"
              "Current free margin is %s <br><p>\n"
              "%s<p>",
              _WEB_VERSION,
              stubLib_getVersion(),
              timeNow,
              CALLBACK_strtime.dayofweek,
              callbackNrStr,
              memMap->megatickSequence,
              INITIAL_timeStr,
              timeUp,
              memMap->AccountingUnit[auIndex].auId,
              userName,
              execName,
              auto_management_connectedS,
              auto_management_toggle_buttonS,
              strategy_connectedS,
              strategy_toggle_buttonS,
              (auto_management_connected ? "" : " disabled"),
              total_equity_EP_S,
              total_equity_STR_S,
              pctGainS,
              total_marginS,
              pctMarginUsedS,
              free_marginS,
              withinOperatingHoursS
      );

      if(ValuationChangeAlarm)
          strcat(buffer, "<strong style=\"background-color: red\">"
                         "Alarm!!: Too big change in valuation detected</strong>"
                         "<form action=\"reset_valuation_change_alarm\" method=\"get\"><input type=\"submit\" value=\"Reset alarm\"></form>\n"
                         "<form></form><p>\n"
                );

      if(DrawdownAlarm)
          strcat(buffer, "<strong style=\"background-color: red\">"
                         "Alarm!!: Excessive drawdown detected</strong>"
                         "<form action=\"reset_drawdown_alarm\" method=\"get\"><input type=\"submit\" value=\"Reset alarm\"></form>\n"
                         "<form></form><p>\n"
                );

      if(TooManyTradesAlarm)
          strcat(buffer, "<strong style=\"background-color: red\">"
                         "Alarm!!: Too many trades</strong>"
                         "<form action=\"reset_too_many_trades_alarm\" method=\"get\"><input type=\"submit\" value=\"Reset alarm\"></form>\n"
                         "<p>\n"
                );

      if(MalfunctionAlarm) {

          sprintf(line, "<strong style=\"background-color: red\">"
                        "Alarm!!: Malfunction: %s</strong>"
                        "<form action=\"reset_malfunction_alarm\" method=\"get\"><input type=\"submit\" value=\"Reset alarm\"></form>\n"
                        "<form></form><p>\n",
                        MalfunctionAlarmText
                );
          strcat(buffer, line);

      }

      strcat(buffer, "<p>\n");

      HTML_update_strategy_currency_shower("report");

      HTML_update_currency_shower("report");

      HTML_show_trading_interfaces();

      strcat(buffer, "<form action=\"reset_cards\" method=\"get\"><input type=\"submit\" value=\"Reset yellow and red cards\"></form>\n");

      strcat(buffer, "<p>\n");

      HTML_show_exposures();


      if(many_messages <= 0) {

          strcat(buffer, "(No notifications yet)\n");
      } else {

          strcat(buffer, "<table><TH><b>Notifications</b></TH>"
                         "<TH><form action=\"reset_notifications\" method=\"get\"><input type=\"submit\" value=\"Reset\"></TH>"
                         "</table></form>\n");
          for(int i=0; i<many_messages; i++) {
              char msg[SLEN];
              render_message_HTML(i, msg);
              sprintf(line, "%s<br>\n", msg);
              strcat(buffer,line);
          }

      }

      strcat(buffer, "<p>\n");

/*
      strcat(buffer, "<form action=\"add_notification\" method=\"post\">"
                     "<input type=\"text\" name=\"msg\">\n"
                     "<input type=\"radio\" name=\"type\" value=\"info\" checked>Info\n"
                     "<input type=\"radio\" name=\"type\" value=\"alarm\" checked>Alarm\n"
                     "<input type=\"radio\" name=\"type\" value=\"critical\" checked>Critical\n"
                     "<input type=\"submit\" value=\"Post notification\">"
                     "</form><p>\n"
            );
*/


// Up to here

      strcat(buffer, "</FONT>\n</body></html>\n");

      ret = 0;

  }



  void HTML_toggle_auto_management(void) {

      if(auto_management_connected) {

          auto_management_connected = false;
          strategy_connected = false;
          stubDbg(DEBUG_INFO_LEVEL, "--- Auto management toggled OFF\n");

      } else {

          auto_management_connected = true;
          stubDbg(DEBUG_INFO_LEVEL, "--- Auto management toggled ON\n");

      }
      // This must write the static variable in the strategy module!!!
      HTML_jump_to("report", 0);

  }


  void HTML_toggle_strategy(void) {

      if(strategy_connected) {
          strategy_connected = false;
          stubDbg(DEBUG_INFO_LEVEL, "--- Strategy toggled OFF\n");
      } else {
          strategy_connected = true;
          stubDbg(DEBUG_INFO_LEVEL, "--- Strategy toggled ON\n");
      }
      // This must write the static variable in the strategy module!!!
      HTML_jump_to("report", 0);

  }


  void HTML_reset_valuation_change_alarm(void) {
      ValuationChangeAlarm = false;
      HTML_jump_to("report", 0);
      add_message("### Valuation change alarm reset", STR_MSG_SYSTEM_INFO);
  }


  void HTML_reset_too_many_trades_alarm(void) {
      TooManyTradesAlarm = false;
      HTML_jump_to("report", 0);
      add_message("### Too many trades alarm reset", STR_MSG_SYSTEM_INFO);
  }


  void HTML_reset_drawdown_alarm(void) {
      DrawdownAlarm = false;
      HTML_jump_to("report", 0);
      add_message("### Drawdown alarm reset", STR_MSG_SYSTEM_INFO);
  }


  void HTML_reset_malfunction_alarm(void) {
      MalfunctionAlarm = false;
      HTML_jump_to("report", 0);
      add_message("### Malfunction alarm reset", STR_MSG_SYSTEM_INFO);
  }


  void HTML_reset_notifications(void) {
      clear_all_messages();
      HTML_jump_to("report", 0);
      add_message("--- Notifications reset", STR_MSG_SYSTEM_INFO);
  }


  void HTML_reset_cards(void) {
      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          tiOperation[ti].yellowCard=false;
          tiOperation[ti].yellowCardCounter=0;
          tiOperation[ti].redCard=0;
      }
      HTML_jump_to("report", 0);
      add_message("--- Yellow / red cards reset", STR_MSG_SYSTEM_INFO);
  }


  void HTML_add_notification(void) {

      char msg[SLEN]="";
      int type = 0;

      // Now parsing parameters


      for(int i = 0; i < post_data_index; i++) {

          if(!strcmp("msg", post_data[i].key)) {
              strcpy(msg, post_data[i].value);
          }

          if(!strcmp("type", post_data[i].key)) {
              type = atoi(post_data[i].value);
          }

          if(!strcmp("type", post_data[i].key)) {
              if(!strcmp("critical", post_data[i].value))
                  type = STR_MSG_CRITICAL;
              else if(!strcmp("alarm", post_data[i].value))
                  type = STR_MSG_ALARM;
              else if(!strcmp("info", post_data[i].value))
                  type = STR_MSG_TRADING_INFO;
              else
                  type = 0;
          }

      }

      add_message(msg, type);
      HTML_jump_to("report", 0);

  }



  void HTML_show_config(void) {

      sprintf(buffer,
              "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
              "<html><head></head><body>\n"
              "<FONT FACE=\"arial\">"
              "<b>Configuration area</b><br>\n"
              "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form><p>\n"
              "<i>- System parameters:</i><p>\n"
              "<form action=\"do_system_config\" method=\"post\">\n"
              "<table border=0>\n"
             );

      for(int i = 0; i < many_system_params; i++) {
          sprintf(line,
                  "<TR bgcolor=#151540><TH><font color=#FFFFFF>%s:</TH><TH><input type=\"text\" name=\"%s\" value = \"%d\"></TH></TR>\n",
                  system_params[i].name, system_params[i].name, system_params[i].value
                 );

          strcat(buffer, line);

      }

      sprintf(line,
              "</table>"
              "<input type=\"submit\" value=\"Submit system config\">\n"
              "</form><p>\n"
              "<i>- Strategy parameters:</i><p>\n"
              "<form action=\"do_strategy_config\" method=\"post\">\n"
              "<table border=0>\n"
             );

      strcat(buffer, line);

      for(int i = 0; i < many_strategy_params; i++) {
          sprintf(line,
                  "<TR bgcolor=#151540><TH><font color=#FFFFFF>%s:</TH><TH><input type=\"text\" name=\"%s\" value = \"%d\"></TH></TR>\n",
                  strategy_params[i].name, strategy_params[i].name, strategy_params[i].value
                 );

          strcat(buffer, line);

      }


      sprintf(line,
              "</table>"
              "<input type=\"submit\" value=\"Submit strategy config\">\n"
              "</form>\n"
              "<i>- TI marks:</i><p>\n"
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
              "<TH><font color=#FFFFFF>TI</TH>\n"
              "<TH><font color=#FFFFFF>Good for market execution?</TH>\n"
              "<TH><font color=#FFFFFF>Good for liquidity offering?</TH>\n"
              "<TH><font color=#FFFFFF>Good for hedging?</TH>\n"
              "</TR>\n"
             );

      strcat(buffer, line);

      for(int tiIndex=0; tiIndex<memMap->nTradingInterfaces; tiIndex++) {
          sprintf(line,
                  "<TR>\n"
                  "<TD>%s</TD>"

                  "<TD><table><tr><td>%s</td><td>"
                  "<form action=\"do_toggle_ti_mark\" method=\"post\">"
                  "<input type=\"hidden\" name=\"tiIndex\" value=\"%d\">"
                  "<input type=\"hidden\" name=\"whichMark\" value=\"goodForMarketExecution\">"
                  "<input type=\"submit\" value=\"Toggle\">"
                  "</form></td></tr></table></TD>"

                  "<TD><table><tr><td>%s</td><td>"
                  "<form action=\"do_toggle_ti_mark\" method=\"post\">"
                  "<input type=\"hidden\" name=\"tiIndex\" value=\"%d\">"
                  "<input type=\"hidden\" name=\"whichMark\" value=\"goodForLiquidityOffering\">"
                  "<input type=\"submit\" value=\"Toggle\">"
                  "</form></td></tr></table></TD>"

                  "<TD><table><tr><td>%s</td><td>"
                  "<form action=\"do_toggle_ti_mark\" method=\"post\">"
                  "<input type=\"hidden\" name=\"tiIndex\" value=\"%d\">"
                  "<input type=\"hidden\" name=\"whichMark\" value=\"goodForHedging\">"
                  "<input type=\"submit\" value=\"Toggle\">"
                  "</form></td></tr></table></TD>"

                  "</TR>\n",
                  memMap->tradingInterfaceName[tiIndex],
                  (goodForMarketExecution[tiIndex] ? "<strong style=\"background-color: lime\">Yes</strong>" : "<strong style=\"background-color: red\">No</strong>"),
                  tiIndex,
                  (goodForLiquidityOffering[tiIndex] ? "<strong style=\"background-color: lime\">Yes</strong>" : "<strong style=\"background-color: red\">No</strong>"),
                  tiIndex,
                  (goodForHedging[tiIndex] ? "<strong style=\"background-color: lime\">Yes</strong>" : "<strong style=\"background-color: red\">No</strong>"),
                  tiIndex
                 );
          strcat(buffer, line);

      } // Loop: trading interfaces


      sprintf(line,
              "</table>"
              "</FONT>\n</body></html>\n"
             );

      strcat(buffer, line);

      ret = 0;

  }


  void HTML_do_config(params_t *params) {

      // First, we complain if strategy is running

      if(strategy_connected) {

          sprintf(buffer,
                  "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
                  "<html><head></head><body>\n"
                  "<FONT FACE=\"arial\">"
                  "Whoa! Are you crazy??? This thing is connected!!!<p>\n Pls go back and at least stop the automatic strategy before attempting to change the configuration - you fool!<p>\n"
                  "Configuration <b>NOT</b> loaded<p>\n"
                  "<form action=\"config\" method=\"get\"><input type=\"submit\" value=\"Back to config\"></form>\n"
                  "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n"
                  "</FONT>\n</body></html>\n"
          );

          ret = 0;

          add_message("!!! Unsuccessful attempt to change config parameters", STR_MSG_ERROR);

      } else {

          // We parse parameters from the http_request and modify the variables
          int params_received = 0;

          for(int i = 0; i < post_data_index; i++) {

              int param;
              if(params == system_params) param = find_system_parameter(post_data[i].key);
              else param = find_strategy_parameter(post_data[i].key);

              if(param >= 0) {
                  strcpy(params[param].name, post_data[i].key);
                  params[param].value = atoll(post_data[i].value);
                  params_received++;

              }

          }


          // Then, we save the new configuration file
          save_parameters();

          // Finally, we cast a response

          sprintf(buffer,
                  "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
                  "<html><head></head><body>\n"
                  "<FONT FACE=\"arial\">"
                  "Configuration successfully loaded (%d params received)<p>\n"
                  "<form action=\"config\" method=\"get\"><input type=\"submit\" value=\"Back to config\"></form>\n"
                  "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n"
                  "</FONT>\n</body></html>\n",
                  params_received
          );

          add_message("--- Config parameters changed", STR_MSG_SYSTEM_INFO);

          ret = 0;

      }

  }


  void HTML_do_toggle_ti_mark(void) {

      idtype tiIndex=0;
      boolean tiIndex_set = false;

      char whichMark[SLEN];
      boolean whichMark_set = false;

      // Now parsing trade parameters

      for(int i = 0; i < post_data_index; i++) {

          if(!strcmp("tiIndex", post_data[i].key)) {
              tiIndex = atoll(post_data[i].value);
              if((tiIndex>=0) && (tiIndex<memMap->nTradingInterfaces)) tiIndex_set = true;
          }

          if(!strcmp("whichMark", post_data[i].key)) {
              whichMark_set = true;
              strcpy(whichMark, post_data[i].value);
          }

      }


      // Now to send the cancel

      if(tiIndex_set && whichMark_set) {
          if(!strcmp(whichMark, "goodForMarketExecution"))
              goodForMarketExecution[tiIndex]=!goodForMarketExecution[tiIndex];
          else if(!strcmp(whichMark, "goodForLiquidityOffering"))
              goodForLiquidityOffering[tiIndex]=!goodForLiquidityOffering[tiIndex];
          else if(!strcmp(whichMark, "goodForHedging"))
              goodForHedging[tiIndex]=!goodForHedging[tiIndex];

          save_ti_marks(memMap);
      }

      // We ignore any errors
      HTML_jump_to("config", 0);


  }




  void HTML_default_response(void) {

      sprintf(buffer,
              "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
              "<html><head></head><body>\n"
              "<FONT FACE=\"arial\">"
              "??? Unknown URL<p>\n"
              "</FONT>\n</body></html>\n"
      );

      ret = 1;

  }



  void HTML_show_trade(void) {

      char timeNow[SLEN];
      getStringTimeNowUTC(timeNow);

      sprintf(buffer,
              "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
              "<html><head></head><body>\n"
              "<FONT FACE=\"arial\">"
              "<b>Trading GUI</b> (glups)<p>\n"
              "Current (UTC) time is %s<p>",
              timeNow);
      if(macro_command_in_progress) {
          strcat(buffer, "<strong style=\"background-color: red\">"
                         "Macro command in progress!!</strong><br>\n");

      }
      strcat(buffer,
              "<table><TR>"
              "<form action=\"trade\" method=\"get\"><input type=\"submit\" value=\"Refresh\"></form>\n"
              "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n"
              "</TR></table><p>\n"
      );

      strcat(buffer,
              "<form action=\"do_trade\" method=\"post\">\n"
              "<table border=0>\n"
      );



      strcat(buffer, "<TR><TD>Trading Interface:</TD>\n<TD><select name=\"trading_interface\">\n");

      for(int i = 0; i < memMap->nTradingInterfaces; i++) {

          sprintf(line, "<option %s value=\"%d\">ti #%d: %s on pb #%d (%s, %s)</option>\n",
                  (memMap->tradingInterfaceOk[i] ? "style=\"color:black\"" : "style=\"color:gray\""),
                  i,
                  i,
                  memMap->tradingInterfaceName[i],
                  memMap->primeBrokerIndex[memMap->whichPrimeBroker[i]],
                  memMap->primeBrokerName[memMap->primeBrokerIndex[memMap->whichPrimeBroker[i]]],
                  (memMap->tradingInterfaceOk[i] ? "Ok" : "NOT Ok")
                 );

          strcat(buffer, line);
      }

      strcat(buffer, "</select></TD></TR>\n");

      strcat(buffer, "<TR><TD>... or use best available price</TD><TD><input name=\"best_available\" type=\"checkbox\" checked></TD></TR>\n");
      strcat(buffer, "<TR><TD>... and use opposite side as ref</TD><TD><input name=\"opposite_side\" type=\"checkbox\" checked></TD></TR><p>\n");


      strcat(buffer, "<TR><TD>Security:</TD><TD>\n<select name=\"security\">\n");

      for(int i = 0; i < many_securities; i++) {
          sprintf(line, "<option value=\"%d\">%s (%d)</option>\n", which_securities[i], securityName[which_securities[i]], which_securities[i]);
          strcat(buffer, line);
      }

      strcat(buffer, "</select></TD></TR>\n");



      strcat(buffer, "<TR><TD>Amount:</TD>\n"
                     "<TD><input type=\"text\" name=\"amount\" value=50000>\n"
                     "<input type=\"radio\" name=\"side\" value=\"buy\" checked>Buy"
                     "<input type=\"radio\" name=\"side\" value=\"sell\">Sell\n"
                     "</TD></TR>\n");



      strcat(buffer, "<TR><TD>Type:</TD>\n"
                     "<TD><input type=\"radio\" name=\"type\" value=\"market\">Market"
                     "<input type=\"radio\" name=\"type\" value=\"limit\" checked>Limit\n"
                     "<input type=\"text\" name=\"limit\" value=\"2\"> (1/10 pips worse than top of book)\n"
                     "</TD></TR>\n");



      strcat(buffer, "<TR><TD>TimeInForce:</TD>\n<TD><select name=\"timeInForce\">\n");

      sprintf(line, "<option value=\"%d\">Day</option>\n", TRADE_TIMEINFORCE_DAY); strcat(buffer, line);
      sprintf(line, "<option value=\"%d\">Good till Cancel</option>\n", TRADE_TIMEINFORCE_GOOD_TILL_CANCEL); strcat(buffer, line);
      sprintf(line, "<option value=\"%d\">Immediate or Cancel</option>\n", TRADE_TIMEINFORCE_INMED_OR_CANCEL); strcat(buffer, line);
      sprintf(line, "<option value=\"%d\">Fill or Kill</option>\n", TRADE_TIMEINFORCE_FILL_OR_KILL); strcat(buffer, line);
      sprintf(line, "<option value=\"%d\">Good till Date</option>\n", TRADE_TIMEINFORCE_DATE); strcat(buffer, line);
      sprintf(line, "<option value=\"%d\" selected>Good for Seconds</option>\n", TRADE_TIMEINFORCE_GOOD_FOR_SECS); strcat(buffer, line);
      sprintf(line, "<option value=\"%d\">Good for Milliseconds</option>\n", TRADE_TIMEINFORCE_GOOD_FOR_MSECS); strcat(buffer, line);

      strcat(buffer, "</select>\n");

      strcat(buffer, "<input type=\"text\" name=\"seconds\" value=\"1\"> seconds\n");

      strcat(buffer, "</TD></TR>\n");

      strcat(buffer, "<TR><TD>Unconditional:</TD>\n"
                     "<TD><input name=\"unconditional\" type=\"checkbox\" checked>\n"
                     "</TD></TR>\n"
      );

      strcat(buffer, "<TR><TD>Reserved data: <input type=\"text\" name=\"reserved\" value=\"0\"></TD></TR>\n");


      strcat(buffer, "</table>\n<input type=\"submit\" name=\"what_to_do\" value=\"Send\">\n");
      strcat(buffer, "\n<input type=\"submit\" name=\"what_to_do\" value=\"Simulate\"><p>\n");
      strcat(buffer, "</form><p>\n");

      HTML_update_currency_shower("trade");

      HTML_show_sorted_prices(currentCurrency);

      strcat(buffer, "<p>\n");

      HTML_show_exposures();

      strcat(buffer, "<form action=\"close_all\" method=\"get\"><input type=\"submit\" value=\"Close all open positions\"></form>\n");

      strcat(buffer, "<p>\n");

      HTML_show_trades_alive();

      strcat(buffer, "<p>\n");

      HTML_show_trades_historical();

      strcat(buffer, "<p>\n");


      strcat(buffer, "</FONT>\n</body></html>\n");


  }



  void HTML_do_trade(void) {

      idtype trading_interface_index = -1;
      idtype trading_interface_ID = -1;
      boolean best_available = false;
      idtype security = -1;
      int32 amount = 0;
      int side = -1;
      int type = -1;
      int limit = 0;
      boolean opposite_side = false;
      int timeInForce = -1;
      int seconds = -1;
      boolean unconditional = false;
      int reserved_data = -1;
      int what_to_do = -1;
#define do_trade_send 1
#define do_trade_simulate 2

      boolean trading_interface_set = false;
      boolean security_set = false;
      boolean amount_set = false;
      boolean side_set = false;
      boolean type_set = false;
      boolean limit_set = false;
      boolean timeInForce_set = false;
      boolean seconds_set = false;
      boolean what_to_do_set = false;


      // Now parsing trade parameters


      for(int i = 0; i < post_data_index; i++) {

          if(!strcmp("trading_interface", post_data[i].key)) {
              trading_interface_set = true;
              trading_interface_index = atoi(post_data[i].value);
              trading_interface_ID = memMap->tradingInterface[trading_interface_index];
          }

          if(!strcmp("best_available", post_data[i].key)) {
              if(!strcmp("on", post_data[i].value)) {
                  best_available = true;
              }
          }

          if(!strcmp("opposite_side", post_data[i].key)) {
              if(!strcmp("on", post_data[i].value)) {
                  opposite_side = true;
              }
          }

          if(!strcmp("security", post_data[i].key)) {
              security_set = true;
              security = atoi(post_data[i].value);
          }

          if(!strcmp("amount", post_data[i].key)) {
              amount_set = true;
              amount = atoll(post_data[i].value);
          }

          if(!strcmp("side", post_data[i].key)) {
              side_set = true;
              if(!strcmp("buy", post_data[i].value))
                  side = TRADE_SIDE_BUY;
              else
                  side = TRADE_SIDE_SELL;
          }

          if(!strcmp("type", post_data[i].key)) {
              if(!strcmp("market", post_data[i].value)) {
                  type = TRADE_TYPE_MARKET;
                  type_set = true;
              } else if(!strcmp("limit", post_data[i].value)) {
                  type = TRADE_TYPE_LIMIT;
                  type_set = true;
              }
          }

          if(!strcmp("limit", post_data[i].key)) {
              limit_set = true;
              limit = atoi(post_data[i].value);
          }

          if(!strcmp("timeInForce", post_data[i].key)) {
              timeInForce_set = true;
              timeInForce = atoi(post_data[i].value);
          }

          if(!strcmp("seconds", post_data[i].key)) {
              seconds_set = true;
              seconds = atoi(post_data[i].value);
          }

          if(!strcmp("unconditional", post_data[i].key)) {
              if(!strcmp("on", post_data[i].value)) {
                  unconditional = true;
              }
          }

          if(!strcmp("reserved", post_data[i].key)) {
              reserved_data = atoi(post_data[i].value);
          }

          if(!strcmp("what_to_do", post_data[i].key)) {
              if(!strcmp("Send", post_data[i].value)) {
                  what_to_do = do_trade_send;
                  what_to_do_set = true;
              } else if(!strcmp("Simulate", post_data[i].value)) {
                  what_to_do = do_trade_simulate;
                  what_to_do_set = true;
              }
          }


      }


      // Now to send the trade
      tradeCommand_t trade;

      if(!best_available) {

          trade.tiId = trading_interface_ID;

          if (side == TRADE_SIDE_BUY) {
              if(opposite_side) {
                  trade.limitPrice = memMap->mapTiTbBid[trading_interface_index][security].price + limit;
              } else {
                  trade.limitPrice = memMap->mapTiTbAsk[trading_interface_index][security].price + limit;
              }
          } else {
              if(opposite_side) {
                  trade.limitPrice = memMap->mapTiTbAsk[trading_interface_index][security].price - limit;
              } else {
                  trade.limitPrice = memMap->mapTiTbBid[trading_interface_index][security].price - limit;
              }
          }

      } else {

          if((side == TRADE_SIDE_BUY) && (memMap->mapSortAskNumberOfTi[security] > 0)) {

              if(!opposite_side) {

                  trade.tiId = memMap->tradingInterface[memMap->mapSortingAsk[security][0].tiIndex];
                  trade.limitPrice = memMap->mapTiTbAsk[memMap->mapSortingAsk[security][0].tiIndex][security].price + limit;

              } else {

                  trade.tiId = memMap->tradingInterface[memMap->mapSortingBid[security][memMap->mapSortBidNumberOfTi[security] - 1].tiIndex];
                  trade.limitPrice = memMap->mapTiTbBid[memMap->mapSortingBid[security][memMap->mapSortBidNumberOfTi[security] - 1].tiIndex][security].price + limit;

              }

          } else if((side == TRADE_SIDE_SELL) && (memMap->mapSortBidNumberOfTi[security] > 0)) {

              if(!opposite_side) {

                  trade.tiId = memMap->tradingInterface[memMap->mapSortingBid[security][0].tiIndex];
                  trade.limitPrice = memMap->mapTiTbBid[memMap->mapSortingBid[security][0].tiIndex][security].price - limit;

              } else {

                  trade.tiId = memMap->tradingInterface[memMap->mapSortingAsk[security][memMap->mapSortAskNumberOfTi[security] - 1].tiIndex];
                  trade.limitPrice = memMap->mapTiTbAsk[memMap->mapSortingAsk[security][memMap->mapSortAskNumberOfTi[security] - 1].tiIndex][security].price - limit;

              }

          } else {

              trade.tiId = -1; // No available trading interface

          }
      }

      if(trade.tiId >= 0) {

      }


      trade.security = security;
      trade.quantity = amount;
      trade.side = side;
      trade.tradeType = type;
//      trade.maxShowQuantity = amount
      trade.timeInForce = timeInForce;
      if(timeInForce == TRADE_TIMEINFORCE_GOOD_FOR_SECS) {
          trade.seconds = seconds;
//        trade.mseconds = 0;
      } else if(timeInForce == TRADE_TIMEINFORCE_GOOD_FOR_MSECS) {
//        trade.seconds = 0;
          trade.mseconds = seconds;
      }
//    trade.expiration = {0};
      trade.unconditional = unconditional;
      trade.checkWorstCase = false;
      trade.reservedData = reserved_data;


      // And now to send the trade

      if(
              trading_interface_set &&
              security_set &&
              amount_set &&
              side_set &&
              type_set &&
              limit_set &&
              timeInForce_set &&
              seconds_set &&
              what_to_do_set
        ) {

          char timeInForceS[SLEN];
          getTimeInForceString (trade.timeInForce, timeInForceS);

          sprintf(buffer,
                  "   trading_interface is %d (%s)\n"
                  "   security is %d (%s)\n"
                  "   amount is %d\n"
                  "   side is %s\n"
                  "   type is %s\n"
                  "   (relative) limit is %d (absolute limit price is %d)\n"
                  "   timeInForce is %s\n"
                  "   seconds is %d\n"
                  "   mseconds is %d\n"
                  "   unconditional is %s\n"
                  "   reserved data is %d\n",
                  trade.tiId,
                  memMap->tradingInterfaceName[memMap->tradingInterfaceIndex[trade.tiId]],
                  trade.security,
                  securityName[trade.security],
                  trade.quantity,
                  ((trade.side == TRADE_SIDE_BUY) ? "buy" : "sell"),
                  ((trade.tradeType == TRADE_TYPE_MARKET) ? "market" : "limit"),
                  limit,
                  trade.limitPrice,
                  timeInForceS,
                  trade.seconds,
                  trade.mseconds,
                  (trade.unconditional ? "on" : "off"),
                  trade.reservedData
          );

          if(trade.tiId >=0) {

              if(what_to_do == do_trade_send) {

                  stubDbg (DEBUG_INFO_LEVEL, ">>> Sending trade to core:\n%s\n", buffer);

                  uint32 tradeStrID = 0;
                  boolean succeeded;
                  int32 result = mySendTradeToCore(memMap, memMap->strUser, &trade, &tradeStrID, &succeeded);
                  stubDbg (DEBUG_INFO_LEVEL, ">>> Trade sent, result = %d, tradeStrID = %d\n\n", result, tradeStrID);
                  if(!succeeded) {
                      stubDbg (DEBUG_INFO_LEVEL, "!!! Trade did not succeed (in the call to mySendTradeToCore, result = %d, tradeStrID = %d )\n\n", result, tradeStrID);
                  }
                  print_prices(memMap, trade.security, true, true);
                  print_prices_full_book(memMap, trade.security);

                  // Now to refresh trade window rather than go to a confirmation page

                  HTML_jump_to("trade", 1);

              } else if(what_to_do == do_trade_simulate) {

                  stubDbg (DEBUG_INFO_LEVEL, ">>> Simulating trade:\n%s\n", buffer);

                  number AUfreemargin = 0;
                  number EPfreemargin = 0;
                  number PBfreemargin = 0;
                  boolean tradeResult;
                  idtype pbID = memMap->whichPrimeBroker[trading_interface_index];
                  idtype pb = memMap->primeBrokerIndex[pbID];

                  int32 result = SimulateTrade (memMap, memMap->strUser, &trade, &AUfreemargin, &EPfreemargin, &PBfreemargin, &tradeResult);

                  stubDbg (DEBUG_INFO_LEVEL, ">>> Trade simulated:\n"
                                             "    Result = %s\n"
                                             "    AU free margin = %.0f\n"
                                             "    EP free margin = %.0f\n"
                                             "    PB free margin = %.0f\n"
                                             "    Value returned = %d\n",
                                             (tradeResult ? "CAN BE DONE" : "CANNOT BE DONE"),
                                             AUfreemargin,
                                             EPfreemargin,
                                             PBfreemargin,
                                             result
                          );

                  stubDbg (DEBUG_INFO_LEVEL, "Margin config parameters for PB #%d (%s, ID = %d):\n"
                                             "mlea = %.8f\n"
                                             "mgum = %.8f\n"
                                             "mmum = %.8f\n",
                                             pb,
                                             memMap->primeBrokerName[pb],
                                             pbID,
                                             memMap->AccountingUnit[auIndex].mlea,
                                             memMap->EquityPool[epIndex].mgum,
                                             memMap->EquityPool[epIndex].mmum
                          );


                  sprintf(buffer,
                                "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
                                "<html><head></head><body>\n"
                                "<FONT FACE=\"arial\">"
                                "<b>The following trade was simulated:</b><p>\n"
                                "trading_interface is %d<br>\n"
                                "security is %d<br>\n"
                                "amount is %d<br>\n"
                                "side is %s<br>\n"
                                "type is %s<br>\n"
                                "(relative) limit is %d (absolute limit price is %d)<br>\n"
                                "timeInForce is %d<br>\n"
                                "seconds is %d<br>\n"
                                "unconditional is %s<br>\n"
                                "reserved data is %d<p>\n",
                                trading_interface_index,
                                security,
                                amount,
                                ((side == TRADE_SIDE_BUY) ? "buy" : "sell"),
                                ((type == TRADE_TYPE_MARKET) ? "market" : ((type == TRADE_TYPE_LIMIT) ? "limit" : "unknown")),
                                limit,
                                trade.limitPrice,
                                timeInForce,
                                seconds,
                                (unconditional ? "on" : "off"),
                                reserved_data
                        );

                  sprintf(line, "<b>And the result is:</b><p>"
                                "Result = %s<br>\n"
                                "AU free margin = %.0f<br>\n"
                                "EP free margin = %.0f<br>\n"
                                "PB free margin = %.0f<br>\n"
                                "Value returned = %d<p>\n",
                                (tradeResult ? "CAN BE DONE" : "CANNOT BE DONE"),
                                AUfreemargin,
                                EPfreemargin,
                                PBfreemargin,
                                result
                          );

                  strcat(buffer, line);

                  sprintf (line, "<b>Margin config parameters for PB #%d (%s):</b><p>\n"
                                 "mlea = %.4f<br>\n"
                                 "mgum = %.4f<br>\n"
                                 "mmum = %.4f<p>\n",
                                 pb,
                                 memMap->primeBrokerName[pb],
                                 memMap->AccountingUnit[auIndex].mlea,
                                 memMap->EquityPool[epIndex].mgum,
                                 memMap->EquityPool[epIndex].mmum
                          );

                  strcat(buffer, line);

                  strcat(buffer, "<form action=\"trade\" method=\"get\"><input type=\"submit\" value=\"Back to trading page\"></form>\n");

                  strcat(buffer, "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n");

                  strcat(buffer, "</FONT>\n</body></html>\n");

              } // do_trade or do_simulate

          } else {

              stubDbg (DEBUG_INFO_LEVEL, "!!! No available TIs found to route trade:\n%s\n", buffer);

              sprintf(buffer,
                            "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
                            "<html><head></head><body>\n"
                            "<FONT FACE=\"arial\">"
                            "<b>The trade was not sent, as the best_available flag was set and no TIs were available in the sorting array:</b><p>\n"
                            "<form action=\"trade\" method=\"get\"><input type=\"submit\" value=\"Back to trading page\"></form>\n"
                            "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n"
                            "</FONT>\n</body></html>\n"
                      );


          } // trade.tiId >= 0

      } else {

          sprintf(buffer,
                        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
                        "<html><head></head><body>\n"
                        "<FONT FACE=\"arial\">"
                        "A very strange error was detected (in HTML_do_trade) - some parameter was not set (?)<p>\n"
                        "trading_interface is %d<br>\n"
                        "security is %d<br>\n"
                        "amount is %d<br>\n"
                        "side is %s<br>\n"
                        "type is %s<br>\n"
                        "(relative) limit is %d (absolute limit price is %d)<br>\n"
                        "timeInForce is %d<br>\n"
                        "seconds is %d<br>\n"
                        "unconditional is %s<br>\n"
                        "reserved data is %d<br>\n"
                        "what to do is %d",
                        trading_interface_index,
                        security,
                        amount,
                        ((side == TRADE_SIDE_BUY) ? "buy" : "sell"),
                        ((type == TRADE_TYPE_MARKET) ? "market" : ((type == TRADE_TYPE_LIMIT) ? "limit" : "unknown")),
                        limit,
                        trade.limitPrice,
                        timeInForce,
                        seconds,
                        (unconditional ? "on" : "off"),
                        reserved_data,
                        what_to_do
                );

          strcat(buffer, "<form action=\"trade\" method=\"get\"><input type=\"submit\" value=\"Back to trading page\"></form>\n");

          strcat(buffer, "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n");

          strcat(buffer, "</FONT>\n</body></html>\n");

      }





      /* Good to detect origin IP for example

            // Now to resolve trade
            char kk[SLEN];
            sprintf(line, "");
            // We first read parameters
            int print_out_key (void *cls, enum MHD_ValueKind kind,
                               const char *key, const char *value)
            {
              sprintf (kk, "%s: %s\n", key, value);
              strcat(line, kk);
              return MHD_YES;
            }

            MHD_get_connection_values (connection, 0xFF, &print_out_key, NULL);

      */



  }



  void HTML_do_cancel(void) {

      char which_trade[MAX_TRADE_ID_STRING_SIZE]={0};
      idtype ti_index=0;

      boolean which_trade_set = false;
      boolean ti_index_set = false;

      // Now parsing trade parameters

      for(int i = 0; i < post_data_index; i++) {

          if(!strcmp("which_trade", post_data[i].key)) {
              which_trade_set = true;
              strcpy(which_trade, post_data[i].value);
          }

          if(!strcmp("ti_index", post_data[i].key)) {
              ti_index_set = true;
              ti_index = atoll(post_data[i].value);
          }

      }


      // Now to send the cancel

      if(which_trade_set && ti_index_set) {

          stubDbg (DEBUG_INFO_LEVEL, ">>> Sending cancel to core for trade ID %s, TI #%d (%s)\n",
                  which_trade, ti_index, memMap->tradingInterfaceName[ti_index]);
          int32 result = myCancelTradeToCore(memMap, memMap->strUser, which_trade, ti_index);
          stubDbg (DEBUG_INFO_LEVEL, ">>> Cancel sent, result = %d\n\n", result);

          // Now to refresh trade window rather than go to a confirmation page

          HTML_jump_to("trade", 1);

      } else {

          sprintf(buffer,
                        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
                        "<html><head></head><body>\n"
                        "<FONT FACE=\"arial\">"
                        "A very strange error was detected (in HTML_do_cancel) - the tradeID parameter was not set (?)<p>\n"
                );

          strcat(buffer, "<form action=\"trade\" method=\"get\"><input type=\"submit\" value=\"Back to trading page\"></form>\n");

          strcat(buffer, "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n");

          strcat(buffer, "</FONT>\n</body></html>\n");

      }


  }



  void HTML_do_replace(void) {

      char which_trade[MAX_TRADE_ID_STRING_SIZE]={0};
      uint32 new_quantity=0;
      uint32 new_price=0;
      idtype ti_index=0;

      boolean which_trade_set = false;
      boolean new_quantity_set = false;
      boolean new_price_set = false;
      boolean ti_index_set = false;

      // Now parsing trade parameters

      for(int i = 0; i < post_data_index; i++) {

          if(!strcmp("which_trade", post_data[i].key)) {
              which_trade_set = true;
              strcpy(which_trade, post_data[i].value);
          }

          if(!strcmp("new_quantity", post_data[i].key)) {
              new_quantity_set = true;
              new_quantity = atoll(post_data[i].value);
          }

          if(!strcmp("new_price", post_data[i].key)) {
              new_price_set = true;
              new_price = atoll(post_data[i].value);
          }

          if(!strcmp("ti_index", post_data[i].key)) {
              ti_index_set = true;
              ti_index = atoll(post_data[i].value);
          }

      }


      // Now to send the replace

      if(which_trade_set && new_quantity_set && new_price_set && ti_index_set) {

          stubDbg (DEBUG_INFO_LEVEL, ">>> Sending replace to core for trade ID %s\n  - New price is %d\n  - New quantity is %d\n",
                   which_trade, new_price, new_quantity);
          int32 result = myModifyTradeToCore(memMap, memMap->strUser, which_trade, ti_index, new_price, new_quantity, true);
          stubDbg (DEBUG_INFO_LEVEL, ">>> Replace sent, result = %d\n\n", result);

          // Now to refresh trade window rather than go to a confirmation page

          HTML_jump_to("trade", 1);

      } else {

          sprintf(buffer,
                        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
                        "<html><head></head><body>\n"
                        "<FONT FACE=\"arial\">"
                        "A very strange error was detected (in HTML_do_replace):<p>\n"
                  );
          if(!which_trade_set) strcat(buffer, "   - which_trade parameter was not set!<n>");
          if(!new_quantity_set) strcat(buffer, "   - new_quantity parameter was not set!<n>");
          if(!new_price_set) strcat(buffer, "   - new_price parameter was not set!<n>");

          strcat(buffer, "<form action=\"trade\" method=\"get\"><input type=\"submit\" value=\"Back to trading page\"></form>\n");

          strcat(buffer, "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n");

          strcat(buffer, "</FONT>\n</body></html>\n");

      }


  }



  void HTML_do_cancel_all(void) {

      macro_command_cancel_all_pending_orders_in_AU(memMap);

      HTML_jump_to("trade", 1);

  }


  void HTML_do_close_all(void) {

      macro_command_close_all_posis_in_AU(memMap);

      HTML_jump_to("trade", 1);

  }


  void HTML_do_resolve(void) {

      char which_trade[MAX_TRADE_ID_STRING_SIZE]={0};
      int32 quantity = 0;
      uint32 price = 0;

      boolean which_trade_set = false;
      boolean quantity_set = false;
      boolean price_set = false;

      // Now parsing trade parameters

      for(int i = 0; i < post_data_index; i++) {

          if(!strcmp("which_trade", post_data[i].key)) {
              which_trade_set = true;
              strcpy(which_trade, post_data[i].value);
          }

          if(!strcmp("quantity", post_data[i].key)) {
              quantity_set = true;
              quantity = atoll(post_data[i].value);
          }

          if(!strcmp("price", post_data[i].key)) {
              price_set = true;
              price = atoll(post_data[i].value);
          }

      }


      // Now to do the resolve
      if(which_trade_set && quantity_set && price_set) {

          stubDbg (DEBUG_INFO_LEVEL, ">>> Resolving indetermined trade, trade ID is %s, quantity is %d, price is %d\n",
                   which_trade, quantity, price);

          int alive_remaining = memMap->AccountingUnit[auIndex].numberOfAlives;
          boolean foundit = false;
          tradeCommand_t tc;
          for(int t=0; (t<MAX_TRADES_LIST_ALIVE) && (alive_remaining > 0); t++) {
              if(memMap->AccountingUnit[auIndex].alive[t].alive) {
                  if(!strcmp(memMap->AccountingUnit[auIndex].alive[t].ids.fixId, which_trade)) {
                      tc = memMap->AccountingUnit[auIndex].alive[t].params;
                      foundit = true;
                      break;
                  }
              }
          }

          if(foundit) {
              tc.finishedQuantity = quantity;
              tc.finishedPrice = price;
              int result=contextTradeResolve(memMap->AccountingUnit[auIndex].auId, &tc, which_trade);
              stubDbg (DEBUG_INFO_LEVEL, ">>> Resolve sent, result=%d\n\n", result);

          } else {

              sprintf(macro_command_output_msg, "Trade not found - %s", which_trade);

          }



          // Now to refresh trade window rather than go to a confirmation page
          HTML_jump_to("trade", 1);

      } else {

          sprintf(buffer,
                        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
                        "<html><head></head><body>\n"
                        "<FONT FACE=\"arial\">"
                        "A very strange error was detected (in HTML_do_replace):<p>\n"
                  );
          if(!which_trade_set) strcat(buffer, "   - which_trade parameter was not set!<n>");
          if(!quantity_set) strcat(buffer, "   - quantity parameter was not set!<n>");
          if(!price_set) strcat(buffer, "   - price parameter was not set!<n>");

          strcat(buffer, "<form action=\"trade\" method=\"get\"><input type=\"submit\" value=\"Back to trading page\"></form>\n");

          strcat(buffer, "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n");

          strcat(buffer, "</FONT>\n</body></html>\n");

      }


  }



  void HTML_show_backoffice(void) {

      sprintf(buffer,
              "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
              "<html><head></head><body>\n"
              "<FONT FACE=\"arial\">"
              "<b>Backoffice proto-GUI</b><p>\n"
              "<table><TR>"
              "<form action=\"backoffice\" method=\"get\"><input type=\"submit\" value=\"Refresh\"></form>\n"
              "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n"
              "<form action=\"do_initialize_all_bo\" method=\"post\"><input type=\"submit\" value=\"Initialize all PBs to \">"
              "<input type=\"text\" name=\"amount_to_initialize_all\" value=20000> EUR</form>\n"
              "</TR></table><p>\n"
      );


      strcat(buffer,
              "<b>Cash changes:</b><p>\n"
              "<form action=\"do_backoffice_change_cash\" method=\"post\">\n"
              "<table border=0>\n"
      );

      strcat(buffer, "<TR><TD>Prime broker:</TD>\n<TD><select name=\"prime_broker\">\n");

      for(int i = 0; i < memMap->nPrimeBrokers; i++) {

          sprintf(line, "<option value=\"%d\">PB #%d: %s</option>\n",
                  i,
                  i,
                  memMap->primeBrokerName[i]
                 );

          strcat(buffer, line);
      }

      strcat(buffer, "</select></TD></TR>\n");


      strcat(buffer, "<TR><TD>Currency:</TD><TD>\n<select name=\"asset\">\n");

      for(int i = 0; i < many_assets; i++) {
          sprintf(line, "<option value=\"%d\">%s (%d)</option>\n", which_assets[i], assetName[which_assets[i]], which_assets[i]);
          strcat(buffer, line);
      }

      strcat(buffer, "</select></TD></TR>\n");



      strcat(buffer, "<TR><TD>Amount:</TD>\n"
                     "<TD><input type=\"text\" name=\"amount\" value=0>\n"
             );

      sprintf(line,  "<TR><TD>Accounting unit (this one by default):</TD>\n"
                     "<TD><input type=\"text\" name=\"accounting_unit\" value=%d>\n",
                     memMap->AccountingUnit[auIndex].auId);
      strcat(buffer, line);

      strcat(buffer, "<input type=\"radio\" name=\"what_to_do\" value=\"add\" checked>Add\n"
                     "<input type=\"radio\" name=\"what_to_do\" value=\"substract\">Substract\n"
                     "<input type=\"radio\" name=\"what_to_do\" value=\"set_to\">Set to\n"
                     "<input type=\"radio\" name=\"what_to_do\" value=\"clean\">Clean\n"
                     "</TD></TR>\n");

      strcat(buffer, "</table>\n<input type=\"submit\" value=\"Send cash change\"></form><p>\n");





      strcat(buffer,
              "<b>Add positions:</b><p>\n"
              "<form action=\"do_backoffice_change_posis\" method=\"post\">\n"
              "<table border=0>\n"
      );

      strcat(buffer, "<TR><TD>Trading Interface:</TD>\n<TD><select name=\"trading_interface\">\n");

      for(int i = 0; i < memMap->nTradingInterfaces; i++) {

          sprintf(line, "<option %s value=\"%d\">ti #%d: %s on pb #%d (%s, %s)</option>\n",
                  (memMap->tradingInterfaceOk[i] ? "style=\"color:black\"" : "style=\"color:gray\""),
                  i,
                  i,
                  memMap->tradingInterfaceName[i],
                  memMap->primeBrokerIndex[memMap->whichPrimeBroker[i]],
                  memMap->primeBrokerName[memMap->primeBrokerIndex[memMap->whichPrimeBroker[i]]],
                  (memMap->tradingInterfaceOk[i] ? "Ok" : "NOT Ok")
                 );

          strcat(buffer, line);
      }

      strcat(buffer, "</select></TD></TR>\n");


      strcat(buffer, "<TR><TD>Security:</TD><TD>\n<select name=\"security\">\n");

      for(int i = 0; i < many_securities; i++) {
          sprintf(line, "<option value=\"%d\">%s (%d)</option>\n", which_securities[i], securityName[which_securities[i]], which_securities[i]);
          strcat(buffer, line);
      }

      strcat(buffer, "</select></TD></TR>\n");


      strcat(buffer, "<TR><TD>Amount:</TD>\n"
                     "<TD><input type=\"text\" name=\"amount\" value=0>\n"
                     "<input type=\"radio\" name=\"side\" value=\"buy\" checked>Buy"
                     "<input type=\"radio\" name=\"side\" value=\"sell\">Sell\n"
                     "</TD></TR>\n"
                     "<TR><TD>Price:</TD>\n"
                     "<TD><input type=\"text\" name=\"price\" value=0>\n"
                     "</TD></TR>\n");

      sprintf(line,  "<TR><TD>Accounting unit (this one by default):</TD>\n"
                     "<TD><input type=\"text\" name=\"accounting_unit\" value=%d></TD></TR>\n",
                     memMap->AccountingUnit[auIndex].auId);
      strcat(buffer, line);

      strcat(buffer, "</table>\n<input type=\"submit\" value=\"Add position\"></form><p>\n");



      HTML_show_exposures();

      strcat(buffer, "</FONT>\n</body></html>\n");

  }



  void HTML_do_backoffice_change_cash(void) {

      idtype prime_broker_index = -1;
      idtype asset = -1;
      idtype auId = -1;
      int32 amount = 0;
      int what_to_do = -1;
      #define add_bo 1
      #define substract_bo 2
      #define set_to_bo 3
      #define clean_bo 4

      boolean prime_broker_set = false;
      boolean asset_set = false;
      boolean auId_set = false;
      boolean amount_set = false;
      boolean what_to_do_set = false;


      // Now parsing backoffice change parameters

      for(int i = 0; i < post_data_index; i++) {

          if(!strcmp("prime_broker", post_data[i].key)) {
              prime_broker_set = true;
              prime_broker_index = atoi(post_data[i].value);
          }

          if(!strcmp("asset", post_data[i].key)) {
              asset_set = true;
              asset = atoi(post_data[i].value);
          }

          if(!strcmp("accounting_unit", post_data[i].key)) {
              auId_set = true;
              auId = atoi(post_data[i].value);
          }

          if(!strcmp("amount", post_data[i].key)) {
              amount_set = true;
              amount = atoll(post_data[i].value);
          }

          if(!strcmp("what_to_do", post_data[i].key)) {
              what_to_do_set = true;
              if(!strcmp("add", post_data[i].value))
                  what_to_do = add_bo;
              else if(!strcmp("substract", post_data[i].value))
                  what_to_do = substract_bo;
              else if(!strcmp("set_to", post_data[i].value))
                  what_to_do = set_to_bo;
              else if(!strcmp("clean", post_data[i].value))
                  what_to_do = clean_bo;
              else what_to_do_set = false;
          }

      } // (post) args


      // Now to do the backoffice change

      if(
              prime_broker_set &&
              asset_set &&
              auId_set &&
              amount_set &&
              what_to_do_set
        ) {

          char what_to_do_name[SLEN];

          switch(what_to_do) {
              case add_bo : sprintf(what_to_do_name, "Add"); break;
              case substract_bo : sprintf(what_to_do_name, "Substract"); break;
              case set_to_bo : sprintf(what_to_do_name, "Set to"); break;
              case clean_bo : sprintf(what_to_do_name, "Clean"); break;
              default : sprintf(what_to_do_name, "Unknown!!"); break;
          }

          sprintf(buffer,
                  "   Prime broker is %d\n"
                  "   asset is %d\n"
                  "   accounting unit is %d\n"
                  "   amount is %d\n"
                  "   what to do is '%s'\n",
                  prime_broker_index,
                  asset,
                  auId,
                  amount,
                  what_to_do_name
          );

          stubDbg (DEBUG_INFO_LEVEL, ">>> Sending backoffice change to core:\n%s\n", buffer);

          int32 result = -1;

switch (what_to_do) {

              case add_bo :
                  result = contextCashAdd (memMap->primeBroker[prime_broker_index], auId, asset, (number) amount);
                  stubDbg (DEBUG_INFO_LEVEL, "   This was an 'add' => Function called was contextCashAdd()\n");
                  break;
              case substract_bo :
                  result = contextCashAdd (memMap->primeBroker[prime_broker_index], auId, asset, (number) - amount);
                  stubDbg (DEBUG_INFO_LEVEL, "   This was a 'substract' => Function called was contextCashAdd()\n");
                  break;
              case set_to_bo :
                  result = contextCashChange(memMap->primeBroker[prime_broker_index], auId, asset, (number) amount);
                  stubDbg (DEBUG_INFO_LEVEL, "   This was a 'set to' => Function called was contextCashChange()\n");
                  break;
              case clean_bo :
                  result = contextCashClean(memMap->primeBroker[prime_broker_index], auId, asset);
                  stubDbg (DEBUG_INFO_LEVEL, "   This was an 'clean' => Function called was contextCashClean()\n");
                  break;

/*
              case add_bo :
                  if(auId == memMap->auId) {
                      add_cash_to_AU_PB(memMap, memMap->primeBroker[prime_broker_index], asset, (number) amount);
                  } else {
                      int result = contextCashAdd (memMap->primeBroker[prime_broker_index], auId, asset, (number) amount);
                      stubDbg (DEBUG_INFO_LEVEL, "   Called directly contextCashAdd() as this is another auId, result is %d\n", result);
                  }
                  break;
              case substract_bo :
                  if(auId == memMap->auId) {
                      add_cash_to_AU_PB(memMap, memMap->primeBroker[prime_broker_index], asset, (number) - amount);
                  } else {
                      int result = contextCashAdd (memMap->primeBroker[prime_broker_index], auId, asset, (number) - amount);
                      stubDbg (DEBUG_INFO_LEVEL, "   Called directly contextCashAdd() as this is another auId, result is %d\n", result);
                  }
                  break;
              case set_to_bo :
                  if(auId == memMap->auId) {
                      set_cash_to_AU_PB(memMap, memMap->primeBroker[prime_broker_index], asset, (number) amount);
                  } else {
                      int result = contextCashChange(memMap->primeBroker[prime_broker_index], auId, asset, (number) amount);
                      stubDbg (DEBUG_INFO_LEVEL, "   Called directly contextCashChange() as this is another auId, result is %d\n", result);
                  }
                  break;
              case clean_bo :
                  if(auId == memMap->auId) {
                      set_cash_to_AU_PB(memMap, memMap->primeBroker[prime_broker_index], asset, 0);
                  } else {
                      int result = contextCashClean(memMap->primeBroker[prime_broker_index], auId, asset);
                      stubDbg (DEBUG_INFO_LEVEL, "   Called directly contextCashClean() as this is another auId, result is %d\n", result);
                  }
                  break;
*/

              default : stubDbg (DEBUG_INFO_LEVEL, "!!! Whoa!! what_to_do was set to %d and therefore no call was made!\n\n", what_to_do);
          }

          stubDbg (DEBUG_INFO_LEVEL, ">>> Trade sent, result = %d\n\n", result);


          // Now to refresh trade window rather than go to a confirmation page

          HTML_jump_to("backoffice", 1);

      } else {

          sprintf(buffer,
                        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
                        "<html><head></head><body>\n"
                        "<FONT FACE=\"arial\">"
                        "A very strange error was detected (in HTML_do_backoffice_change_cash) - some parameter was not set (?)<p>\n"
                        "prime broker is %d<br>\n"
                        "asset is %d<br>\n"
                        "accounting unit is %d<br>\n"
                        "amount is %d<br>\n"
                        "what to do is %d<br>\n",
                        prime_broker_index,
                        asset,
                        auId,
                        amount,
                        what_to_do
                );

          strcat(buffer, "<form action=\"backoffice\" method=\"get\"><input type=\"submit\" value=\"Back to backoffice page\"></form>\n");

          strcat(buffer, "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n");

          strcat(buffer, "</FONT>\n</body></html>\n");

      }


  }




  void HTML_do_backoffice_change_posis(void) {

      idtype auId = -1;
      idtype trading_interface_index = -1;
      idtype trading_interface_ID = -1;
      idtype security = -1;
      int32 amount = 0;
      int side = -1;
      uint32 price = 0;

      boolean auId_set = false;
      boolean trading_interface_set = false;
      boolean security_set = false;
      boolean amount_set = false;
      boolean side_set = false;
      boolean price_set = false;

      // Now parsing backoffice change parameters

      for(int i = 0; i < post_data_index; i++) {

          if(!strcmp("accounting_unit", post_data[i].key)) {
              auId_set = true;
              auId = atoi(post_data[i].value);
          }

          if(!strcmp("trading_interface", post_data[i].key)) {
              trading_interface_set = true;
              trading_interface_index = atoi(post_data[i].value);
              trading_interface_ID = memMap->tradingInterface[trading_interface_index];
          }

          if(!strcmp("security", post_data[i].key)) {
              security_set = true;
              security = atoi(post_data[i].value);
          }

          if(!strcmp("amount", post_data[i].key)) {
              amount_set = true;
              amount = atoll(post_data[i].value);
          }

          if(!strcmp("side", post_data[i].key)) {
              side_set = true;
              if(!strcmp("buy", post_data[i].value))
                  side = TRADE_SIDE_BUY;
              else
                  side = TRADE_SIDE_SELL;
          }

          if(!strcmp("price", post_data[i].key)) {
              price_set = true;
              price = atoi(post_data[i].value);
          }

      } // (post) args


      // Now to send the trade
      tradeCommand_t trade;

      trade.tiId = trading_interface_ID;
      trade.security = security;
      trade.quantity = amount;
      trade.finishedQuantity = amount;
      trade.side = side;
      trade.tradeType = TRADE_TYPE_MARKET;
      trade.timeInForce = TRADE_TIMEINFORCE_INMED_OR_CANCEL;
      trade.unconditional = true;
      trade.reservedData = 0;
      trade.finishedPrice = price;


      // Now to do the backoffice change

      if(
              auId_set &&
              trading_interface_set &&
              security_set &&
              amount_set &&
              side_set &&
              price_set
        ) {

          char priceStr[NLEN];
          renderPrice(priceStr, price, getSecurityDecimals(security));
          sprintf(buffer,
                  "   accounting unit is %d\n"
                  "   Trading Interface is %d (%s)\n"
                  "   security is %d (%s)\n"
                  "   amount is %d\n"
                  "   side is '%s'\n"
                  "   price is %s",
                  auId,
                  trading_interface_index,
                  memMap->tradingInterfaceName[trading_interface_index],
                  security,
                  securityName[security],
                  amount,
                  (side == TRADE_SIDE_BUY ? "BUY" : "SELL"),
                  priceStr
          );

          stubDbg (DEBUG_INFO_LEVEL, ">> Sending backoffice change to core:\n%s\n", buffer);

          char tradeId[SLEN];
          sprintf(tradeId, "Accounted_%d_%d", CALLBACK_systime.sec, CALLBACK_systime.usec);

          int32 result = contextTradeAccount(auId, &trade, tradeId);

          stubDbg (DEBUG_INFO_LEVEL, ">>> Trade added, result = %d\n\n", result);


          // Now to refresh trade window rather than go to a confirmation page

          HTML_jump_to("backoffice", 1);

      } else {

          sprintf(buffer,
                        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
                        "<html><head></head><body>\n"
                        "<FONT FACE=\"arial\">"
                        "A very strange error was detected (in HTML_do_backoffice_change_posis) - some parameter was not set (?)<p>\n"
                );

          strcat(buffer, "<form action=\"backoffice\" method=\"get\"><input type=\"submit\" value=\"Back to backoffice page\"></form>\n");

          strcat(buffer, "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n");

          strcat(buffer, "</FONT>\n</body></html>\n");

      }


  }



  void HTML_do_initialize_all_bo(void) {

      int32 amount_to_initialize_all = 0;
      boolean amount_to_initialize_all_set = false;


      // Now parsing backoffice change parameters

      for(int i = 0; i < post_data_index; i++) {

          if(!strcmp("amount_to_initialize_all", post_data[i].key)) {
              amount_to_initialize_all_set = true;
              amount_to_initialize_all = atoll(post_data[i].value);
          }

      } // (post) args


      // Now to do the backoffice change

      if(amount_to_initialize_all_set) {

          char msg[MLEN];
          sprintf(msg, ">>> Now initializing backoffice. All cash and swap posis will be erased, all pending orders will be resolved, and %d EUR will be added to all PBs in AU #3", amount_to_initialize_all);
          add_message(msg, STR_MSG_TRADING_INFO);
         // IMPORTANT!!! Assumes AU #3 is the cash transfers one!!

          uint32 tradesProcessed=0;
          for(uint32 t=0; (tradesProcessed<memMap->AccountingUnit[auIndex].numberOfAlives) && (t<MAX_TRADES_LIST_ALIVE); t++) {

              if(!memMap->AccountingUnit[auIndex].alive[t].alive) continue;

              tradesProcessed++;

              tradeCommand_t tc;
              tc.finishedQuantity=0;
              tc.finishedPrice=0;

              contextTradeResolve(memMap->AccountingUnit[auIndex].auId, &tc, memMap->AccountingUnit[auIndex].alive[t].ids.fixId);

          } // for trades alive


          for(int pbIndex=0; pbIndex<memMap->nPrimeBrokers; pbIndex++) {

              idtype pbID=memMap->primeBroker[pbIndex];

              for(int s=1; s<NUM_SECURITIES; s++) {

                  if(memMap->AccountingUnit[auIndex].SecurityExposureInt[pbIndex][s].amount != 0) {

                      int tiForThisPB=-1;
                      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {

                          if(memMap->whichPrimeBroker[ti]==pbID) {
                              tiForThisPB=ti; // we found it
                              break;
                          }

                      }

                      if((tiForThisPB<0) || (tiForThisPB>=memMap->nTradingInterfaces)) {

                          add_message("!!! ERROR initializing backoffice!! didn't find a valid TI for a PB!!!", STR_MSG_CRITICAL);

                      }

                      tradeCommand_t tc;
                      char tID[SLEN];
                      tc.tiId = memMap->tradingInterface[tiForThisPB];
                      tc.security = s;
                      tc.quantity = abs(memMap->AccountingUnit[auIndex].SecurityExposureInt[pbIndex][s].amount);
                      tc.side = memMap->AccountingUnit[auIndex].SecurityExposureInt[pbIndex][s].amount > 0 ? TRADE_SIDE_SELL : TRADE_SIDE_BUY;
                      tc.tradeType = TRADE_TYPE_MARKET; // irrelevant
//                        tc.limitPrice = 0; // irrelevant
//                        tc.maxShowQuantity = 0; // irrelevant
                      tc.timeInForce = TRADE_TIMEINFORCE_INMED_OR_CANCEL; // irrelevant
//                        tc.seconds = 0; // irrelevant
//                        tc.mseconds = 0; // irrelevant
//                        tc.expiration = XX; // irrelevant
                      tc.unconditional = true; // irrelevant
                      tc.checkWorstCase = false; // irrelevant
                      tc.reservedData = 0; // irrelevant

                      tc.finishedPrice = memMap->AccountingUnit[auIndex].SecurityExposureInt[pbIndex][s].price;
                      tc.finishedQuantity = tc.quantity;

                      stubDbg(DEBUG_INFO_LEVEL, ">>> (through backoffice change) Closing position %d %s on PB #%d (%s) through TI #%d (%s)\n",
                              memMap->AccountingUnit[auIndex].SecurityExposureInt[pbIndex][s].amount,
                              securityName[s],
                              pbIndex,
                              memMap->primeBrokerName[pbIndex],
                              tiForThisPB,
                              memMap->tradingInterfaceName[tiForThisPB]);
                      contextTradeAccount(memMap->AccountingUnit[auIndex].auId, &tc, tID);

                  } // if security exposure != 0

              } // for securities

              stubDbg(DEBUG_INFO_LEVEL, ">>> Sending contextCashClean on all assets for PB #%d (%s) in AUs #%d and #3\n",
                      pbIndex, memMap->primeBrokerName[pbIndex], memMap->AccountingUnit[auIndex].auId);
              for(int a=0; a<NUM_ASSETS; a++) {

                  contextCashClean(pbID, memMap->AccountingUnit[auIndex].auId, a);
                  contextCashClean(pbID, 3, a);

              } // for assets

              stubDbg(DEBUG_INFO_LEVEL, ">>> Adding %d EURs to PB #%d (%s) in AUs #3\n",
                      amount_to_initialize_all, pbIndex, memMap->primeBrokerName[pbIndex]);
              contextCashAdd(pbID, 3, EUR, amount_to_initialize_all);

          } // for PBs

          HTML_jump_to("backoffice", 1);

      } else {

          sprintf(buffer,
                        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
                        "<html><head></head><body>\n"
                        "<FONT FACE=\"arial\">"
                        "A very strange error was detected (in HTML_do_initialize_all_bo) - some parameter was not set (?)<p>\n"
                        "amount_to_initialize_all is %d<br>\n",
                        amount_to_initialize_all
                );

          strcat(buffer, "<form action=\"backoffice\" method=\"get\"><input type=\"submit\" value=\"Back to backoffice page\"></form>\n");

          strcat(buffer, "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n");

          strcat(buffer, "</FONT>\n</body></html>\n");

      }


  }


  void HTML_show_debug(void) {

      sprintf(buffer,
              "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
              "<html><head></head><body>\n"
              "<FONT FACE=\"arial\">"
              "<b>Memory map debug - exposures and equities</b><p>\n"
              "<table><TR>"
              "<form action=\"debug\" method=\"get\"><input type=\"submit\" value=\"Refresh\"></form>\n"
              "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n"
              "</TR></table><p>\n"
      );

      // We first show system prices (securities and assets)

      strcat(buffer,
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Security (system) prices:</b></TH>\n"
              "<TH> </TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Asset (system) prices:</b></TH></TR>\n"
          );

      strcat(buffer, "<TR><TD align=\"center\">\n");

      for(idtype s = 0; s < many_securities; s++) {

          char priceBid[SLEN];
          char priceAsk[SLEN];
          char systemPrice[SLEN];

          renderPrice(priceBid,
                      memMap->mapBookSpBidInt[which_securities[s]],
                      getSecurityDecimals(which_securities[s])
                     );
          renderPrice(priceAsk,
                      memMap->mapBookSpAskInt[which_securities[s]],
                      getSecurityDecimals(which_securities[s])
                     );
          renderPrice(systemPrice,
                      memMap->mapBookSpInt[which_securities[s]],
                      getSecurityDecimals(which_securities[s])
                     );
          sprintf(line, "%s: %s / %s [%s]<br>\n", securityName[which_securities[s]], priceBid, priceAsk, systemPrice);
          strcat(buffer, line);

      }

      strcat(buffer, "</TD><TD> </TD><TD align=\"center\">\n");

      for(idtype a = 0; a < many_assets; a++) {

          sprintf(line, "%s: %.5f", assetName[which_assets[a]], memMap->mapBookAp[which_assets[a]].price);
          strcat(buffer, line);
          if(memMap->mapBookAp[which_assets[a]].price != 0) {
              sprintf(line, " [reverse is %.5f]<br>\n", 1/memMap->mapBookAp[which_assets[a]].price);
              strcat(buffer, line);
          } else {
              strcat(buffer, "<br>\n");
          }

      }

      strcat(buffer, "</TD></TR></table><p>\n");



      // First the header row:

      strcat(buffer,
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
              "<TH><font color=#FFFFFF># PB</TH>\n"
              "<TH><font color=#FFFFFF>PB name</TH>\n"
              "<TH><font color=#FFFFFF>What</TH>\n"
              "<TH><font color=#FFFFFF>Level</TH>\n"
      );

      for(idtype a = 0; a < many_assets; a++) {
          sprintf(line, "<TH><font color=#FFFFFF>%s</TH>\n", assetName[which_assets[a]]);
          strcat(buffer, line);
      }

      for(idtype s = 0; s < many_securities; s++) {
          sprintf(line, "<TH><font color=#FFFFFF>%s</TH>\n", securityName[which_securities[s]]);
          strcat(buffer, line);
      }

      strcat(buffer,"</TR>\n");

      // We iterate PBs

      for(idtype i = 0; i < memMap->nPrimeBrokers; i++) {

          sprintf(line,
                  "<TR bgcolor=#FFFFA0>\n"
                  "<TD><b>#%d</b></TD>\n"
                  "<TD><b>%s</b></TD>\n",
                  i,
                  memMap->primeBrokerName[i]
                 );

          strcat(buffer, line);

          strcat(buffer,
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                );

          for(idtype a = 0; a < many_assets; a++) {
              strcat(buffer, "<TD></TD>\n");
          }

          for(idtype s = 0; s < many_securities; s++) {
              strcat(buffer, "<TD></TD>\n");
          }

          strcat(buffer, "</TR>\n");

// We start w M1 at all levels

          // First AU:

          strcat(buffer,
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b>M1</b> (cash)</TD>\n"
                  "<TD><b>AU</b></TD>\n"
                );

          for(idtype a = 0; a < many_assets; a++) {
              char exp[SLEN];
              renderWithDots(exp, memMap->AccountingUnit[auIndex].AssetExposure[i][which_assets[a]], false);
              sprintf(line, "<TD>%s</TD>\n", exp);
              strcat(buffer, line);
          }

          for(idtype s = 0; s < many_securities; s++) {
              strcat(buffer, "<TD></TD>\n");
          }

          strcat(buffer, "</TR>\n");

          // Second EP:

          strcat(buffer,
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b>EP</b></TD>\n"
                );

          for(idtype a = 0; a < many_assets; a++) {
              char exp[SLEN];
              renderWithDots(exp, memMap->EquityPool[epIndex].AssetExposureEP[i][which_assets[a]], false);
              sprintf(line, "<TD>%s</TD>\n", exp);
              strcat(buffer, line);
          }

          for(idtype s = 0; s < many_securities; s++) {
              strcat(buffer, "<TD></TD>\n");
          }

          strcat(buffer, "</TR>\n");

          // Then PB:

          strcat(buffer,
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b>PB</b></TD>\n"
                );

          for(idtype a = 0; a < many_assets; a++) {
              char exp[SLEN];
              renderWithDots(exp, memMap->AssetExposurePB[i][which_assets[a]], false);
              sprintf(line, "<TD>%s</TD>\n", exp);
              strcat(buffer, line);
          }

          for(idtype s = 0; s < many_securities; s++) {
              strcat(buffer, "<TD></TD>\n");
          }

          strcat(buffer, "</TR>\n");

// Done w M1 !!!



// Now we go for M2 at all levels

          // First AU:

          strcat(buffer,
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b>M2</b> (swaps)</TD>\n"
                  "<TD><b>AU</b></TD>\n"
                );

          for(idtype a = 0; a < many_assets; a++) {
              strcat(buffer, "<TD></TD>\n");
          }

          for(idtype s = 0; s < many_securities; s++) {
              char exp[SLEN];
              char price[SLEN];
              renderWithDots(exp, memMap->AccountingUnit[auIndex].SecurityExposure[i][which_securities[s]].exposure, false);
              renderPrice(price,
                          memMap->AccountingUnit[auIndex].SecurityExposureInt[i][which_securities[s]].price,
                          getSecurityDecimals(which_securities[s])
                         );
              if(memMap->AccountingUnit[auIndex].SecurityExposure[i][which_securities[s]].exposure != 0) {
                  sprintf(line, "<TD>%s @%s</TD>\n", exp, price);
              } else {
                  sprintf(line, "<TD>0</TD>\n");
              }
              strcat(buffer, line);
          }

          strcat(buffer, "</TR>\n");

          // Second EP:

          strcat(buffer,
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b>EP</b></TD>\n"
                );

          for(idtype a = 0; a < many_assets; a++) {
              strcat(buffer, "<TD></TD>\n");
          }

          for(idtype s = 0; s < many_securities; s++) {
              char exp[SLEN];
              char price[SLEN];
              renderWithDots(exp, memMap->EquityPool[epIndex].SecurityExposureEP[i][which_securities[s]].exposure, false);
              renderPrice(price,
                          memMap->EquityPool[epIndex].SecurityExposureEPInt[i][which_securities[s]].price,
                          getSecurityDecimals(which_securities[s])
                         );
              if(memMap->EquityPool[epIndex].SecurityExposureEP[i][which_securities[s]].exposure != 0) {
                  sprintf(line, "<TD>%s @%s</TD>\n", exp, price);
              } else {
                  sprintf(line, "<TD>0</TD>\n");
              }
              strcat(buffer, line);
          }

          strcat(buffer, "</TR>\n");

          // Then PB:

          strcat(buffer,
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b>PB</b></TD>\n"
                );

          for(idtype a = 0; a < many_assets; a++) {
              strcat(buffer, "<TD></TD>\n");
          }

          for(idtype s = 0; s < many_securities; s++) {
              char exp[SLEN];
              char price[SLEN];
              renderWithDots(exp, memMap->SecurityExposurePB[i][which_securities[s]].exposure, false);
              renderPrice(price,
                          memMap->SecurityExposurePBInt[i][which_securities[s]].price,
                          getSecurityDecimals(which_securities[s])
                         );
              if(memMap->SecurityExposurePB[i][which_securities[s]].exposure != 0) {
                  sprintf(line, "<TD>%s @%s</TD>\n", exp, price);
              } else {
                  sprintf(line, "<TD>0</TD>\n");
              }
              strcat(buffer, line);
          }

          strcat(buffer, "</TR>\n");

// Done w M2 !!!


// Then w M4 at all levels

          // First AU:

          strcat(buffer,
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b>M4</b> (cash-equiv risk)</TD>\n"
                  "<TD><b>AU</b></TD>\n"
                );

          for(idtype a = 0; a < many_assets; a++) {
              char exp[SLEN];
              renderWithDots(exp, memMap->AccountingUnit[auIndex].TotalExposure[i][which_assets[a]], false);
              sprintf(line, "<TD>%s</TD>\n", exp);
              strcat(buffer, line);
          }

          for(idtype s = 0; s < many_securities; s++) {
              strcat(buffer, "<TD></TD>\n");
          }

          strcat(buffer, "</TR>\n");

          // Second EP:

          strcat(buffer,
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b>EP</b></TD>\n"
                );

          for(idtype a = 0; a < many_assets; a++) {
              char exp[SLEN];
              renderWithDots(exp, memMap->EquityPool[epIndex].TotalExposureEP[i][which_assets[a]], false);
              sprintf(line, "<TD>%s</TD>\n", exp);
              strcat(buffer, line);
          }

          for(idtype s = 0; s < many_securities; s++) {
              strcat(buffer, "<TD></TD>\n");
          }

          strcat(buffer, "</TR>\n");

          // Then PB:

          strcat(buffer,
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b>PB</b></TD>\n"
                );

          for(idtype a = 0; a < many_assets; a++) {
              char exp[SLEN];
              renderWithDots(exp, memMap->TotalExposurePB[i][which_assets[a]], false);
              sprintf(line, "<TD>%s</TD>\n", exp);
              strcat(buffer, line);
          }

          for(idtype s = 0; s < many_securities; s++) {
              strcat(buffer, "<TD></TD>\n");
          }

          strcat(buffer, "</TR>\n");

// Done w M4 !!!


// Finally w M5 at all levels

          // First AU:

          strcat(buffer,
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b>M5</b> (total risk in Eur)</TD>\n"
                  "<TD><b>AU</b></TD>\n"
                );

          for(idtype a = 0; a < many_assets; a++) {
              char exp[SLEN];
              renderWithDots(exp, memMap->AccountingUnit[auIndex].TotalExposureWprice[i][which_assets[a]], false);
              sprintf(line, "<TD>%s</TD>\n", exp);
              strcat(buffer, line);
          }

          for(idtype s = 0; s < many_securities; s++) {
              strcat(buffer, "<TD></TD>\n");
          }

          strcat(buffer, "</TR>\n");

          // Second EP:

          strcat(buffer,
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b>EP</b></TD>\n"
                );

          for(idtype a = 0; a < many_assets; a++) {
              char exp[SLEN];
              renderWithDots(exp, memMap->EquityPool[epIndex].TotalExposureWpriceEP[i][which_assets[a]], false);
              sprintf(line, "<TD>%s</TD>\n", exp);
              strcat(buffer, line);
          }

          for(idtype s = 0; s < many_securities; s++) {
              strcat(buffer, "<TD></TD>\n");
          }

          strcat(buffer, "</TR>\n");

          // Then PB:

          strcat(buffer,
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b></b></TD>\n"
                  "<TD><b>PB</b></TD>\n"
                );

          for(idtype a = 0; a < many_assets; a++) {
              char exp[SLEN];
              renderWithDots(exp, memMap->TotalExposureWpricePB[i][which_assets[a]], false);
              sprintf(line, "<TD>%s</TD>\n", exp);
              strcat(buffer, line);
          }

          for(idtype s = 0; s < many_securities; s++) {
              strcat(buffer, "<TD></TD>\n");
          }

          strcat(buffer, "</TR>\n");

// Done w M5 !!!


      } // PB loop



// Now w totals (all PBs consolidated)

      strcat(buffer,
              "<TR bgcolor=#FFFFA0>\n"
              "<TD><b></b></TD>\n"
              "<TD><b>All PBs consolidated</b></TD>\n"
             );

      strcat(buffer,
              "<TD><b></b></TD>\n"
              "<TD><b></b></TD>\n"
            );

      for(idtype a = 0; a < many_assets; a++) {
          strcat(buffer, "<TD></TD>\n");
      }

      for(idtype s = 0; s < many_securities; s++) {
          strcat(buffer, "<TD></TD>\n");
      }

      strcat(buffer, "</TR>\n");

// We start w M1 at all levels

      // First AU:

      strcat(buffer,
              "<TD><b></b></TD>\n"
              "<TD><b></b></TD>\n"
              "<TD><b>M1</b> (cash)</TD>\n"
              "<TD><b>AU</b></TD>\n"
            );

      for(idtype a = 0; a < many_assets; a++) {
          char exp[SLEN];
          renderWithDots(exp, memMap->AccountingUnit[auIndex].GblAssetExposureAU[which_assets[a]], false);
          sprintf(line, "<TD>%s</TD>\n", exp);
          strcat(buffer, line);
      }

      for(idtype s = 0; s < many_securities; s++) {
          strcat(buffer, "<TD></TD>\n");
      }

      strcat(buffer, "</TR>\n");

      // Second EP:

      strcat(buffer,
              "<TD><b></b></TD>\n"
              "<TD><b></b></TD>\n"
              "<TD><b></b></TD>\n"
              "<TD><b>EP</b></TD>\n"
            );

      for(idtype a = 0; a < many_assets; a++) {
          char exp[SLEN];
          renderWithDots(exp, memMap->EquityPool[epIndex].GblAssetExposureEP[which_assets[a]], false);
          sprintf(line, "<TD>%s</TD>\n", exp);
          strcat(buffer, line);
      }

      for(idtype s = 0; s < many_securities; s++) {
          strcat(buffer, "<TD></TD>\n");
      }

      strcat(buffer, "</TR>\n");

      // Then PB:

      strcat(buffer,
              "<TD><b></b></TD>\n"
              "<TD><b></b></TD>\n"
              "<TD><b></b></TD>\n"
              "<TD><b>PB</b></TD>\n"
            );

      for(idtype a = 0; a < many_assets; a++) {
          char exp[SLEN];
          renderWithDots(exp, memMap->GblAssetExposurePB[which_assets[a]], false);
          sprintf(line, "<TD>%s</TD>\n", exp);
          strcat(buffer, line);
      }

      for(idtype s = 0; s < many_securities; s++) {
          strcat(buffer, "<TD></TD>\n");
      }

      strcat(buffer, "</TR>\n");

// Done w M1 !!!



// Now we go for M2 at all levels

      // First AU:

      strcat(buffer,
              "<TD><b></b></TD>\n"
              "<TD><b></b></TD>\n"
              "<TD><b>M2</b> (swaps)</TD>\n"
              "<TD><b>AU</b></TD>\n"
            );

      for(idtype a = 0; a < many_assets; a++) {
          strcat(buffer, "<TD></TD>\n");
      }

      for(idtype s = 0; s < many_securities; s++) {

          char exp[SLEN];
          char price[SLEN];
          renderWithDots(exp, memMap->AccountingUnit[auIndex].GblSecurityExposureAU[which_securities[s]].exposure, false);
          renderPrice(price,
                      memMap->AccountingUnit[auIndex].GblSecurityExposureAUInt[which_securities[s]].price,
                      getSecurityDecimals(which_securities[s])
                     );
          if(memMap->AccountingUnit[auIndex].GblSecurityExposureAU[which_securities[s]].exposure != 0) {
              sprintf(line, "<TD>%s @%s</TD>\n", exp, price);
          } else {
              sprintf(line, "<TD>0</TD>\n");
          }
          strcat(buffer, line);
      }

      strcat(buffer, "</TR>\n");

      // Second EP:

      strcat(buffer,
              "<TD><b></b></TD>\n"
              "<TD><b></b></TD>\n"
              "<TD><b></b></TD>\n"
              "<TD><b>EP</b></TD>\n"
            );

      for(idtype a = 0; a < many_assets; a++) {
          strcat(buffer, "<TD></TD>\n");
      }

      for(idtype s = 0; s < many_securities; s++) {
          char exp[SLEN];
          char price[SLEN];
          renderWithDots(exp, memMap->EquityPool[epIndex].GblSecurityExposureEP[which_securities[s]].exposure, false);
          renderPrice(price,
                      memMap->EquityPool[epIndex].GblSecurityExposureEPInt[which_securities[s]].price,
                      getSecurityDecimals(which_securities[s])
                     );
          if(memMap->EquityPool[epIndex].GblSecurityExposureEP[which_securities[s]].exposure != 0) {
              sprintf(line, "<TD>%s @%s</TD>\n", exp, price);
          } else {
              sprintf(line, "<TD>0</TD>\n");
          }
          strcat(buffer, line);
      }

      strcat(buffer, "</TR>\n");

      // Then PB:

      strcat(buffer,
              "<TD><b></b></TD>\n"
              "<TD><b></b></TD>\n"
              "<TD><b></b></TD>\n"
              "<TD><b>PB</b></TD>\n"
            );

      for(idtype a = 0; a < many_assets; a++) {
          strcat(buffer, "<TD></TD>\n");
      }

      for(idtype s = 0; s < many_securities; s++) {
          char exp[SLEN];
          char price[SLEN];
          renderWithDots(exp, memMap->GblSecurityExposurePB[which_securities[s]].exposure, false);
          renderPrice(price,
                      memMap->GblSecurityExposurePBInt[which_securities[s]].price,
                      getSecurityDecimals(which_securities[s])
                     );
          if(memMap->GblSecurityExposurePB[which_securities[s]].exposure != 0) {
              sprintf(line, "<TD>%s @%s</TD>\n", exp, price);
          } else {
              sprintf(line, "<TD>0</TD>\n");
          }
          strcat(buffer, line);
      }

      strcat(buffer, "</TR>\n");


// Done w M2 !!!

// Then w M4 at all levels

      // First AU:

      strcat(buffer,
              "<TD><b></b></TD>\n"
              "<TD><b></b></TD>\n"
              "<TD><b>M4</b> (cash-equiv risk)</TD>\n"
              "<TD><b>AU</b></TD>\n"
            );

      for(idtype a = 0; a < many_assets; a++) {
          char exp[SLEN];
          renderWithDots(exp, memMap->AccountingUnit[auIndex].GblTotalExposureAU[which_assets[a]], false);
          sprintf(line, "<TD>%s</TD>\n", exp);
          strcat(buffer, line);
      }

      for(idtype s = 0; s < many_securities; s++) {
          strcat(buffer, "<TD></TD>\n");
      }

      strcat(buffer, "</TR>\n");

      // Second EP:

      strcat(buffer,
              "<TD><b></b></TD>\n"
              "<TD><b></b></TD>\n"
              "<TD><b></b></TD>\n"
              "<TD><b>EP</b></TD>\n"
            );

      for(idtype a = 0; a < many_assets; a++) {
          char exp[SLEN];
          renderWithDots(exp, memMap->EquityPool[epIndex].GblTotalExposureEP[which_assets[a]], false);
          sprintf(line, "<TD>%s</TD>\n", exp);
          strcat(buffer, line);
      }

      for(idtype s = 0; s < many_securities; s++) {
          strcat(buffer, "<TD></TD>\n");
      }

      strcat(buffer, "</TR>\n");

      // Then PB:

      strcat(buffer,
              "<TD><b></b></TD>\n"
              "<TD><b></b></TD>\n"
              "<TD><b></b></TD>\n"
              "<TD><b>PB</b></TD>\n"
            );

      for(idtype a = 0; a < many_assets; a++) {
          char exp[SLEN];
          renderWithDots(exp, memMap->GblTotalExposurePB[which_assets[a]], false);
          sprintf(line, "<TD>%s</TD>\n", exp);
          strcat(buffer, line);
      }

      for(idtype s = 0; s < many_securities; s++) {
          strcat(buffer, "<TD></TD>\n");
      }

      strcat(buffer, "</TR>\n");


// Done w M4 !!!


// Done w totals (all PBs consolidated)

      strcat(buffer, "</table>\n");

      sprintf(line,
              "<p>\n"
              "Total AU Equity is %.2f<br>\n"
              "Total EP Equity is %.2f<br>\n"
              "Total System Equity is %.2f<br>\n",
              memMap->AccountingUnit[auIndex].GlobalEquity_STR,
              memMap->EquityPool[epIndex].GlobalEquity_EP,
              memMap->GlobalEquity_PB
             );

      strcat(buffer, line);


      strcat(buffer, "</FONT>\n</body></html>\n");

  }





  void HTML_show_indicators(void) {

      sprintf(buffer,
              "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
              "<html><head></head><body>\n"
              "<FONT FACE=\"arial\">"
              "<b>Memory map debug - exposures and equities</b><p>\n"
              "<table><TR>"
              "<form action=\"indicators\" method=\"get\"><input type=\"submit\" value=\"Refresh\"></form>\n"
              "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n"
              "</TR></table><p>\n"
      );

      uint manyIndicators;
      static char indicatorNames[MAX_INDICATORS][NUM_TIMEFRAMES][SLEN];
      static char indicatorValues[MAX_INDICATORS][NUM_TIMEFRAMES][SLEN];


      for(int pbIndex=0; pbIndex<memMap->nPrimeBrokers; pbIndex++) {

          sprintf(line, "PB #%d (%s)<p>\n", pbIndex, memMap->primeBrokerName[pbIndex]);
          strcat(buffer, line);
          strcat(buffer, "<table border=0>\n"
                         "<TR bgcolor=#151540>\n"
                         "<TH colspan=\"2\" align=\"center\"><font color=#FFFFFF><b>Today</b></TH>\n"
                         "<TH colspan=\"2\" align=\"center\"><font color=#FFFFFF><b>This week</b></TH>\n"
                         "<TH colspan=\"2\" align=\"center\"><font color=#FFFFFF><b>This month</b></TH>\n"
                         "<TH colspan=\"2\" align=\"center\"><font color=#FFFFFF><b>Since inception</b></TH>\n"
                         "</TR>\n"
              );

          renderIndicators(memMap, false, pbIndex, &manyIndicators, indicatorNames, indicatorValues);

          for(int ind=0; ind<manyIndicators; ind++) {

              sprintf(line,
                      "<TR>\n"
                      "<TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD>\n"
                      "</TR>\n",
                      indicatorNames[ind][TIMEFRAME_TODAY], indicatorValues[ind][TIMEFRAME_TODAY],
                      indicatorNames[ind][TIMEFRAME_THIS_WEEK], indicatorValues[ind][TIMEFRAME_THIS_WEEK],
                      indicatorNames[ind][TIMEFRAME_THIS_MONTH], indicatorValues[ind][TIMEFRAME_THIS_MONTH],
                      indicatorNames[ind][TIMEFRAME_SINCE_INCEPTION], indicatorValues[ind][TIMEFRAME_SINCE_INCEPTION]
                     );
              strcat(buffer, line);

          }

          strcat(buffer, "</table><br>\n");

      } // loop pbIndex


      sprintf(line, "All PBs<p>\n");
      strcat(buffer, line);
      strcat(buffer, "<table border=0>\n"
                     "<TR bgcolor=#151540>\n"
                     "<TH colspan=\"2\" align=\"center\"><font color=#FFFFFF><b>Today</b></TH>\n"
                     "<TH colspan=\"2\" align=\"center\"><font color=#FFFFFF><b>This week</b></TH>\n"
                     "<TH colspan=\"2\" align=\"center\"><font color=#FFFFFF><b>This month</b></TH>\n"
                     "<TH colspan=\"2\" align=\"center\"><font color=#FFFFFF><b>Since inception</b></TH>\n"
                     "</TR>\n"
          );

      renderIndicators(memMap, true, 0, &manyIndicators, indicatorNames, indicatorValues);

      for(int ind=0; ind<manyIndicators; ind++) {

          sprintf(line,
                  "<TR>\n"
                  "<TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD><TD>%s</TD>\n"
                  "</TR>\n",
                  indicatorNames[ind][TIMEFRAME_TODAY], indicatorValues[ind][TIMEFRAME_TODAY],
                  indicatorNames[ind][TIMEFRAME_THIS_WEEK], indicatorValues[ind][TIMEFRAME_THIS_WEEK],
                  indicatorNames[ind][TIMEFRAME_THIS_MONTH], indicatorValues[ind][TIMEFRAME_THIS_MONTH],
                  indicatorNames[ind][TIMEFRAME_SINCE_INCEPTION], indicatorValues[ind][TIMEFRAME_SINCE_INCEPTION]
                 );
          strcat(buffer, line);

      }

      strcat(buffer, "</table><br>\n");


      strcat(buffer, "</FONT>\n</body></html>\n");

  }



  void HTML_show_latency_histograms(void) {

      buildFixToUsHistograms(memMap);
      buildCoreHistograms(memMap);
      buildExecTimeHistograms(memMap);
      buildProfilingHistograms(memMap);

      sprintf(buffer,
              "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
              "<html><head></head><body>\n"
              "<FONT FACE=\"arial\">"
              "<b>Latency histograms</b><p>\n"
              "<table><TR>"
              "<form action=\"latency_histograms\" method=\"get\"><input type=\"submit\" value=\"Refresh\"></form>\n"
              "<form action=\"print_samples\" method=\"get\"><input type=\"submit\" value=\"Print samples\"></form>\n"
              "<form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form>\n"
              "</TR></table><p>\n"
      );

      // First, fix to core histograms

      strcat(buffer, "<b>1. Fix to Core:</b><br>\n");

      strcat(buffer,
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
              "<TH></TH>\n"
          );

      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          sprintf(line, "<TH colspan=\"2\"align=\"center\"><font color=#FFFFFF><b>TI #%d (%s)</b></TH>\n", ti, memMap->tradingInterfaceName[ti]);
          strcat(buffer, line);
      }
      strcat(buffer, "</TR>\n");

      strcat(buffer, "<TR bgcolor=#151540><TH align=\"center\"><font color=#FFFFFF><b>Bucket</b></TH>\n");
      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          strcat(buffer, "<TH align=\"center\"><font color=#FFFFFF><b>Samples</b></TH>\n"
                         "<TH align=\"center\"><font color=#FFFFFF><b>Acc %</b></TH>\n"
                );
      }
      strcat(buffer, "</TR>\n");

      for(int b=0; b<=FixToCore[0].nBuckets; b++) { // Important! all TIs with the same nBuckets!!

          sprintf(line, "<TR><TD><b>%s</b></TD>\n", FixToCore[0].bucket[b].bucketName);
          strcat(buffer, line);

          for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
              sprintf(line, "<TD>%d</TD><TD>%.1f</TD>\n", FixToCore[ti].bucket[b].samples, 100.0*FixToCore[ti].bucket[b].accPct);
              strcat(buffer, line);
          }

          strcat(buffer, "</TR>\n");

      } // for (buckets)

      strcat(buffer,
              "<TR bgcolor=#151540>\n"
              "<TH></TH>\n"
          );

      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          sprintf(line, "<TH><font color=#FFFFFF><b>%d</b></TH><TH><font color=#FFFFFF>100.0</TH>\n",
                  FixToCore[ti].bucket[FixToCore[ti].nBuckets].accSamples);
          strcat(buffer, line);
      }
      strcat(buffer, "</TR>\n");

      strcat(buffer, "</table><p>\n");




      // Second, core to stub histograms

      strcat(buffer, "<b>2. Core to Stub (assuming core and stub in the same server!!):</b><br>\n");

      strcat(buffer,
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Bucket</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Samples</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>%</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Acc samples</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Acc %</b></TH>\n"
              "</TR>\n"
          );

      for(int b=0; b<=CoreToStub.nBuckets; b++) {

          sprintf(line, "<TR><TD><b>%s</b></TD>"
                        "<TD>%d</TD>"
                        "<TD>%.1f</TD>"
                        "<TD>%d</TD>"
                        "<TD>%.1f</TD>"
                        "</TR>",
                        CoreToStub.bucket[b].bucketName,
                        CoreToStub.bucket[b].samples,
                        100.0*CoreToStub.bucket[b].pct,
                        CoreToStub.bucket[b].accSamples,
                        100.0*CoreToStub.bucket[b].accPct
                 );
          strcat(buffer, line);

      } // for (buckets)

      strcat(buffer, "</table><p>\n");




      // Third, fix to stub histograms

      strcat(buffer, "<b>3. Fix to Stub:</b><br>\n");

      strcat(buffer,
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
              "<TH></TH>\n"
          );

      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          sprintf(line, "<TH colspan=\"2\"align=\"center\"><font color=#FFFFFF><b>TI #%d (%s)</b></TH>\n", ti, memMap->tradingInterfaceName[ti]);
          strcat(buffer, line);
      }
      strcat(buffer, "</TR>\n");

      strcat(buffer, "<TR bgcolor=#151540><TH align=\"center\"><font color=#FFFFFF><b>Bucket</b></TH>\n");
      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          strcat(buffer, "<TH align=\"center\"><font color=#FFFFFF><b>Samples</b></TH>\n"
                         "<TH align=\"center\"><font color=#FFFFFF><b>Acc %</b></TH>\n"
                );
      }
      strcat(buffer, "</TR>\n");

      for(int b=0; b<=FixToStub[0].nBuckets; b++) { // Important! all TIs with the same nBuckets!!

          sprintf(line, "<TR><TD><b>%s</b></TD>\n", FixToStub[0].bucket[b].bucketName);
          strcat(buffer, line);

          for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
              sprintf(line, "<TD>%d</TD><TD>%.1f</TD>\n", FixToStub[ti].bucket[b].samples, 100.0*FixToStub[ti].bucket[b].accPct);
              strcat(buffer, line);
          }

          strcat(buffer, "</TR>\n");

      } // for (buckets)

      strcat(buffer,
              "<TR bgcolor=#151540>\n"
              "<TH></TH>\n"
          );

      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          sprintf(line, "<TH><font color=#FFFFFF><b>%d</b></TH><TH><font color=#FFFFFF>100.0</TH>\n",
                  FixToStub[ti].bucket[FixToStub[ti].nBuckets].accSamples);
          strcat(buffer, line);
      }
      strcat(buffer, "</TR>\n");

      strcat(buffer, "</table><p>\n");



      // Fourth, tick-to-order histograms

      strcat(buffer, "<b>4. Tick-to-order:</b><br>\n");

      strcat(buffer,
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Bucket</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Samples</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>%</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Acc samples</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Acc %</b></TH>\n"
              "</TR>\n"
          );

      for(int b=0; b<=TickToOrder.nBuckets; b++) {

          sprintf(line, "<TR><TD><b>%s</b></TD>"
                        "<TD>%d</TD>"
                        "<TD>%.1f</TD>"
                        "<TD>%d</TD>"
                        "<TD>%.1f</TD>"
                        "</TR>",
                        TickToOrder.bucket[b].bucketName,
                        TickToOrder.bucket[b].samples,
                        100.0*TickToOrder.bucket[b].pct,
                        TickToOrder.bucket[b].accSamples,
                        100.0*TickToOrder.bucket[b].accPct
                 );
          strcat(buffer, line);

      } // for (buckets)

      strcat(buffer, "</table><p>\n");



      // Fifth, callback histograms

      strcat(buffer, "<b>5. Callback total delays:</b><br>\n");

      strcat(buffer,
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Bucket</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Samples</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>%</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Acc samples</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Acc %</b></TH>\n"
              "</TR>\n"
          );

      for(int b=0; b<=Callback.nBuckets; b++) {

          sprintf(line, "<TR><TD><b>%s</b></TD>"
                        "<TD>%d</TD>"
                        "<TD>%.1f</TD>"
                        "<TD>%d</TD>"
                        "<TD>%.1f</TD>"
                        "</TR>",
                        Callback.bucket[b].bucketName,
                        Callback.bucket[b].samples,
                        100.0*Callback.bucket[b].pct,
                        Callback.bucket[b].accSamples,
                        100.0*Callback.bucket[b].accPct
                 );
          strcat(buffer, line);

      } // for (buckets)

      strcat(buffer, "</table><p>\n");



      // Sixth, immediate limit execution histograms

      strcat(buffer, "<b>6. Immediate limit execution histograms:</b><br>\n");

      strcat(buffer,
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
              "<TH></TH>\n"
          );

      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          sprintf(line, "<TH colspan=\"2\"align=\"center\"><font color=#FFFFFF><b>TI #%d (%s)</b></TH>\n", ti, memMap->tradingInterfaceName[ti]);
          strcat(buffer, line);
      }
      strcat(buffer, "</TR>\n");

      strcat(buffer, "<TR bgcolor=#151540><TH align=\"center\"><font color=#FFFFFF><b>Bucket</b></TH>\n");
      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          strcat(buffer, "<TH align=\"center\"><font color=#FFFFFF><b>Samples</b></TH>\n"
                         "<TH align=\"center\"><font color=#FFFFFF><b>Acc %</b></TH>\n"
                );
      }
      strcat(buffer, "</TR>\n");

      for(int b=0; b<=ExecTimesImmediateLimit[0].nBuckets; b++) { // Important! all TIs with the same nBuckets!!

          sprintf(line, "<TR><TD><b>%s</b></TD>\n", ExecTimesImmediateLimit[0].bucket[b].bucketName);
          strcat(buffer, line);

          for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
              sprintf(line, "<TD>%d</TD><TD>%.1f</TD>\n", ExecTimesImmediateLimit[ti].bucket[b].samples, 100.0*ExecTimesImmediateLimit[ti].bucket[b].accPct);
              strcat(buffer, line);
          }

          strcat(buffer, "</TR>\n");

      } // for (buckets)

      strcat(buffer,
              "<TR bgcolor=#151540>\n"
              "<TH></TH>\n"
          );

      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          sprintf(line, "<TH><font color=#FFFFFF><b>%d</b></TH><TH><font color=#FFFFFF>100.0</TH>\n",
                  ExecTimesImmediateLimit[ti].bucket[ExecTimesImmediateLimit[ti].nBuckets].accSamples);
          strcat(buffer, line);
      }
      strcat(buffer, "</TR>\n");

      strcat(buffer, "</table><p>\n");


      // Seventh, market execution histograms

      strcat(buffer, "<b>7. Market execution histograms:</b><br>\n");

      strcat(buffer,
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
              "<TH></TH>\n"
          );

      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          sprintf(line, "<TH colspan=\"2\"align=\"center\"><font color=#FFFFFF><b>TI #%d (%s)</b></TH>\n", ti, memMap->tradingInterfaceName[ti]);
          strcat(buffer, line);
      }
      strcat(buffer, "</TR>\n");

      strcat(buffer, "<TR bgcolor=#151540><TH align=\"center\"><font color=#FFFFFF><b>Bucket</b></TH>\n");
      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          strcat(buffer, "<TH align=\"center\"><font color=#FFFFFF><b>Samples</b></TH>\n"
                         "<TH align=\"center\"><font color=#FFFFFF><b>Acc %</b></TH>\n"
                );
      }
      strcat(buffer, "</TR>\n");

      for(int b=0; b<=ExecTimesMarket[0].nBuckets; b++) { // Important! all TIs with the same nBuckets!!

          sprintf(line, "<TR><TD><b>%s</b></TD>\n", ExecTimesMarket[0].bucket[b].bucketName);
          strcat(buffer, line);

          for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
              sprintf(line, "<TD>%d</TD><TD>%.1f</TD>\n", ExecTimesMarket[ti].bucket[b].samples, 100.0*ExecTimesMarket[ti].bucket[b].accPct);
              strcat(buffer, line);
          }

          strcat(buffer, "</TR>\n");

      } // for (buckets)

      strcat(buffer,
              "<TR bgcolor=#151540>\n"
              "<TH></TH>\n"
          );

      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          sprintf(line, "<TH><font color=#FFFFFF><b>%d</b></TH><TH><font color=#FFFFFF>100.0</TH>\n",
                  ExecTimesMarket[ti].bucket[ExecTimesMarket[ti].nBuckets].accSamples);
          strcat(buffer, line);
      }
      strcat(buffer, "</TR>\n");

      strcat(buffer, "</table><p>\n");


      // Eighth, cancel execution histograms

      strcat(buffer, "<b>8. Cancel execution histograms:</b><br>\n");

      strcat(buffer,
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
              "<TH></TH>\n"
          );

      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          sprintf(line, "<TH colspan=\"2\"align=\"center\"><font color=#FFFFFF><b>TI #%d (%s)</b></TH>\n", ti, memMap->tradingInterfaceName[ti]);
          strcat(buffer, line);
      }
      strcat(buffer, "</TR>\n");

      strcat(buffer, "<TR bgcolor=#151540><TH align=\"center\"><font color=#FFFFFF><b>Bucket</b></TH>\n");
      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          strcat(buffer, "<TH align=\"center\"><font color=#FFFFFF><b>Samples</b></TH>\n"
                         "<TH align=\"center\"><font color=#FFFFFF><b>Acc %</b></TH>\n"
                );
      }
      strcat(buffer, "</TR>\n");

      for(int b=0; b<=ExecTimesCancel[0].nBuckets; b++) { // Important! all TIs with the same nBuckets!!

          sprintf(line, "<TR><TD><b>%s</b></TD>\n", ExecTimesCancel[0].bucket[b].bucketName);
          strcat(buffer, line);

          for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
              sprintf(line, "<TD>%d</TD><TD>%.1f</TD>\n", ExecTimesCancel[ti].bucket[b].samples, 100.0*ExecTimesCancel[ti].bucket[b].accPct);
              strcat(buffer, line);
          }

          strcat(buffer, "</TR>\n");

      } // for (buckets)

      strcat(buffer,
              "<TR bgcolor=#151540>\n"
              "<TH></TH>\n"
          );

      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          sprintf(line, "<TH><font color=#FFFFFF><b>%d</b></TH><TH><font color=#FFFFFF>100.0</TH>\n",
                  ExecTimesCancel[ti].bucket[ExecTimesCancel[ti].nBuckets].accSamples);
          strcat(buffer, line);
      }
      strcat(buffer, "</TR>\n");

      strcat(buffer, "</table><p>\n");


      // Ninth, replace execution histograms

      strcat(buffer, "<b>8. Replace execution histograms:</b><br>\n");

      strcat(buffer,
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
              "<TH></TH>\n"
          );

      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          sprintf(line, "<TH colspan=\"2\"align=\"center\"><font color=#FFFFFF><b>TI #%d (%s)</b></TH>\n", ti, memMap->tradingInterfaceName[ti]);
          strcat(buffer, line);
      }
      strcat(buffer, "</TR>\n");

      strcat(buffer, "<TR bgcolor=#151540><TH align=\"center\"><font color=#FFFFFF><b>Bucket</b></TH>\n");
      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          strcat(buffer, "<TH align=\"center\"><font color=#FFFFFF><b>Samples</b></TH>\n"
                         "<TH align=\"center\"><font color=#FFFFFF><b>Acc %</b></TH>\n"
                );
      }
      strcat(buffer, "</TR>\n");

      for(int b=0; b<=ExecTimesReplace[0].nBuckets; b++) { // Important! all TIs with the same nBuckets!!

          sprintf(line, "<TR><TD><b>%s</b></TD>\n", ExecTimesReplace[0].bucket[b].bucketName);
          strcat(buffer, line);

          for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
              sprintf(line, "<TD>%d</TD><TD>%.1f</TD>\n", ExecTimesReplace[ti].bucket[b].samples, 100.0*ExecTimesReplace[ti].bucket[b].accPct);
              strcat(buffer, line);
          }

          strcat(buffer, "</TR>\n");

      } // for (buckets)

      strcat(buffer,
              "<TR bgcolor=#151540>\n"
              "<TH></TH>\n"
          );

      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          sprintf(line, "<TH><font color=#FFFFFF><b>%d</b></TH><TH><font color=#FFFFFF>100.0</TH>\n",
                  ExecTimesReplace[ti].bucket[ExecTimesReplace[ti].nBuckets].accSamples);
          strcat(buffer, line);
      }
      strcat(buffer, "</TR>\n");

      strcat(buffer, "</table><p>\n");


      // Tenth, tick-to-order breakdown histograms

      strcat(buffer, "<b>10. Tick-to-order breakdown:</b><br>\n");

      strcat(buffer,
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
              "<TH></TH>\n"
          );

      for(int h=0; h<=TEST_TICKTOTRADE_TIME_MESAURES; h++) {
          sprintf(line, "<TH colspan=\"2\"align=\"center\"><font color=#FFFFFF><b>Diff #%d</b></TH>\n", h);
          strcat(buffer, line);
      }
      strcat(buffer, "</TR>\n");

      strcat(buffer, "<TR bgcolor=#151540><TH align=\"center\"><font color=#FFFFFF><b>Bucket</b></TH>\n");
      for(int h=0; h<=TEST_TICKTOTRADE_TIME_MESAURES; h++) {
          strcat(buffer, "<TH align=\"center\"><font color=#FFFFFF><b>Samples</b></TH>\n"
                         "<TH align=\"center\"><font color=#FFFFFF><b>Acc %</b></TH>\n"
                );
      }
      strcat(buffer, "</TR>\n");

      for(int b=0; b<=profilingHistograms[0].nBuckets; b++) { // Important! all profiling histograms in the same nBuckets!!

          sprintf(line, "<TR><TD><b>%s</b></TD>\n", profilingHistograms[0].bucket[b].bucketName);
          strcat(buffer, line);

          for(int h=0; h<=TEST_TICKTOTRADE_TIME_MESAURES; h++) {
              sprintf(line, "<TD>%d</TD><TD>%.1f</TD>\n", profilingHistograms[h].bucket[b].samples, 100.0*profilingHistograms[h].bucket[b].accPct);
              strcat(buffer, line);
          }

          strcat(buffer, "</TR>\n");

      } // for (buckets)

      strcat(buffer,
              "<TR bgcolor=#151540>\n"
              "<TH></TH>\n"
          );

      for(int h=0; h<=TEST_TICKTOTRADE_TIME_MESAURES; h++) {
          sprintf(line, "<TH><font color=#FFFFFF><b>%d</b></TH><TH><font color=#FFFFFF>100.0</TH>\n",
                  profilingHistograms[h].bucket[profilingHistograms[h].nBuckets].accSamples);
          strcat(buffer, line);
      }
      strcat(buffer, "</TR>\n");

      strcat(buffer, "</table><p>\n");



// Done w histograms


      strcat(buffer, "</FONT>\n</body></html>\n");

  }



  void HTML_do_print_samples(void) {

      stubDbg (DEBUG_INFO_LEVEL, "Printing samples in histogram arrays\n");

      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) save_samples(memMap);

      HTML_jump_to("latency_histograms", 1);

  }



  void HMTL_show_full_book_for_a_security(sortingFB_t OthersQuotesBid[MAX_QUOTES],
                                          int NumberOthersQuotesBid,
                                          sortingFB_t OwnQuotesBid[MAX_QUOTES],
                                          int NumberOwnQuotesBid,
                                          sortingFB_t OthersQuotesAsk[MAX_QUOTES],
                                          int NumberOthersQuotesAsk,
                                          sortingFB_t OwnQuotesAsk[MAX_QUOTES],
                                          int NumberOwnQuotesAsk,
                                          idtype sec) {



      if(NumberOthersQuotesBid+NumberOwnQuotesBid+NumberOthersQuotesAsk+NumberOwnQuotesAsk<=0) {
          strcat(buffer, "(No BID or ASK quotes at all for ");
          strcat(buffer, securityName[sec]);
          strcat(buffer, ")<p>\n");
          return;
      }


      strcat(buffer, "<b>Full book for ");
      strcat(buffer, securityName[sec]);
      strcat(buffer, "</b><p>\n");

      strcat(buffer,
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
            );

      for(int ti=memMap->nTradingInterfaces-1; ti>=0; ti--) {
          strcat(buffer, "<TH><font color=#FFFFFF>");
          strcat(buffer, memMap->tradingInterfaceName[ti]);
          strcat(buffer, "</TH>\n");
      }

      strcat(buffer, "<TH><font color=#FFFFFF>Own BUYs</TH><TH></TH><TH><font color=#FFFFFF>Own SELLs</TH>\n"); // This is for own quotes and prices

      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
          strcat(buffer, "<TH><font color=#FFFFFF>");
          strcat(buffer, memMap->tradingInterfaceName[ti]);
          strcat(buffer, "</TH>\n");
      }

      strcat(buffer, "</TR>\n"); // This ends up the header



      idtype currentOthersAskIndex = NumberOthersQuotesAsk-1;
      idtype currentOwnAskIndex = NumberOwnQuotesAsk-1;
      idtype currentOthersBidIndex = NumberOthersQuotesBid>0 ? 0 : -1;
      idtype currentOwnBidIndex = NumberOwnQuotesBid>0 ? 0 : -1;

      uint32 lastPrice=0;
      while(true) { // One iteration per row in the HTML table
          sortingFB_t *thisElementOthersBid, *thisElementOwnBid, *thisElementOthersAsk, *thisElementOwnAsk;
          mtbookElement_t *thisQuoteOthersBid, *thisQuoteOwnBid, *thisQuoteOthersAsk, *thisQuoteOwnAsk;

          if((currentOthersBidIndex>=0) && (currentOthersBidIndex<NumberOthersQuotesBid)) {
              thisElementOthersBid = &(OthersQuotesBid[currentOthersBidIndex]);
              thisQuoteOthersBid=&(memMap->mapFBQuotesBid[thisElementOthersBid->tiIndex][sec][thisElementOthersBid->quoteIndex]);
          } else {
              thisElementOthersBid = NULL;
              thisQuoteOthersBid = NULL;
          }

          if((currentOwnBidIndex>=0) && (currentOwnBidIndex<NumberOwnQuotesBid)) {
              thisElementOwnBid = &(OwnQuotesBid[currentOwnBidIndex]);
              thisQuoteOwnBid=&(memMap->mapFBQuotesBid[thisElementOwnBid->tiIndex][sec][thisElementOwnBid->quoteIndex]);
          } else {
              thisElementOwnBid = NULL;
              thisQuoteOwnBid = NULL;
          }

          if((currentOthersAskIndex>=0) && (currentOthersAskIndex<NumberOthersQuotesAsk)) {
              thisElementOthersAsk = &(OthersQuotesAsk[currentOthersAskIndex]);
              thisQuoteOthersAsk=&(memMap->mapFBQuotesAsk[thisElementOthersAsk->tiIndex][sec][thisElementOthersAsk->quoteIndex]);
          } else {
              thisElementOthersAsk = NULL;
              thisQuoteOthersAsk = NULL;
          }

          if((currentOwnAskIndex>=0) && (currentOwnAskIndex<NumberOwnQuotesAsk)) {
              thisElementOwnAsk = &(OwnQuotesAsk[currentOwnAskIndex]);
              thisQuoteOwnAsk=&(memMap->mapFBQuotesAsk[thisElementOwnAsk->tiIndex][sec][thisElementOwnAsk->quoteIndex]);
          } else {
              thisElementOwnAsk = NULL;
              thisQuoteOwnAsk = NULL;
          }

          uint32 currentPrice=0;

          if(thisElementOthersBid)
                  currentPrice = thisElementOthersBid->price > currentPrice ?
                          thisElementOthersBid->price : currentPrice;
          if(thisElementOwnBid)
                  currentPrice = thisElementOwnBid->price > currentPrice ?
                          thisElementOwnBid->price : currentPrice;
          if(thisElementOthersAsk)
                  currentPrice = thisElementOthersAsk->price > currentPrice ?
                          thisElementOthersAsk->price : currentPrice;
          if(thisElementOwnAsk)
                  currentPrice = thisElementOwnAsk->price > currentPrice ?
                          thisElementOwnAsk->price : currentPrice;


          if(currentPrice==0) break; // This is the end case

          if(lastPrice>currentPrice+1) {
              strcat(buffer, "<TR>");
              for(int ti=memMap->nTradingInterfaces-1; ti>=0; ti--) {
                  strcat(buffer, "<TD></TD>");
              }
              strcat(buffer, "<TD></TD><TD>...</TD><TD></TD>");
              for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
                  strcat(buffer, "<TD></TD>");
              }
              strcat(buffer, "</TR>\n");
          }


          uint32 aggBid[MAX_NUMBER_TI], aggAsk[MAX_NUMBER_TI];
          uint32 nQuotesBid[MAX_NUMBER_TI], nQuotesAsk[MAX_NUMBER_TI];
          for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {
              aggBid[ti]=0;
              aggAsk[ti]=0;
              nQuotesBid[ti]=0;
              nQuotesAsk[ti]=0;
          }
          uint32 aggOwnBid=0;
          uint32 aggOwnAsk=0;
          uint32 nQuotesOwnBid=0;
          uint32 nQuotesOwnAsk=0;

          if(thisElementOthersBid) {
              while(currentPrice==thisElementOthersBid->price) {
                  aggBid[thisElementOthersBid->tiIndex]+=thisQuoteOthersBid->liquidity;
                  nQuotesBid[thisElementOthersBid->tiIndex]++;
                  currentOthersBidIndex++;
                  if((currentOthersBidIndex>=0) && (currentOthersBidIndex<NumberOthersQuotesBid)) {
                      thisElementOthersBid = &(OthersQuotesBid[currentOthersBidIndex]);
                      thisQuoteOthersBid=&(memMap->mapFBQuotesBid[thisElementOthersBid->tiIndex][sec][thisElementOthersBid->quoteIndex]);
                  } else {
                      thisElementOthersBid = NULL;
                      thisQuoteOthersBid = NULL;
                      break; // to get out of the while loop
                  }
              } // while (thisElementOthersBid has currentPrice)
          } // if (thisElementOthersBid

          if(thisElementOwnBid) {
              while(currentPrice==thisElementOwnBid->price) {
                  aggOwnBid+=thisQuoteOwnBid->liquidity;
                  nQuotesOwnBid++;
                  currentOwnBidIndex++;
                  if((currentOwnBidIndex>=0) && (currentOwnBidIndex<NumberOwnQuotesBid)) {
                      thisElementOwnBid = &(OwnQuotesBid[currentOwnBidIndex]);
                      thisQuoteOwnBid=&(memMap->mapFBQuotesBid[thisElementOwnBid->tiIndex][sec][thisElementOwnBid->quoteIndex]);
                  } else {
                      thisElementOwnBid = NULL;
                      thisQuoteOwnBid = NULL;
                      break; // to get out of the while loop
                  }
              } // while (thisElementOwnBid has currentPrice)
          } // if (thisElementOwnBid

          if(thisElementOthersAsk) {
              while(currentPrice==thisElementOthersAsk->price) {
                  aggAsk[thisElementOthersAsk->tiIndex]+=thisQuoteOthersAsk->liquidity;
                  nQuotesAsk[thisElementOthersAsk->tiIndex]++;
                  currentOthersAskIndex--;
                  if((currentOthersAskIndex>=0) && (currentOthersAskIndex<NumberOthersQuotesAsk)) {
                      thisElementOthersAsk = &(OthersQuotesAsk[currentOthersAskIndex]);
                      thisQuoteOthersAsk=&(memMap->mapFBQuotesAsk[thisElementOthersAsk->tiIndex][sec][thisElementOthersAsk->quoteIndex]);
                  } else {
                      thisElementOthersAsk = NULL;
                      thisQuoteOthersAsk = NULL;
                      break; // to get out of the while loop
                  }
              } // while (thisElementOthersAsk has currentPrice)
          } // if (thisElementOthersAsk

          if(thisElementOwnAsk) {
              while(currentPrice==thisElementOwnAsk->price) {
                  aggOwnAsk+=thisQuoteOwnAsk->liquidity;
                  nQuotesOwnAsk++;
                  currentOwnAskIndex--;
                  if((currentOwnAskIndex>=0) && (currentOwnAskIndex<NumberOwnQuotesAsk)) {
                      thisElementOwnAsk = &(OwnQuotesAsk[currentOwnAskIndex]);
                      thisQuoteOwnAsk=&(memMap->mapFBQuotesAsk[thisElementOwnAsk->tiIndex][sec][thisElementOwnAsk->quoteIndex]);
                  } else {
                      thisElementOwnAsk = NULL;
                      thisQuoteOwnAsk = NULL;
                      break; // to get out of the while loop
                  }
              } // while (thisElementOwnAsk has currentPrice)
          } // if (thisElementOwnAsk


          strcat(buffer, "<TR>\n");
          char qStr[SLEN];

          for(int ti=memMap->nTradingInterfaces-1; ti>=0; ti--) {

              if(nQuotesBid[ti]<=0)
                  sprintf(qStr,"<TD bgcolor=#BBFFFF></TD>\n");
              else if(aggBid[ti]<1000000)
                  sprintf(qStr,"<TD bgcolor=#BBFFFF>%dK [%d]</TD>\n",
                          aggBid[ti]/1000,
                          nQuotesBid[ti]
                         );
              else
                  sprintf(qStr,"<TD bgcolor=#BBFFFF>%.1fM [%d]</TD>\n",
                          ((double) aggBid[ti])/1000000.0,
                          nQuotesBid[ti]
                         );
              strcat(buffer, qStr);

          } // for (TIs)


          if(nQuotesOwnBid<=0)
              sprintf(qStr, "<TD></TD>");
          else if(aggOwnBid<1000000)
              sprintf(qStr, "<TD bgcolor=#008888><font color=#FFFFFF>%dK [%d]</TD>\n",
                      aggOwnBid/1000,
                      nQuotesOwnBid);
          else
              sprintf(qStr, "<TD bgcolor=#008888><font color=#FFFFFF>%.1fM [%d]</TD>\n",
                      ((double) aggOwnBid)/1000000,
                      nQuotesOwnBid);
          strcat(buffer, qStr);


          strcat(buffer, "<TD><b>\n");
          char priceStr[NLEN];
          renderPrice(priceStr, currentPrice, getSecurityDecimals(sec));
          strcat(buffer, priceStr);
          strcat(buffer, "</b></TD>\n");

          if(nQuotesOwnAsk<=0)
              sprintf(qStr, "<TD></TD>");
          else if(aggOwnAsk<1000000)
              sprintf(qStr, "<TD bgcolor=#008888><font color=#FFFFFF>%dK [%d]</TD>\n",
                      aggOwnAsk/1000,
                      nQuotesOwnAsk);
          else
              sprintf(qStr, "<TD bgcolor=#008888><font color=#FFFFFF>%.1fM [%d]</TD>\n",
                      ((double) aggOwnAsk)/1000000,
                      nQuotesOwnAsk);
          strcat(buffer, qStr);


          for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {

              char qStr[SLEN];
              if(nQuotesAsk[ti]<=0)
                  sprintf(qStr,"<TD bgcolor=#FFFFBB></TD>\n");
              else if(aggAsk[ti]<1000000)
                  sprintf(qStr,"<TD bgcolor=#FFFFBB>%dK [%d]</TD>\n",
                          aggAsk[ti]/1000,
                          nQuotesAsk[ti]
                         );
              else
                  sprintf(qStr,"<TD bgcolor=#FFFFBB>%.1fM [%d]</TD>\n",
                          ((double) aggAsk[ti])/1000000.0,
                          nQuotesAsk[ti]
                         );
              strcat(buffer, qStr);

          } // for (TIs)

          strcat(buffer, "</TR>\n"); // This ends up the row
          lastPrice=currentPrice;

      } // while (HTML rows)


      strcat(buffer, "</table><p>\n"); // This ends up the table


}



  void HTML_show_quotes_list(sortingFB_t quoteList[MAX_QUOTES], int numberOfQuotes, idtype sec, boolean isBid, boolean showTI) {

      if(numberOfQuotes<=0) {
          strcat(buffer, "(No quotes)\n");
          return;
      }

      strcat(buffer,
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Quote Nr</b></TH>\n"
            );

      if(showTI) {
          strcat(buffer,
                  "<TH align=\"center\"><font color=#FFFFFF><b>TI #</b></TH>\n"
                  "<TH align=\"center\"><font color=#FFFFFF><b>TI Name</b></TH>\n");
      }

      strcat(buffer,
              "<TH align=\"center\"><font color=#FFFFFF><b>Quote Price</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Liquidity</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Provider</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Entry index</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Replaces</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Quote time</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Bad?</b></TH>\n"
              "</TR>\n"
          );



      for(int q=0; q<numberOfQuotes; q++) {
          char qPrice[NLEN];
          char liqStr[NLEN];
          sortingFB_t *thisQuote=&(quoteList[q]);
          mtbookElement_t *thisElement = isBid ?
                                            &(memMap->mapFBQuotesBid[thisQuote->tiIndex][sec][thisQuote->quoteIndex]) :
                                            &(memMap->mapFBQuotesAsk[thisQuote->tiIndex][sec][thisQuote->quoteIndex]);
          renderPrice(qPrice,thisQuote->price,getSecurityDecimals(sec));
          char quoteTime[SLEN];
          timestampToDateStr(thisElement->timestamp.sec, quoteTime);
          sprintf(line, ".%03d", thisElement->timestamp.usec / 1000);
          strcat(quoteTime, line);
          if(thisElement->liquidity<1000000)
              sprintf(liqStr, "%d K",thisElement->liquidity/1000);
          else
              sprintf(liqStr, "%.1f M",((double) thisElement->liquidity)/1000000.0);

          sprintf(line,"<tr><td>%d</td>", q);
          strcat(buffer, line);
          if(showTI) {
              sprintf(line, "<td>%d</td><td>%s</td>",
                            thisQuote->tiIndex,
                            memMap->tradingInterfaceName[thisQuote->tiIndex]);
              strcat(buffer, line);
          }
          sprintf(line, "<td>%s</td><td>%s</td><td>%s</td><td>%d</td><td>%d</td><td>%s</td>%s</tr>\n",
                        qPrice,
                        liqStr,
                        ( isBid ?
                                memMap->mapFBQuotesProvidersBid[thisQuote->tiIndex][sec][thisQuote->quoteIndex] :
                                memMap->mapFBQuotesProvidersAsk[thisQuote->tiIndex][sec][thisQuote->quoteIndex] ),
                        thisElement->entryIndex,
                        thisElement->replaces,
                        quoteTime,
                        ( isBid ?
                                (
                                        (
                                                quotesTradingHistoryAsk[thisQuote->tiIndex][sec][thisQuote->quoteIndex].currentQuoteFailed &&
                                                (quotesTradingHistoryAsk[thisQuote->tiIndex][sec][thisQuote->quoteIndex].currentQuoteTimestamp.sec == memMap->mapFBQuotesAsk[thisQuote->tiIndex][sec][thisQuote->quoteIndex].timestamp.sec) &&
                                                (quotesTradingHistoryAsk[thisQuote->tiIndex][sec][thisQuote->quoteIndex].currentQuoteTimestamp.usec == memMap->mapFBQuotesAsk[thisQuote->tiIndex][sec][thisQuote->quoteIndex].timestamp.usec)
                                        ) ?
                                                "<TD align=\"center\" style=\"background-color: red\"><b>BAD</b></TD>" :
                                               "<td></td>"
                                ) :
                                (
                                       (
                                               quotesTradingHistoryBid[thisQuote->tiIndex][sec][thisQuote->quoteIndex].currentQuoteFailed &&
                                              (quotesTradingHistoryBid[thisQuote->tiIndex][sec][thisQuote->quoteIndex].currentQuoteTimestamp.sec == memMap->mapFBQuotesBid[thisQuote->tiIndex][sec][thisQuote->quoteIndex].timestamp.sec) &&
                                              (quotesTradingHistoryBid[thisQuote->tiIndex][sec][thisQuote->quoteIndex].currentQuoteTimestamp.usec == memMap->mapFBQuotesBid[thisQuote->tiIndex][sec][thisQuote->quoteIndex].timestamp.usec)
                                       ) ?
                                               "<TD align=\"center\" style=\"background-color: red\"><b>BAD</b></TD>" :
                                               "<td></td>"
                                )

                        )
                 );
          strcat(buffer, line);
      }
      strcat(buffer, "</table><p>\n");

  }




  void HTML_show_quotes_by_TI_list(sortingFBbyTI_t quoteList[MAX_QUOTES], int numberOfQuotes, idtype sec, idtype whichTI, boolean isBid) {

      if(numberOfQuotes<=0) {
          strcat(buffer, "(No quotes)\n");
          return;
      }

      strcat(buffer,
              "<table border=0>\n"
              "<TR bgcolor=#151540>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Quote Nr</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Quote Price</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Liquidity</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Provider</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Entry index</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Replaces</b></TH>\n"
              "<TH align=\"center\"><font color=#FFFFFF><b>Quote time</b></TH>\n"
              "</TR>\n"
          );

      for(int q=0; q<numberOfQuotes; q++) {
          char qPrice[NLEN];
          char liqStr[NLEN];
          sortingFBbyTI_t *thisQuote=&(quoteList[q]);
          mtbookElement_t *thisElement = isBid ?
                                            &(memMap->mapFBQuotesBid[whichTI][sec][thisQuote->quoteIndex]) :
                                            &(memMap->mapFBQuotesAsk[whichTI][sec][thisQuote->quoteIndex]);
          renderPrice(qPrice,thisQuote->price,getSecurityDecimals(sec));
          char quoteTime[SLEN];
          timestampToDateStr(thisElement->timestamp.sec, quoteTime);
          sprintf(line, ".%03d", thisElement->timestamp.usec / 1000);
          strcat(quoteTime, line);
          if(thisElement->liquidity<1000000)
              sprintf(liqStr, "%d K",thisElement->liquidity/1000);
          else
              sprintf(liqStr, "%.1f M",((double) thisElement->liquidity)/1000000.0);

          sprintf(line,"<tr><td>%d</td><td>%s</td><td>%s</td><td>%s</td><td>%d</td><td>%d</td><td>%s</td></tr>\n",
                        q,
                        qPrice,
                        liqStr,
                        ( isBid ?
                                memMap->mapFBQuotesProvidersBid[whichTI][sec][thisQuote->quoteIndex] :
                                memMap->mapFBQuotesProvidersAsk[whichTI][sec][thisQuote->quoteIndex] ),
                        thisElement->entryIndex,
                        thisElement->replaces,
                        quoteTime
                 );
          strcat(buffer, line);
      }
      strcat(buffer, "</table><p>\n");

  }



  void HTML_show_full_book(void) {

      sprintf(buffer,
              "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
              "<html><head></head><body>\n"
              "<FONT FACE=\"arial\">"
              "<b>Full Book (disaggregated)</b><br>\n"
              "<table><TR>"
              "<TD><form action=\"show_full_book\" method=\"get\"><input type=\"submit\" value=\"Refresh\"></form></TD>\n"
              "<TD><form action=\"report\" method=\"get\"><input type=\"submit\" value=\"Back to reporting page\"></form></TD>\n"
              "</table><br><p>\n"
             );

      char timeNow[SLEN];
      getStringTimeNowUTC(timeNow);
      sprintf(line, "Timestamp is %s<p>\n", timeNow);
      strcat(buffer, line);

      HTML_update_currency_shower("show_full_book");

      sortingFB_t OthersQuotesBid[MAX_QUOTES*MAX_NUMBER_TI];
      int NumberOthersQuotesBid;
      sortingFB_t OwnQuotesBid[MAX_TRADES_LIST_ALIVE];
      int NumberOwnQuotesBid;
      sortingFBbyTI_t OthersQuotesByTIBid[MAX_NUMBER_TI][MAX_QUOTES];
      int NumberOthersQuotesbyTIBid[MAX_NUMBER_TI];
      sortingFBbyTI_t OwnQuotesByTIBid[MAX_NUMBER_TI][MAX_TRADES_LIST_ALIVE];
      int NumberOwnQuotesbyTIBid[MAX_NUMBER_TI];

      removeOwnOrdersFromQuoteList(memMap,
                                   currentCurrency,
                                   true, // Bid
                                   OthersQuotesBid,
                                   &NumberOthersQuotesBid,
                                   OwnQuotesBid,
                                   &NumberOwnQuotesBid,
                                   OthersQuotesByTIBid,
                                   NumberOthersQuotesbyTIBid,
                                   OwnQuotesByTIBid,
                                   NumberOwnQuotesbyTIBid
                                   );


      sortingFB_t OthersQuotesAsk[MAX_QUOTES*MAX_NUMBER_TI];
      int NumberOthersQuotesAsk;
      sortingFB_t OwnQuotesAsk[MAX_TRADES_LIST_ALIVE];
      int NumberOwnQuotesAsk;
      sortingFBbyTI_t OthersQuotesByTIAsk[MAX_NUMBER_TI][MAX_QUOTES];
      int NumberOthersQuotesbyTIAsk[MAX_NUMBER_TI];
      sortingFBbyTI_t OwnQuotesByTIAsk[MAX_NUMBER_TI][MAX_TRADES_LIST_ALIVE];
      int NumberOwnQuotesbyTIAsk[MAX_NUMBER_TI];

      removeOwnOrdersFromQuoteList(memMap,
                                   currentCurrency,
                                   false, // Ask
                                   OthersQuotesAsk,
                                   &NumberOthersQuotesAsk,
                                   OwnQuotesAsk,
                                   &NumberOwnQuotesAsk,
                                   OthersQuotesByTIAsk,
                                   NumberOthersQuotesbyTIAsk,
                                   OwnQuotesByTIAsk,
                                   NumberOwnQuotesbyTIAsk
                                   );


      HTML_show_trading_interfaces();

      HTML_show_sorted_prices(currentCurrency);

      HTML_show_sorted_prices_ex_own_orders(currentCurrency);

      HMTL_show_full_book_for_a_security(OthersQuotesBid,
                                         NumberOthersQuotesBid,
                                         OwnQuotesBid,
                                         NumberOwnQuotesBid,
                                         OthersQuotesAsk,
                                         NumberOthersQuotesAsk,
                                         OwnQuotesAsk,
                                         NumberOwnQuotesAsk,
                                         currentCurrency);

      sprintf(line, "<p><span style=\"background-color:#FFFFBB\"><b>Complete book Ask quotes (%d in total):</b></span><p>\n", memMap->mapFBNumberofsortingAsk[currentCurrency]);
      strcat(buffer, line);
      HTML_show_quotes_list(memMap->mapFBSortingAsk[currentCurrency], memMap->mapFBNumberofsortingAsk[currentCurrency], currentCurrency, false, true);

      sprintf(line, "<p><span style=\"background-color:#BBFFFF\"><b>Complete book Bid quotes (%d in total):</b></span><p>\n", memMap->mapFBNumberofsortingBid[currentCurrency]);
      strcat(buffer, line);
      HTML_show_quotes_list(memMap->mapFBSortingBid[currentCurrency], memMap->mapFBNumberofsortingBid[currentCurrency], currentCurrency, true, true);


      strcat(buffer, "<p><span <span style=\"background-color:#FFFFBB\">Others' Ask quotes:</span><br>\n");
      HTML_show_quotes_list(OthersQuotesAsk, NumberOthersQuotesAsk, currentCurrency, false, true);
      strcat(buffer, "<br><span style=\"background-color:#FFFFBB\">Own Ask quotes:</span><br>\n");
      HTML_show_quotes_list(OwnQuotesAsk, NumberOwnQuotesAsk, currentCurrency, false, true);

      strcat(buffer, "<p><span style=\"background-color:#BBFFFF\">Others' Bid quotes:</span><br>\n");
      HTML_show_quotes_list(OthersQuotesBid, NumberOthersQuotesBid, currentCurrency, true, true);
      strcat(buffer, "<br><span style=\"background-color:#BBFFFF\">Own Bid quotes:\n</span><br>");
      HTML_show_quotes_list(OwnQuotesBid, NumberOwnQuotesBid, currentCurrency, true, true);


      for(int ti=0; ti<memMap->nTradingInterfaces; ti++) {

          sprintf(line, "<p><b>Quotes from TI #%d (%s):</b><p>\n", ti, memMap->tradingInterfaceName[ti]);
          strcat(buffer, line);

          strcat(buffer, "<table><tr><th bgcolor=#FFFFBB>Other's Ask</th><th bgcolor=#BBFFFF>Other's Bid</th></tr>\n<tr>\n");
          strcat(buffer, "<td>\n");
          HTML_show_quotes_by_TI_list(OthersQuotesByTIAsk[ti], NumberOthersQuotesbyTIAsk[ti], currentCurrency, ti, false);
          strcat(buffer, "</td>\n");
          strcat(buffer, "<td>\n");
          HTML_show_quotes_by_TI_list(OthersQuotesByTIBid[ti], NumberOthersQuotesbyTIBid[ti], currentCurrency, ti, true);
          strcat(buffer, "</td>\n</tr>\n");

          strcat(buffer, "<tr><th bgcolor=#FFFFBB>Own Ask</th><th bgcolor=#BBFFFF>Own Bid</th></tr>\n<tr>\n");
          strcat(buffer, "<td>\n");
          HTML_show_quotes_by_TI_list(OwnQuotesByTIAsk[ti], NumberOwnQuotesbyTIAsk[ti], currentCurrency, ti, false);
          strcat(buffer, "</td>\n");
          strcat(buffer, "<td>\n");
          HTML_show_quotes_by_TI_list(OwnQuotesByTIBid[ti], NumberOwnQuotesbyTIBid[ti], currentCurrency, ti, true);
          strcat(buffer, "</td>\n</tr></table><p>\n");

      }


      // Now debugging full book matrix rendering
      static uint             nColumns;
      static uint             nRows;
      static uint32           prices[MAX_FB_RENDER_ROWS];
      static uint32           midPrice;
      static char             headers[MAX_FB_RENDER_COLUMNS][NLEN];
      static char             toDisplay[MAX_FB_RENDER_ROWS][MAX_FB_RENDER_COLUMNS][NLEN];
      static idtype           colorToDisplay[MAX_FB_RENDER_ROWS][MAX_FB_RENDER_COLUMNS];
      static char             expand[MAX_FB_RENDER_ROWS][MAX_FB_RENDER_COLUMNS][SLEN];
      static uint32           totalLiquidity[MAX_FB_RENDER_ROWS][MAX_FB_RENDER_COLUMNS];
      static uint             manyQuotes[MAX_FB_RENDER_ROWS][MAX_FB_RENDER_COLUMNS];
      static boolean          tooManyRows;


      for(idtype sec=1; sec<NUM_SECURITIES; sec++) {

          sprintf(line, "<p><b>Showing renderFB data for %s</b><br>\n", securityName[sec]);
          strcat(buffer, line);

          renderFB(memMap, sec, false, &nColumns, &nRows, prices, &midPrice, headers, toDisplay, colorToDisplay, expand, totalLiquidity, manyQuotes, &tooManyRows);

          if(nRows <= 0) {
              strcat(buffer, "(No quotes)<br>\n");
              continue;
          }

          if(tooManyRows) {
              strcat(buffer, "Too many rows!! - some will not be rendered<br>\n");
          }

          strcat(buffer,
                  "<table border=0>\n"
                  "<TR bgcolor=#151540>\n"
                );

          for(int col=0; col<nColumns; col++) {

              sprintf(line, "<TH><font color=#FFFFFF>%s</TH>\n", headers[col]);
              strcat(buffer, line);

          }

          strcat(buffer, "</TR>\n");

          for(int row=0; row<nRows; row++) {

              strcat(buffer, "<TR>\n");
              for(int col=0; col<nColumns; col++) {

                  char colorS[NLEN];
                  switch(colorToDisplay[row][col]) {
                      case FB_RENDER_COLOR_EMPTY :
                          sprintf(colorS, "#FFFFFF");
                          break;
                      case FB_RENDER_COLOR_EMPTY_BID :
                          sprintf(colorS, "#EEFFFF");
                          break;
                      case FB_RENDER_COLOR_QUOTE_BID :
                          sprintf(colorS, "#CCFFFF");
                          break;
                      case FB_RENDER_COLOR_TI_TOB_BID :
                          sprintf(colorS, "#AAFFFF");
                          break;
                      case FB_RENDER_COLOR_OVERALL_TOB_BID :
                          sprintf(colorS, "#66FFFF");
                          break;
                      case FB_RENDER_COLOR_EMPTY_ASK :
                          sprintf(colorS, "#FFFFEE");
                          break;
                      case FB_RENDER_COLOR_QUOTE_ASK :
                          sprintf(colorS, "#FFFFCC");
                          break;
                      case FB_RENDER_COLOR_TI_TOB_ASK :
                          sprintf(colorS, "#FFFFAA");
                          break;
                      case FB_RENDER_COLOR_OVERALL_TOB_ASK :
                          sprintf(colorS, "#FFFF66");
                          break;
                      case FB_RENDER_COLOR_OWN_BUY :
                          sprintf(colorS, "#AADDDD");
                          break;
                      case FB_RENDER_COLOR_OWN_SELL :
                          sprintf(colorS, "#DDDDAA");
                          break;
                      case FB_RENDER_COLOR_PRICE :
                          sprintf(colorS, "#DDDDDD");
                          break;

                  }

                  sprintf(line, "<TD bgcolor=%s>%s\n", colorS, toDisplay[row][col]);
                  if(strlen(expand[row][col])>0) {
                      strcat(line, " [");
                      strcat(line, expand[row][col]);
                      strcat(line, "]");
                  }
                  strcat(line, "</TD>\n");
                  strcat(buffer, line);

              } //Loop: columns

              strcat(buffer, "</TR>\n");

          } // Loop: rows


          strcat(buffer, "</table>\n");

      }


      strcat(buffer,
              "</FONT>\n</body></html>\n"
             );

  }




  void HTML_authenticate(void) {

      sprintf(buffer,
              "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
              "<html><head>\n"
              "<script language=\"javascript\" type=\"text/javascript\">\n"
              "function SetFocus(InputID) { document.getElementById(InputID).focus();}\n"
              "</script>\n"
              "</head><body onload=\"SetFocus('PasswdBox')\">\n"
              "<FONT FACE=\"arial\">"
              "<b>Enter password:</b>"
              "<form action=\"do_authenticate\" method=\"post\">"
              "<input type=\"password\" name=\"passwd\" id=\"PasswdBox\">"
              "<input type=\"submit\" value=\"Authenticate\"></form>\n"
              "</FONT>\n</body></html>\n"
             );

  }



  void HTML_do_authenticate(void) {

      char passwd[SLEN];
      boolean passwd_set=false;
      boolean authenticated=false;

      for(int i = 0; i < post_data_index; i++) {

          if(!strcmp("passwd", post_data[i].key)) {
              passwd_set = true;
              strcpy(passwd, post_data[i].value);
          }

      }

      if(passwd_set && !strcmp(passwd, PASSWORD)) {

          // password correct
          timestruct_t now_strtime;
          timetype_t now_systime;
          getStructTimeNowUTC(&now_strtime, &now_systime);

          for(int i=0; i<MAX_AUTH_CONNECTIONS; i++) {

              if(now_systime.sec-auth_data[i].timestamp > AUTH_TIMEOUT) {

                  // This one is free
                  const union MHD_ConnectionInfo *CI = MHD_get_connection_info (connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
                  auth_data[i].timestamp=now_systime.sec;
                  auth_data[i].tls_session=CI->tls_session;
                  for(int j=0; j<14; j++) {
                      auth_data[i].sa_data[j]=CI->client_addr->sa_data[j];
                  }
                  authenticated=true;
                  break;
              }

          }

          if(!authenticated) {

              sprintf(buffer,
                      "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
                      "<html><head></head><body>\n"
                      "<FONT FACE=\"arial\">"
                      "<b>Too many people snooping!!:</b>"
                      "<form action=\"authenticate\" method=\"gest\"><input type=\"submit\" value=\"Back to authentication page\"></form>\n"
                      "</FONT>\n</body></html>\n"
                     );

          } else {

              HTML_jump_to("report", 0);

          }

      } else {

          HTML_jump_to("authenticate", 0);

      }

  }



  boolean is_this_connection_authenticated(void) {

      boolean authenticated=false;
      const union MHD_ConnectionInfo *CI = MHD_get_connection_info (connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
      timestruct_t now_strtime;
      timetype_t now_systime;
      getStructTimeNowUTC(&now_strtime, &now_systime);

      for(int i=0; i<MAX_AUTH_CONNECTIONS; i++) {

          if(
                  (now_systime.sec-auth_data[i].timestamp <= AUTH_TIMEOUT) &&
//                  (auth_data[i].tls_session == CI->tls_session) && // So this works as well in cell phones
                  (auth_data[i].sa_data[2] == CI->client_addr->sa_data[2]) &&
                  (auth_data[i].sa_data[3] == CI->client_addr->sa_data[3]) &&
                  (auth_data[i].sa_data[4] == CI->client_addr->sa_data[4]) &&
                  (auth_data[i].sa_data[5] == CI->client_addr->sa_data[5]) &&
                  (auth_data[i].sa_data[6] == CI->client_addr->sa_data[6]) &&
                  (auth_data[i].sa_data[7] == CI->client_addr->sa_data[7])
             ) {

              // the connection is authenticated
              auth_data[i].timestamp=now_systime.sec; // to reset timeout
              authenticated=true;
              break;
          }

      }

      return authenticated;

  }

  ///////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////



  total_equity_STR = memMap->AccountingUnit[auIndex].GlobalEquity_STR;
  total_equity_EP = memMap->EquityPool[epIndex].GlobalEquity_EP;
  total_margin = 0;
  total_free_margin = 0;
  for(idtype i = 0; i < memMap->nPrimeBrokers; i++) {
      total_margin += memMap->AccountingUnit[auIndex].UsedMargin_STR[i];
      total_free_margin += memMap->AccountingUnit[auIndex].FreeMargin_STR[i];
  }


  renderWithDots(total_equity_STR_S, total_equity_STR, false);
  renderWithDots(total_equity_EP_S, total_equity_EP, false);
  renderWithDots(total_marginS, total_margin, false);
  renderPct(pctGainS, total_equity_STR, total_equity_EP - total_equity_STR, true);
  renderPct(pctMarginUsedS, total_margin, total_equity_EP, false);
  renderWithDots(free_marginS, total_free_margin, false);



  // Now responding to HTML navigation:

  if(!strcmp("/do_authenticate", url)) {

      HTML_do_authenticate();

  } else if(PASSWORD_ON && !is_this_connection_authenticated()) {

      HTML_authenticate();

  } else if(!strcmp("/", url)) {

      HTML_jump_to("report", 0);

  } else if(!strcmp("/report", url)) {

      HTML_show_report();

  } else if(!strcmp("/toggle_auto_management", url)) {

      HTML_toggle_auto_management();

  } else if(!strcmp("/toggle_strategy", url)) {

      HTML_toggle_strategy();

  } else if(!strcmp("/reset_valuation_change_alarm", url)) {

      HTML_reset_valuation_change_alarm();

  } else if(!strcmp("/reset_drawdown_alarm", url)) {

      HTML_reset_drawdown_alarm();

  } else if(!strcmp("/reset_too_many_trades_alarm", url)) {

      HTML_reset_too_many_trades_alarm();

  } else if(!strcmp("/reset_malfunction_alarm", url)) {

      HTML_reset_malfunction_alarm();

  } else if(!strcmp("/config", url)) {

      HTML_show_config();

  } else if(!strcmp("/do_system_config", url)) {

      HTML_do_config(system_params);

  } else if(!strcmp("/do_strategy_config", url)) {

      HTML_do_config(strategy_params);

  } else if(!strcmp("/trade", url)) {

      HTML_show_trade();

  } else if(!strcmp("/do_trade", url)) {

      HTML_do_trade();

  } else if(!strcmp("/do_cancel", url)) {

      HTML_do_cancel();

  } else if(!strcmp("/do_replace", url)) {

      HTML_do_replace();

  } else if(!strcmp("/cancel_all", url)) {

      HTML_do_cancel_all();

  } else if(!strcmp("/close_all", url)) {

      HTML_do_close_all();

  } else if(!strcmp("/do_resolve", url)) {

      HTML_do_resolve();

  } else if(!strcmp("/backoffice", url)) {

      HTML_show_backoffice();

  } else if(!strcmp("/do_backoffice_change_cash", url)) {

      HTML_do_backoffice_change_cash();

  } else if(!strcmp("/do_backoffice_change_posis", url)) {

      HTML_do_backoffice_change_posis();

  } else if(!strcmp("/do_initialize_all_bo", url)) {

      HTML_do_initialize_all_bo();

  } else if(!strcmp("/debug", url)) {

      HTML_show_debug();

  } else if(!strcmp("/indicators", url)) {

      HTML_show_indicators();

  } else if(!strcmp("/reset_notifications", url)) {

      HTML_reset_notifications();

  } else if(!strcmp("/reset_cards", url)) {

      HTML_reset_cards();

  } else if(!strcmp("/add_notification", url)) {

      HTML_add_notification();

  } else if(!strcmp("/latency_histograms", url)) {

      HTML_show_latency_histograms();

  } else if(!strcmp("/print_samples", url)) {

      HTML_do_print_samples();

  } else if(!strcmp("/show_full_book", url)) {

      HTML_show_full_book();

  } else if(!strcmp("/do_change_currency", url)) {

      HTML_do_change_current_currency();

  } else if(!strcmp("/do_change_strategy_currency", url)) {

      HTML_do_change_strategy_currency();

  } else if(!strcmp("/do_toggle_ti_mark", url)) {

      HTML_do_toggle_ti_mark();

  } else {

      HTML_default_response();
  }


  // Finally, we create the http_response

  response = MHD_create_response_from_buffer (strlen (buffer), (void *) buffer, MHD_RESPMEM_PERSISTENT);
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);


  // And now releasing the map
  releaseMap();

  return ret;

}



