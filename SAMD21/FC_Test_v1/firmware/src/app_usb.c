/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_usb.c

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

#include "app_usb.h"

#define APP_READ_BUFFER_SIZE                                512
#define APP_USB_SWITCH_DEBOUNCE_COUNT_FS                    150
#define APP_USB_SWITCH_DEBOUNCE_COUNT_HS                    1200
// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************
const uint8_t __attribute__((aligned(16))) switchPromptUSB[] = "\r\nPUSH BUTTON PRESSED";

uint8_t CACHE_ALIGN cdcReadBuffer[APP_READ_BUFFER_SIZE];
uint8_t CACHE_ALIGN cdcWriteBuffer[APP_READ_BUFFER_SIZE];
// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_USB_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

APP_USB_DATA app_usbData;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************


/*******************************************************
 * USB CDC Device Events - Application Event Handler
 *******************************************************/

USB_DEVICE_CDC_EVENT_RESPONSE APP_USBDeviceCDCEventHandler
(
    USB_DEVICE_CDC_INDEX index,
    USB_DEVICE_CDC_EVENT event,
    void * pData,
    uintptr_t userData
)
{
    APP_USB_DATA * app_usbDataObject;
    USB_CDC_CONTROL_LINE_STATE * controlLineStateData;
    USB_DEVICE_CDC_EVENT_DATA_READ_COMPLETE * eventDataRead;
    
    app_usbDataObject = (APP_USB_DATA *)userData;

    switch(event)
    {
        case USB_DEVICE_CDC_EVENT_GET_LINE_CODING:

            /* This means the host wants to know the current line
             * coding. This is a control transfer request. Use the
             * USB_DEVICE_ControlSend() function to send the data to
             * host.  */

            USB_DEVICE_ControlSend(app_usbDataObject->deviceHandle,
                    &app_usbDataObject->getLineCodingData, sizeof(USB_CDC_LINE_CODING));

            break;

        case USB_DEVICE_CDC_EVENT_SET_LINE_CODING:

            /* This means the host wants to set the line coding.
             * This is a control transfer request. Use the
             * USB_DEVICE_ControlReceive() function to receive the
             * data from the host */

            USB_DEVICE_ControlReceive(app_usbDataObject->deviceHandle,
                    &app_usbDataObject->setLineCodingData, sizeof(USB_CDC_LINE_CODING));

            break;

        case USB_DEVICE_CDC_EVENT_SET_CONTROL_LINE_STATE:

            /* This means the host is setting the control line state.
             * Read the control line state. We will accept this request
             * for now. */

            controlLineStateData = (USB_CDC_CONTROL_LINE_STATE *)pData;
            app_usbDataObject->controlLineStateData.dtr = controlLineStateData->dtr;
            app_usbDataObject->controlLineStateData.carrier = controlLineStateData->carrier;

            USB_DEVICE_ControlStatus(app_usbDataObject->deviceHandle, USB_DEVICE_CONTROL_STATUS_OK);

            break;

        case USB_DEVICE_CDC_EVENT_SEND_BREAK:

            /* This means that the host is requesting that a break of the
             * specified duration be sent. Read the break duration */

            app_usbDataObject->breakData = ((USB_DEVICE_CDC_EVENT_DATA_SEND_BREAK *)pData)->breakDuration;
            
            /* Complete the control transfer by sending a ZLP  */
            USB_DEVICE_ControlStatus(app_usbDataObject->deviceHandle, USB_DEVICE_CONTROL_STATUS_OK);
            
            break;

        case USB_DEVICE_CDC_EVENT_READ_COMPLETE:

            /* This means that the host has sent some data*/
            eventDataRead = (USB_DEVICE_CDC_EVENT_DATA_READ_COMPLETE *)pData;
            app_usbDataObject->isReadComplete = true;
            app_usbDataObject->numBytesRead = eventDataRead->length; 
            break;

        case USB_DEVICE_CDC_EVENT_CONTROL_TRANSFER_DATA_RECEIVED:

            /* The data stage of the last control transfer is
             * complete. For now we accept all the data */

            USB_DEVICE_ControlStatus(app_usbDataObject->deviceHandle, USB_DEVICE_CONTROL_STATUS_OK);
            break;

        case USB_DEVICE_CDC_EVENT_CONTROL_TRANSFER_DATA_SENT:

            /* This means the GET LINE CODING function data is valid. We don't
             * do much with this data in this demo. */
            break;

        case USB_DEVICE_CDC_EVENT_WRITE_COMPLETE:

            /* This means that the data write got completed. We can schedule
             * the next read. */

            app_usbDataObject->isWriteComplete = true;
            break;

        default:
            break;
    }

    return USB_DEVICE_CDC_EVENT_RESPONSE_NONE;
}

/***********************************************
 * Application USB Device Layer Event Handler.
 ***********************************************/
void USB_Device_Event_Callback 
(
    USB_DEVICE_EVENT event, 
    void * eventData, 
    uintptr_t context 
)
{
    USB_DEVICE_EVENT_DATA_CONFIGURED *configuredEventData;

    switch(event)
    {
        case USB_DEVICE_EVENT_SOF:

            /* This event is used for switch debounce. This flag is reset
             * by the switch process routine. */
            app_usbData.sofEventHasOccurred = true;
            
            break;

        case USB_DEVICE_EVENT_RESET:

            app_usbData.isConfigured = false;

            break;

        case USB_DEVICE_EVENT_CONFIGURED:

            /* Check the configuration. We only support configuration 1 */
            configuredEventData = (USB_DEVICE_EVENT_DATA_CONFIGURED*)eventData;
            
            if ( configuredEventData->configurationValue == 1)
            {
                
                /* Register the CDC Device application event handler here.
                 * Note how the app_usbData object pointer is passed as the
                 * user data */

                USB_DEVICE_CDC_EventHandlerSet(USB_DEVICE_CDC_INDEX_0, APP_USBDeviceCDCEventHandler, (uintptr_t)&app_usbData);

                /* Mark that the device is now configured */
                app_usbData.isConfigured = true;
            }
            
            break;

        case USB_DEVICE_EVENT_POWER_DETECTED:

            /* VBUS was detected. We can attach the device */
            USB_DEVICE_Attach(app_usbData.deviceHandle);
            
            break;

        case USB_DEVICE_EVENT_POWER_REMOVED:

            /* VBUS is not available any more. Detach the device. */
            USB_DEVICE_Detach(app_usbData.deviceHandle);
            
            break;

        case USB_DEVICE_EVENT_SUSPENDED:
            
            break;

        case USB_DEVICE_EVENT_RESUMED:
        case USB_DEVICE_EVENT_ERROR:
        default:
            
            break;
    }
}


// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


void APP_ProcessSwitchPress(void)
{
    /* This function checks if the switch is pressed and then
     * debounces the switch press*/
    
//    if(!SWITCH_Get())
    {
        if(app_usbData.ignoreSwitchPress)
        {
            /* This means the key press is in progress */
            if(app_usbData.sofEventHasOccurred)
            {
                /* A timer event has occurred. Update the debounce timer */
                app_usbData.switchDebounceTimer ++;
                app_usbData.sofEventHasOccurred = false;
                
                if (USB_DEVICE_ActiveSpeedGet(app_usbData.deviceHandle) == USB_SPEED_FULL)
                {
                    app_usbData.debounceCount = APP_USB_SWITCH_DEBOUNCE_COUNT_FS;
                }
                else if (USB_DEVICE_ActiveSpeedGet(app_usbData.deviceHandle) == USB_SPEED_HIGH)
                {
                    app_usbData.debounceCount = APP_USB_SWITCH_DEBOUNCE_COUNT_HS;
                }
                if(app_usbData.switchDebounceTimer == app_usbData.debounceCount)
                {
                    /* Indicate that we have valid switch press. The switch is
                     * pressed flag will be cleared by the application tasks
                     * routine. We should be ready for the next key press.*/
                    app_usbData.isSwitchPressed = true;
                    app_usbData.switchDebounceTimer = 0;
                    app_usbData.ignoreSwitchPress = false;
                }
            }
        }
        else
        {
            /* We have a fresh key press */
            app_usbData.ignoreSwitchPress = true;
            app_usbData.switchDebounceTimer = 0;
        }
    }
    else
    {
        /* No key press. Reset all the indicators. */
        app_usbData.ignoreSwitchPress = false;
        app_usbData.switchDebounceTimer = 0;
        app_usbData.sofEventHasOccurred = false;
    }
}

/*****************************************************
 * This function is called in every step of the
 * application state machine.
 *****************************************************/

bool APP_StateReset(void)
{
    /* This function returns true if the device
     * was reset  */

    bool retVal;

    if(app_usbData.isConfigured == false)
    {
        app_usbData.state = APP_USB_STATE_WAIT_FOR_CONFIGURATION;
        app_usbData.readTransferHandle = USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;
        app_usbData.writeTransferHandle = USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;
        app_usbData.isReadComplete = true;
        app_usbData.isWriteComplete = true;
        retVal = true;
    }
    else
    {
        retVal = false;
    }

    return(retVal);
}


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_USB_Initialize ( void )

  Remarks:
    See prototype in app_usb.h.
 */

void APP_USB_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    app_usbData.state = APP_USB_STATE_INIT;

    /* Device Layer Handle  */
    app_usbData.deviceHandle = USB_DEVICE_HANDLE_INVALID ;

    /* Device configured status */
    app_usbData.isConfigured = false;

    /* Initial get line coding state */
    app_usbData.getLineCodingData.dwDTERate = 9600;
    app_usbData.getLineCodingData.bParityType = 0;
    app_usbData.getLineCodingData.bCharFormat = 0;
    app_usbData.getLineCodingData.bDataBits = 8;

    /* Read Transfer Handle */
    app_usbData.readTransferHandle = USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;

    /* Write Transfer Handle */
    app_usbData.writeTransferHandle = USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;

    /* Initialize the read complete flag */
    app_usbData.isReadComplete = true;

    /*Initialize the write complete flag*/
    app_usbData.isWriteComplete = true;

    /* Initialize Ignore switch flag */
    app_usbData.ignoreSwitchPress = false;

    /* Reset the switch debounce counter */
    app_usbData.switchDebounceTimer = 0;

    /* Reset other flags */
    app_usbData.sofEventHasOccurred = false;
    
    /* To know status of Switch */
    app_usbData.isSwitchPressed = false;

    /* Set up the read buffer */
    app_usbData.cdcReadBuffer = &cdcReadBuffer[0];

    /* Set up the read buffer */
    app_usbData.cdcWriteBuffer = &cdcWriteBuffer[0];  
}


/******************************************************************************
  Function:
    void APP_USB_Tasks ( void )

  Remarks:
    See prototype in app_usb.h.
 */

void APP_USB_Tasks ( void )
{

    /* Check the application's current state. */
    switch ( app_usbData.state )
    {
        case APP_USB_STATE_INIT:
        {
            
            app_usbData.deviceHandle = USB_DEVICE_Open(USB_DEVICE_INDEX_0, DRV_IO_INTENT_READWRITE);
            
            if (app_usbData.deviceHandle != USB_DEVICE_HANDLE_INVALID) 
            {
                USB_DEVICE_EventHandlerSet(app_usbData.deviceHandle, USB_Device_Event_Callback, (uintptr_t) & app_usbData);
                app_usbData.state = APP_USB_STATE_WAIT_FOR_CONFIGURATION;
            }
            break;
        }

        case APP_USB_STATE_WAIT_FOR_CONFIGURATION:

            /* Check if the device was configured */
            if(app_usbData.isConfigured)
            {
                /* If the device is configured then lets start reading */
                //app_usbData.state = APP_USB_STATE_SCHEDULE_READ;
                app_usbData.state = APP_USB_STATE_CHECK_SWITCH_PRESSED;
            }
            
            break;

        case APP_USB_STATE_SCHEDULE_READ:

            if(APP_StateReset())
            {
                break;
            }

            /* If a read is complete, then schedule a read
             * else wait for the current read to complete */

            app_usbData.state = APP_USB_STATE_WAIT_FOR_READ_COMPLETE;
            if(app_usbData.isReadComplete == true)
            {
                app_usbData.isReadComplete = false;
                app_usbData.readTransferHandle =  USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;

                USB_DEVICE_CDC_Read (USB_DEVICE_CDC_INDEX_0,
                        &app_usbData.readTransferHandle, app_usbData.cdcReadBuffer,
                        APP_READ_BUFFER_SIZE);
                
                if(app_usbData.readTransferHandle == USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID)
                {
                    app_usbData.state = APP_USB_STATE_ERROR;
                    break;
                }
            }

            break;

        case APP_USB_STATE_WAIT_FOR_READ_COMPLETE:
        case APP_USB_STATE_CHECK_SWITCH_PRESSED:

            if(APP_StateReset())
            {
                break;
            }

            APP_ProcessSwitchPress();

            /* Check if a character was received or a switch was pressed.
             * The isReadComplete flag gets updated in the CDC event handler. */

            if(app_usbData.isReadComplete && app_usbData.isSwitchPressed)
            {
                app_usbData.state = APP_USB_STATE_SCHEDULE_WRITE;
            }

            break;


        case APP_USB_STATE_SCHEDULE_WRITE:

            if(APP_StateReset())
            {
                break;
            }

            /* Setup the write */

            app_usbData.writeTransferHandle = USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;
            app_usbData.isWriteComplete = false;
            app_usbData.state = APP_USB_STATE_WAIT_FOR_WRITE_COMPLETE;

            if(app_usbData.isSwitchPressed)
            {
                /* If the switch was pressed, then send the switch prompt*/
                app_usbData.isSwitchPressed = false;
                USB_DEVICE_CDC_Write(USB_DEVICE_CDC_INDEX_0,
                        &app_usbData.writeTransferHandle, switchPromptUSB, sizeof(switchPromptUSB),
                        USB_DEVICE_CDC_TRANSFER_FLAGS_DATA_COMPLETE);
            }
//            else
//            {
//                /* Else echo each received character by adding 1 */
//                for(i = 0; i < app_usbData.numBytesRead; i++)
//                {
//                    if((app_usbData.cdcReadBuffer[i] != 0x0A) && (app_usbData.cdcReadBuffer[i] != 0x0D))
//                    {
//                        app_usbData.cdcWriteBuffer[i] = app_usbData.cdcReadBuffer[i] + 1;
//                    }
//                }
//                USB_DEVICE_CDC_Write(USB_DEVICE_CDC_INDEX_0,
//                        &app_usbData.writeTransferHandle,
//                        app_usbData.cdcWriteBuffer, app_usbData.numBytesRead,
//                        USB_DEVICE_CDC_TRANSFER_FLAGS_DATA_COMPLETE);
//            }

            break;

        case APP_USB_STATE_WAIT_FOR_WRITE_COMPLETE:

            if(APP_StateReset())
            {
                break;
            }

            /* Check if a character was sent. The isWriteComplete
             * flag gets updated in the CDC event handler */

            if(app_usbData.isWriteComplete == true)
            {
                app_usbData.state = APP_USB_STATE_IDLE;
            }

            break;

        case APP_USB_STATE_ERROR:
        default:
            
            break;        
        /* State machine idle state */
        case APP_USB_STATE_IDLE:
        {
            app_usbData.state = APP_USB_STATE_CHECK_SWITCH_PRESSED;
            break;
        }
    }
}


/*******************************************************************************
 End of File
 */
