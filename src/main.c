/*--------------------------------------------------------------------------------------------------
-- Project     :    StubHFT
-- Filename    :    name: main.c
--                  created_by: carlos
--                  date_created: Jun 12, 2013
--------------------------------------------------------------------------------------------------
-- File Purpose: Main application to run remote stub
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

#include "hftUtils_types.h"
#include "main.h"
#include "wserver.h"
#include "stubDbg.h"

/* Static data */
static boolean exitProgram = false;


/* Main function definition: don touch it */
int main(int argc, char* argv[])
{
    // main application
    int32  result = OK;

    /* Get parameters and check it */
    /* Main parameters are:
     * First: accounting Id identifier (supplied by your manager)
     * Second: IP address of core machine in IPv4 dot notation
     * Third: Port number of core machine server (1234 as default)
     */
    if (checkParams (argc, argv) == OK)
    {
        /* launch debug system */
        stubDbgInit ();

        /* Launch stratetegy thread and memory map initialization */
        result = strategyStart ();

        if (result == OK)
        {
            /* Init webserver */
//            webServerStart (getWebPort()); // Taken to strategy.c

            /* If strategy was appropriately initialized keep this loop as supervision */
            while (!exitProgram)
            {
                /* watchdog can use 'exitProgram' to finish the program */

                /* Watchdog supervision */
                /* Wait for 1 seconds to evaluate watchdog again */
                usleep (1000000);
            }

            /* Stop web server */
//            webServerStop (); // Taken to strategy.c

            /* If exit if called from this thread, stop strategy before exiting */
            strategyStop ();
        }
        else
        {
            fprintf (stderr, "remoteStub: ERROR starting strategy thread (result = %d)\n", result);
        }

        /* stop debug thread */
        stubDbgTerm ();
    }

    return result;
}

/* terminate program, global exit */
void exitStub (void)
{
    exitProgram = true;

    return;
}

/* Strategy exit, for multiple strategies */
void exitFromCoreToStrategy (idtype strategyId)
{
    fprintf (stdout, "EXIT command received from core for Strategy %d\n", strategyId);
    return;
}

