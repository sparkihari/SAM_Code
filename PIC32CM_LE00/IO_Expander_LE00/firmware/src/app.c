/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

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

#include "app.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

#define IODIRA   	0x00
#define IODIRB   	0x01
#define IPOLA    	0x02
#define IPOLB    	0x03
#define GPINTENA 	0x04
#define GPINTENB 	0x05
#define DEFVALA  	0x06
#define DEFVALB  	0x07
#define INTCONA  	0x08
#define INTCONB  	0x09
#define IOCONA   	0x0A
#define IOCONB   	0x0B
#define GPPUA    	0x0C
#define GPPUB    	0x0D
#define INTFA    	0x0E
#define INTFB    	0x0F
#define INTCAPA  	0x010
#define INTCAPB  	0x011
#define GPIOA    	0x012
#define GPIOB    	0x013
#define OLATA    	0x014
#define OLATB    	0x015

#define MCP_SLAVE_ADDR 0x27 

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;

uint8_t i, led_pattern = 0;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

static void APP_I2C_EventHandler( DRV_I2C_TRANSFER_EVENT event,
    DRV_I2C_TRANSFER_HANDLE transferHandle, uintptr_t context)
{
    appData.transferStatus = event;
}


// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


bool i2c_write(uint8_t reg, uint8_t data) 
{
    appData.txBuffer[0] = reg;
    appData.txBuffer[1] = data;

    DRV_I2C_WriteTransferAdd(appData.i2cHandle, MCP_SLAVE_ADDR, 
            appData.txBuffer, 2, &appData.transferHandle);
    
    if (appData.transferHandle == DRV_I2C_TRANSFER_HANDLE_INVALID)
    {
        return false;        
    }
    
    SYSTICK_DelayMs(1);
    while (appData.transferStatus != DRV_I2C_TRANSFER_EVENT_COMPLETE);
    
    return true;
}

uint8_t i2c_read(uint8_t reg) 
{
    appData.txBuffer[0] = reg;
    
    DRV_I2C_WriteReadTransferAdd(appData.i2cHandle, MCP_SLAVE_ADDR, 
            appData.txBuffer, 1, appData.rxBuffer, 1, 
            &appData.transferHandle);
    
    if (appData.transferHandle == DRV_I2C_TRANSFER_HANDLE_INVALID)
    {
        return false;        
    }
    SYSTICK_DelayMs(1);
    while (appData.transferStatus != DRV_I2C_TRANSFER_EVENT_COMPLETE);
    
    return true;
}

uint8_t i2c_test(void) 
{    
    for (int i=0; i<128; i++)
    {
        DRV_I2C_ReadTransferAdd(appData.i2cHandle, i, 
                appData.rxBuffer, 1, &appData.transferHandle);

        if (appData.transferHandle == DRV_I2C_TRANSFER_HANDLE_INVALID)
        {
            return false;        
        }
        SYSTICK_DelayMs(1);
    }
    
    return true;
}


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;
    appData.i2cHandle       = DRV_HANDLE_INVALID;
    appData.transferHandle  = DRV_I2C_TRANSFER_HANDLE_INVALID;
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks ( void )
{

    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            /* Open I2C driver client */
            appData.i2cHandle = DRV_I2C_Open( DRV_I2C_INDEX_0, DRV_IO_INTENT_READWRITE);    
                        
            if(appData.i2cHandle != DRV_HANDLE_INVALID)
            {
                /* Register the I2C Driver client event callback */
                DRV_I2C_TransferEventHandlerSet(appData.i2cHandle, APP_I2C_EventHandler, 0); 
            
                appData.state = APP_STATE_SERVICE_TASKS;
                SYSTICK_TimerStart();
                
                printf("\n\r APP_TASK: MCP23017 LS60 Test");
                                
            }
            else
            {
                appData.state = APP_STATE_ERROR;
            }

			break;
        }

        case APP_STATE_SERVICE_TASKS:
        {
            i2c_write(GPPUA,  0x0F);    
            i2c_write(IOCONA, 0x40);    
            i2c_write(IODIRA, 0x00);    
            i2c_write(IODIRB, 0x00);    
            
            printf("\n\r APP_TASK: MCP23017 Configuration is Done");
            
            appData.state = APP_STATE_IDLE;
            break;
        }

        case APP_STATE_IDLE:
        {
            led_pattern = 1 << i;
            
            i2c_write(GPIOA,  led_pattern);
            SYSTICK_DelayMs(100);
            
            if(i >= 7)   i=0;
            
            else i++;

            break;
        }
        
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
