/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_com.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "app_com.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_COM_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

APP_COM_DATA app_comData;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_COM_Initialize ( void )

  Remarks:
    See prototype in app_com.h.
 */

void APP_COM_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    app_comData.state = APP_COM_STATE_WAIT_USB_CONSOLE_CONFIGURED;
    
    SYSTICK_TimerStart();
}


/******************************************************************************
  Function:
    void APP_COM_Tasks ( void )

  Remarks:
    See prototype in app_com.h.
 */

void APP_COM_Tasks ( void )
{

    /* Check the application's current state. */
    switch ( app_comData.state )
    {
        case APP_COM_STATE_WAIT_USB_CONSOLE_CONFIGURED:
            if (SYS_CONSOLE_Status(SYS_CONSOLE_INDEX_0) == SYS_STATUS_READY)
            {
                app_comData.state = APP_COM_STATE_GET_CONSOLE_HANDLE;
            }
            break;

        case APP_COM_STATE_GET_CONSOLE_HANDLE:
            /* Get handles to both the USB console instances */
            app_comData.console0Handle = SYS_CONSOLE_HandleGet(SYS_CONSOLE_INDEX_0);
            app_comData.console1Handle = SYS_CONSOLE_HandleGet(SYS_CONSOLE_INDEX_1);

            if ((app_comData.console0Handle != SYS_CONSOLE_HANDLE_INVALID) &&
                    (app_comData.console1Handle != SYS_CONSOLE_HANDLE_INVALID))
            {
                app_comData.state = APP_COM_STATE_IDLE;
            }
            else
            {
                app_comData.state = APP_COM_STATE_ERROR;
            }
            break;
            
        case APP_COM_STATE_IDLE:
            SYSTICK_DelayMs(2000);
            SYS_CONSOLE_PRINT("\n\r CONSOLE is Ready ");
            app_comData.state = APP_COM_STATE_ERROR;
            break;
        
        case APP_COM_STATE_ERROR:
        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}


/*******************************************************************************
 End of File
 */
