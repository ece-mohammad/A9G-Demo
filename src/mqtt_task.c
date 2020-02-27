/* ------------------------------------------------------------------------- */
/* ------------------------------ mqtt_task.c ------------------------------ */
/* ------------------------------------------------------------------------- */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "api_os.h"
#include "api_event.h"
#include "api_debug.h"
#include "api_sim.h"

#include "api_network.h"
#include "api_mqtt.h"
#include "api_info.h"

#include "api_hal_uart.h"

#include "../include/mqtt_task.h"

/* ------------------------------------------------------------------------- */
/* --------------------- Global Variables & Definitions -------------------- */
/* ------------------------------------------------------------------------- */

extern bool bNetworkRegisteration;

HANDLE mqttTaskHandle;
HANDLE uartTaskHandle;

static const Network_PDP_Context_t MQTT_sNetworkContext = {
    .apn = MQTT_NETWORK_AP_NAME,
    .userName = MQTT_NETWORK_USERNAME,
    .userPasswd = MQTT_NETWORK_PASSWORD,
};

static bool bNetworkActivated = false;

static uint8_t DeviceIMEI[20] = {0};
static uint8_t DeviceICCID[20] = {0};
static uint8_t DeviceIMSI[20] = {0};

static MQTT_Connect_Info_t MQTT_sConnectionInfo = {0};
static MQTT_Client_t *MQTT_psClient = NULL;

static uint8_t MQTT_au8Rxbuffer[1024] = {0};

static MQTT_Err_t MQTT_eLastError = MQTT_ERR_NONE;
static MQTT_Error_t MQTT_eErrorCause = MQTT_ERROR_NONE;
static MQTT_State_t MQTT_eCurrentState = MQTT_STATE_INIT;

static HANDLE MQTT_semClientConnected;
static HANDLE MQTT_semClientsubscribed;

static uint8_t UART_au8RxBuffer[1024] = {0};
static MQTT_Buffer_t MQTT_sTxBuffer;

static uint32_t MQTT_u32MessageId = 0;

#define MQTT_MESSAGE_FORMAT "{\"client_id\":%s, \"message_id\":%d}"

/* ------------------------------------------------------------------------- */
/* ---------------------------- API Definitions ---------------------------- */
/* ------------------------------------------------------------------------- */
void MQTT_Task(void *pData)
{
    memset(&MQTT_sTxBuffer, 0x00, sizeof(MQTT_sTxBuffer));
    MQTT_sTxBuffer.is_dirty = false;
    MQTT_sTxBuffer.lock = OS_CreateSemaphore(1);

    if (INFO_GetIMEI(DeviceIMEI))
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT] Device IMEI: %s", DeviceIMEI);
    }

    if (SIM_GetICCID(DeviceICCID))
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT] Device ICCID: %s", DeviceICCID);
    }

    if (SIM_GetIMSI(DeviceIMSI))
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT] Device IMSI: %s", DeviceIMSI);
    }

    while (!bNetworkRegisteration)
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT] Waiting for network registration.");
        OS_Sleep(5000);
    }
    Trace(MQTT_TRACE_INDEX, "[MQTT] Network registration complete.");

    while (!bNetworkActivated)
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT] Waiting for network activation.");
        OS_Sleep(5000);
    }
    Trace(MQTT_TRACE_INDEX, "[MQTT] Network activation complete.");

    while (1)
    {

        switch (MQTT_eCurrentState)
        {
        case MQTT_STATE_INIT:
            Trace(MQTT_TRACE_INDEX, "[MQTT FSM] Current state: %s", "MQTT_STATE_INIT");
            MQTT_Init_StateHandler(NULL);
            if (MQTT_eLastError == MQTT_ERR_NONE)
            {
                MQTT_eCurrentState = MQTT_STATE_DISCONNECTED;
            }
            else
            {
                MQTT_eCurrentState = MQTT_STATE_ERROR;
            }
            break;

        case MQTT_STATE_DISCONNECTED:
            Trace(MQTT_TRACE_INDEX, "[MQTT FSM] Current state: %s", "MQTT_STATE_DISCONNECTED");
            MQTT_semClientConnected = OS_CreateSemaphore(0);
            MQTT_Disconnected_StateHandler(NULL);
            OS_WaitForSemaphore(MQTT_semClientConnected, OS_WAIT_FOREVER);
            OS_DeleteSemaphore(MQTT_semClientConnected);
            if (MQTT_eLastError == MQTT_ERR_NONE)
            {
                MQTT_eCurrentState = MQTT_STATE_CONNECTED;
            }
            else
            {
                MQTT_eCurrentState = MQTT_STATE_ERROR;
            }
            break;

        case MQTT_STATE_CONNECTED:
            Trace(MQTT_TRACE_INDEX, "[MQTT FSM] Current state: %s", "MQTT_STATE_CONNECTED");
            MQTT_semClientsubscribed = OS_CreateSemaphore(0);
            MQTT_Connected_StateHandler(NULL);
            OS_WaitForSemaphore(MQTT_semClientsubscribed, OS_WAIT_FOREVER);
            OS_DeleteSemaphore(MQTT_semClientsubscribed);
            if (MQTT_eLastError == MQTT_ERR_NONE)
            {
                MQTT_eCurrentState = MQTT_STATE_IDLE;
            }
            else
            {
                MQTT_eCurrentState = MQTT_STATE_ERROR;
            }
            break;

        case MQTT_STATE_PUBLISH_DATA:
            Trace(MQTT_TRACE_INDEX, "[MQTT FSM] Current state: %s", "MQTT_STATE_PUBLISH_DATA");
            MQTT_Publish_StateHandler(NULL);
            MQTT_eCurrentState = MQTT_STATE_IDLE;
            break;

        case MQTT_STATE_IDLE:
            MQTT_Idle_StateHandler(NULL);
            break;

        case MQTT_STATE_ERROR:
            Trace(MQTT_TRACE_INDEX, "[MQTT FSM] Current state: %s", "MQTT_STATE_ERROR");
            MQTT_Error_StateHandler(NULL);
            break;

        default:
            break;
        }
        OS_Sleep(5);
    }
}

/* ------------------------------------------------------------------------- */

void UART_Task(void *pData)
{
    UART_Config_t Local_sUartConf = {0};
    Local_sUartConf.baudRate = UART_BAUD_RATE_115200;
    Local_sUartConf.dataBits = UART_DATA_BITS_8;
    Local_sUartConf.parity = UART_PARITY_NONE;
    Local_sUartConf.stopBits = UART_STOP_BITS_1;
    Local_sUartConf.errorCallback = NULL;
    Local_sUartConf.rxCallback = NULL;
    Local_sUartConf.useEvent = true;

    if (UART_Init(UART1, Local_sUartConf))
    {
        Trace(UART_TRACE_INDEX, "[UART TASK] UART1 configuration complete.");
    }
    else
    {
        Trace(UART_TRACE_INDEX, "[UART TASK] UART1 configuration failed.");
    }

    while (1)
    {
        OS_Sleep(60000);
    }
}

/* ------------------------------------------------------------------------- */
/* ---------------------------- State  Handlers ---------------------------- */
/* ------------------------------------------------------------------------- */

void MQTT_Init_StateHandler(void *Copy_pvArgs)
{
    MQTT_psClient = MQTT_ClientNew();
    if (MQTT_psClient)
    {
        MQTT_sConnectionInfo.broker_hostname = MQTT_SERVER_ADDRESS;
        MQTT_sConnectionInfo.client_id = DeviceIMEI;
        MQTT_sConnectionInfo.client_user = MQTT_USER_KEY;
        MQTT_sConnectionInfo.client_pass = MQTT_USER_PASSWORD;
        MQTT_sConnectionInfo.keep_alive = 120;
        MQTT_sConnectionInfo.clean_session = 1;
        MQTT_sConnectionInfo.use_ssl = false;

        MQTT_eLastError = MQTT_ERR_NONE;
    }
    else
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT INIT STATE HANDLER] Failed to inistantiate a new MQTT instacne.");
        MQTT_eLastError = MQTT_ERR_NO_CLIENT;
    }
}

/* ------------------------------------------------------------------------- */

void MQTT_Disconnected_StateHandler(void *Copy_pvArgs)
{
    MQTT_Error_t Local_eError = MQTT_ERROR_NONE;

    if (MQTT_IsConnected(MQTT_psClient))
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT DISCONNECTED STATE HANDLER] MQTT Client is connected. Disconnecting from the server.");
        if ((Local_eError = MQTT_Disconnect(MQTT_psClient)) == MQTT_ERROR_NONE)
        {
            Trace(MQTT_TRACE_INDEX, "[MQTT DISCONNECTED STATE HANDLER] MQTT Client disconnected successfully.");
        }
        else
        {
            Trace(MQTT_TRACE_INDEX, "[MQTT DISCONNECTED STATE HANDLER] MQTT Client failed to diceonnect. Error code [ %d ].", Local_eError);
        }
    }

    Local_eError = MQTT_Connect(MQTT_psClient, MQTT_SERVER_ADDRESS, MQTT_SERVER_PORT, MQTT_Connection_Callback, NULL, &MQTT_sConnectionInfo);
    if (Local_eError == MQTT_ERROR_NONE)
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT DISCONNECTED STATE HANDLER] MQTT connection to the server succedded.");
        MQTT_eLastError = MQTT_ERR_NONE;
    }
    else
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT DISCONNECTED STATE HANDLER] MQTT connection to the server failed. Error code [ %d ]", Local_eError);
        MQTT_eLastError = MQTT_ERR_CONNECTION_FAILED;
        MQTT_eErrorCause = Local_eError;
    }
}

/* ------------------------------------------------------------------------- */

void MQTT_Connected_StateHandler(void *Copy_pvArgs)
{
    MQTT_Error_t Local_eError = MQTT_ERROR_NONE;

    MQTT_SetInPubCallback(MQTT_psClient, MQTT_Received_Callback, MQTT_ReceivedData_Callback, NULL);
    Local_eError = MQTT_Subscribe(MQTT_psClient, MQTT_TOPIC, MQTT_TOPIC_QOS, MQTT_Subscribed_Callback, (void *)MQTT_TOPIC);
    if (Local_eError == MQTT_ERROR_NONE)
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT CONNECTED STATE HANDLER] MQTT Client subscribed to topic: %s", MQTT_TOPIC);
        MQTT_eLastError = MQTT_ERR_NONE;
        MQTT_StartPublishTimer(MQTT_CLIENT_PUBLISH_INTERVAL, MQTT_psClient);
    }
    else
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT CONNECTED STATE HANDLER] MQTT Client failed to subscribe to topic: %s", MQTT_TOPIC);
        MQTT_eLastError = MQTT_ERR_SUBSCRIPTION_FAILED;
        MQTT_eErrorCause = Local_eError;
    }
}

/* ------------------------------------------------------------------------- */

void MQTT_Publish_StateHandler(void *pvArgs)
{
    MQTT_Error_t Local_eError = MQTT_ERROR_NONE;
    OS_WaitForSemaphore(MQTT_sTxBuffer.lock, OS_WAIT_FOREVER);
    if(MQTT_sTxBuffer.is_dirty)
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT PUBLISH STATE HANDLER] publishing topic [ %s ] data : %s", MQTT_TOPIC, MQTT_sTxBuffer.buffer);
        Local_eError = MQTT_Publish(MQTT_psClient, MQTT_TOPIC, MQTT_sTxBuffer.buffer, MQTT_sTxBuffer.len, 1, MQTT_TOPIC_QOS, 0, MQTT_Published_Callback, NULL);
        if (Local_eError == MQTT_ERROR_NONE)
        {
            Trace(MQTT_TRACE_INDEX, "[MQTT PUBLISH STATE HANDLeR] MQTT Publish successful.");
            MQTT_eCurrentState = MQTT_STATE_IDLE;
            MQTT_sTxBuffer.is_dirty = false;
        }
        else
        {
            Trace(MQTT_TRACE_INDEX, "[MQTT PUBLISH STATE HANDLeR] MQTT Publish failed. Error code [ %d ].", Local_eError);
            MQTT_eErrorCause = Local_eError;
            MQTT_eLastError = MQTT_ERR_PUBLISH_FAILED;
            MQTT_eCurrentState = MQTT_STATE_ERROR;
        }
    }
    else
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT PUBLISH STATE HANDLER] MQTT Tx buffer is clean.");
    }
    OS_ReleaseSemaphore(MQTT_sTxBuffer.lock);
}

/* ------------------------------------------------------------------------- */

void MQTT_Idle_StateHandler(void *pvArgs)
{
    MQTT_Error_t Local_eError = MQTT_IsConnected(MQTT_psClient);
    if (!Local_eError)
    {
        MQTT_eLastError = MQTT_ERR_DISCONNECTED;
        MQTT_eErrorCause = Local_eError;
    }    
}

/* ------------------------------------------------------------------------- */

void MQTT_Error_StateHandler(void *pvArgs)
{
    switch (MQTT_eLastError)
    {

    case MQTT_ERR_NONE:
        MQTT_eCurrentState = MQTT_STATE_IDLE;
        break;

    case MQTT_ERR_CONNECTION_FAILED:
        Trace(MQTT_TRACE_INDEX, "[MQTT ERROR STATE HANDLER] Connection failed. Trying to reconnect.");
        MQTT_eCurrentState = MQTT_STATE_DISCONNECTED;
        break;

    case MQTT_ERR_SUBSCRIPTION_FAILED:
        Trace(MQTT_TRACE_INDEX, "[MQTT ERROR STATE HANDLER] Subscription failed. Trying to re-sibscribe.");
        MQTT_eCurrentState = MQTT_STATE_CONNECTED;
        break;

    case MQTT_ERR_PUBLISH_FAILED:
        Trace(MQTT_TRACE_INDEX, "[MQTT ERROR STATE HANDLER] Publish failed.");
        MQTT_eCurrentState = MQTT_STATE_IDLE;
        break;

    case MQTT_ERR_DISCONNECTED:
        Trace(MQTT_TRACE_INDEX, "[MQTT ERROR STATE HANDLER] MQTT Client disconnected. Trying to re-connect.");
        MQTT_eCurrentState = MQTT_STATE_DISCONNECTED;
        break;

    case MQTT_ERR_NO_CLIENT:
        Trace(MQTT_TRACE_INDEX, "[MQTT ERROR STATE HANDLER] No MQTT Client instance.");
        MQTT_eCurrentState = MQTT_STATE_INIT;
        break;
        

    default:
        MQTT_eCurrentState = MQTT_STATE_IDLE;
        break;
    }

    switch(MQTT_eErrorCause)
        {
        case MQTT_CONNECTION_REFUSED_IDENTIFIER:
            break;
            
        case MQTT_CONNECTION_REFUSED_SERVER:
            break;

        case MQTT_CONNECTION_REFUSED_USERNAME_PASS:
            break;

        case MQTT_CONNECTION_REFUSED_NOT_AUTHORIZED:
            break;

        case MQTT_CONNECTION_DISCONNECTED:
            Trace(MQTT_TRACE_INDEX, "[MQTT ERROR HANDLER] Connection Disconnected. trying to reconnect.");
            MQTT_eCurrentState = MQTT_STATE_DISCONNECTED;
            MQTT_eLastError = MQTT_ERR_NONE;
            MQTT_eErrorCause = MQTT_ERROR_NONE;
            break;

        case MQTT_CONNECTION_TIMEOUT:
            Trace(MQTT_TRACE_INDEX, "[MQTT ERROR HANDLER] Connection timed out. trying to reconnect.");
            MQTT_eCurrentState = MQTT_STATE_DISCONNECTED;
            MQTT_eLastError = MQTT_ERR_NONE;
            MQTT_eErrorCause = MQTT_ERROR_NONE;
            break;

        case MQTT_CONNECTION_DNS_FAIL:
            Trace(MQTT_TRACE_INDEX, "[MQTT ERROR HANDLER] DNS Failed. trying to reconnect.");
            MQTT_eCurrentState = MQTT_STATE_DISCONNECTED;
            MQTT_eLastError = MQTT_ERR_NONE;
            MQTT_eErrorCause = MQTT_ERROR_NONE;
            break;

        default:
            break;
        }

    MQTT_eLastError = MQTT_ERR_NONE;
}

/* ------------------------------------------------------------------------- */
/* -------------------------- Callback  Functions -------------------------- */
/* ------------------------------------------------------------------------- */

void MQTT_Connection_Callback(MQTT_Client_t *Copy_psMqttClient, void *Copy_pvArgs, MQTT_Connection_Status_t Copy_eConnStatus)
{
    if (Copy_eConnStatus == MQTT_CONNECTION_ACCEPTED)
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT CONNECTION CALLBACK] Connection Accepted: %d", Copy_eConnStatus);
        MQTT_eLastError = MQTT_ERROR_NONE;
    }
    else
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT CONNECTION CALLBACK] Connection failed. Status: %d", Copy_eConnStatus);
        MQTT_eLastError = MQTT_ERR_CONNECTION_FAILED;
        MQTT_eErrorCause = Copy_eConnStatus;
    }
    OS_ReleaseSemaphore(MQTT_semClientConnected);
}

/* ------------------------------------------------------------------------- */

void MQTT_Subscribed_Callback(void *Copy_pvArgs, MQTT_Error_t Copy_eError)
{
    if (Copy_eError == MQTT_ERROR_NONE)
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT SUBSRIBED EVENT] Client subscribed to topic: %s", (const char *)Copy_pvArgs);
    }
    else
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT SUBSRIBED EVENT] Client subcription failed. Error code: %d", Copy_eError);
        MQTT_eLastError = MQTT_ERR_SUBSCRIPTION_FAILED;
        MQTT_eErrorCause = Copy_eError;
    }
    OS_ReleaseSemaphore(MQTT_semClientsubscribed);
}

/* ------------------------------------------------------------------------- */

void MQTT_Published_Callback(void *Copy_pvArgs, MQTT_Error_t Copy_eError)
{
    if (Copy_eError == MQTT_ERROR_NONE)
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT PUBLISHED EVENt] Client publish is successful");
    }
    else
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT PUBLISHED EVENt] Client publish is failed. Error code: %d", Copy_eError);
        MQTT_eErrorCause = Copy_eError;
        MQTT_eLastError = MQTT_ERR_PUBLISH_FAILED;
    }
}

/* ------------------------------------------------------------------------- */

void MQTT_Received_Callback(void *Copy_pvArgs, const char *Copy_psTopic, uint32_t Copy_u32Len)
{
    Trace(MQTT_TRACE_INDEX, "[MQTT RECEIVED EVENT] MQTT cleint received [ %d ] data bytes for topic [ %s ] ", Copy_u32Len, Copy_psTopic);
}

/* ------------------------------------------------------------------------- */

void MQTT_ReceivedData_Callback(void *Copy_pvArgs, const uint8_t *Copy_pu8Data, uint16_t Copy_u16Len, MQTT_Flags_t Copy_eFlags)
{
    memset(MQTT_au8Rxbuffer, 0x00, sizeof(MQTT_au8Rxbuffer));
    memcpy(MQTT_au8Rxbuffer, Copy_pu8Data, MIN(1023, Copy_u16Len));
    Trace(MQTT_TRACE_INDEX, "[MQTT RECEIVED DATA EVENT] MQTT cleint received [ %d ] data bytes : %s", Copy_u16Len, MQTT_au8Rxbuffer);
    if (Copy_eFlags == MQTT_FLAG_DATA_LAST)
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT RECEIVED DATA EVENT] Data frame is last frame.");
    }
}

/* ------------------------------------------------------------------------- */
/* ---------------------------- Event  Handlers ---------------------------- */
/* ------------------------------------------------------------------------- */

void UART_RX_EventHandler(API_Event_t *pEvent)
{
    MQTT_Error_t Local_eMqttError = MQTT_ERROR_NONE;

    switch (pEvent->param1)
    {
    case UART1:
    {
        uint8_t *Local_pcMessageStart = NULL;
        memset(UART_au8RxBuffer, 0x00, sizeof(UART_au8RxBuffer));
        memcpy(UART_au8RxBuffer, pEvent->pParam1, MIN(pEvent->param2, sizeof(UART_au8RxBuffer) - 1));
        Trace(UART_TRACE_INDEX, "[UART RX EVENT] UART1 received [ %d ] bytes [ %s ].", pEvent->param2, UART_au8RxBuffer);
        Local_pcMessageStart = strstr(UART_au8RxBuffer, UART_CMD_PREFIX);
        if (Local_pcMessageStart)
        {
            Local_pcMessageStart += strlen(UART_CMD_PREFIX);
            OS_WaitForSemaphore(MQTT_sTxBuffer.lock, OS_WAIT_FOREVER);
            memset(MQTT_sTxBuffer.buffer, 0x00, sizeof(MQTT_sTxBuffer.buffer));
            memcpy(MQTT_sTxBuffer.buffer, Local_pcMessageStart, MIN(strlen(Local_pcMessageStart), sizeof(MQTT_sTxBuffer.buffer) - 1));
            MQTT_sTxBuffer.is_dirty = true;
            MQTT_sTxBuffer.len = strlen(Local_pcMessageStart);
            OS_ReleaseSemaphore(MQTT_sTxBuffer.lock);
            Trace(UART_TRACE_INDEX, "[UART RX EVENT] MQTT message updated to [ %s ].", MQTT_sTxBuffer.buffer);
            if (MQTT_psClient)
            {
                if ((Local_eMqttError = MQTT_IsConnected(MQTT_psClient)))
                {
                    MQTT_eCurrentState = MQTT_STATE_PUBLISH_DATA;
                    Trace(MQTT_TRACE_INDEX, "[MQTT UART RX EVENT] Set MQTT state to publish data.");
                }
                else
                {
                    MQTT_eLastError = MQTT_ERR_DISCONNECTED;
                    MQTT_eErrorCause = Local_eMqttError;
                    MQTT_eCurrentState = MQTT_STATE_ERROR;
                    Trace(MQTT_TRACE_INDEX, "[MQTT UART RX EVENT] MQTT Error. Client is disconnected.");
                }
            }
            else
            {
                Trace(MQTT_TRACE_INDEX, "[MQTT UART RX EVENT] MQTT Client not yet initialized.");
            }
        }
        else
        {
            Trace(UART_TRACE_INDEX, "[UART RX EVENT] Invalid command prefix.");
        }
    }
    break;

    default:
        break;
    }
}

/* ------------------------------------------------------------------------- */

void MQTT_NetworkRegistered_EventHandler(API_Event_t *pEvent)
{
    Trace(MQTT_TRACE_INDEX, "[MQTT NETWORK REGISTERED EVENT] Network registration complete.");
    Network_StartAttach();
}

/* ------------------------------------------------------------------------- */

void MQTT_NetworkAttached_EventHandler(API_Event_t *pEvent)
{
    Trace(MQTT_TRACE_INDEX, "[MQTT NETWORK ATTACHED EVENT] Network Attach complete.");
    Network_StartActive(MQTT_sNetworkContext);
}

void MQTT_NetworkAttachFailed_EventHandler(API_Event_t *pEvent)
{
    Trace(MQTT_TRACE_INDEX, "[MQTT NETWORK ATTACHED FAILED EVENT] Network Attach Failed.");
    Network_StartAttach();
}

/* ------------------------------------------------------------------------- */

void MQTT_NetworkDeAttached_EventHandler(API_Event_t *pEvent)
{
    Trace(MQTT_TRACE_INDEX, "[MQTT NETWORK DE-ATTACHED EVENT] Network De-Attached.");
    Network_StartAttach();
}

/* ------------------------------------------------------------------------- */

void MQTT_NetworkActivated_EventHandler(API_Event_t *pEvent)
{
    Trace(MQTT_TRACE_INDEX, "[MQTT NETWORK ACTIVATED EVENT] Network Activated.");
    bNetworkActivated = true;
}

/* ------------------------------------------------------------------------- */

void MQTT_NetworkActivationFailed_EventHandler(API_Event_t *pEvent)
{
    Trace(MQTT_TRACE_INDEX, "[MQTT NETWORK ACTIVATE FAILED EVENT] Network Attach complete.");
    Network_StartActive(MQTT_sNetworkContext);
}

/* ------------------------------------------------------------------------- */

void MQTT_NetworkDeActivated_EventHandler(API_Event_t *pEvent)
{
    Trace(MQTT_TRACE_INDEX, "[MQTT NETWORK DE-ACTIVATED EVENT] Network Attach complete.");
    Network_StartActive(MQTT_sNetworkContext);
}

/* ------------------------------------------------------------------------- */

void MQTT_PublishTimerCallback(void * pvArgs)
{
    MQTT_StartPublishTimer(MQTT_CLIENT_PUBLISH_INTERVAL, MQTT_psClient);
    if(MQTT_psClient)
    {
        OS_WaitForSemaphore(MQTT_sTxBuffer.lock, OS_WAIT_FOREVER);
        MQTT_sTxBuffer.len = snprintf(MQTT_sTxBuffer.buffer, sizeof(MQTT_sTxBuffer.buffer) - 1, MQTT_MESSAGE_FORMAT, DeviceIMEI, ++MQTT_u32MessageId);
        MQTT_sTxBuffer.is_dirty = true;
        OS_ReleaseSemaphore(MQTT_sTxBuffer.lock);
        MQTT_eCurrentState = MQTT_STATE_PUBLISH_DATA;
        Trace(MQTT_TRACE_INDEX, "[MQTT PUBLISH TIMER CALLBACK] Updated publish message.");
    }
    else
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT PUBLISH TIMER CALLBACK] Can't publish. ");
        MQTT_eLastError = MQTT_ERR_NO_CLIENT;
    }
}

/* ------------------------------------------------------------------------- */

void MQTT_StartPublishTimer(uint32_t u32WaitTime, MQTT_Client_t * psClient)
{
    HANDLE Local_hMainTaskHandle = OS_GetUserMainHandle();
    if(Local_hMainTaskHandle)
    {
        OS_StartCallbackTimer(Local_hMainTaskHandle, u32WaitTime, MQTT_PublishTimerCallback, psClient);
        Trace(MQTT_TRACE_INDEX, "[MQTT START PUBLISH TIMER] Started publish timer. Will publish after [ %d ] seconds.", u32WaitTime/1000);
    }
    else
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT START PUBLISH TIMER] Failed to start publish timer. Coundln't get main task handle.");
    }
    
}

/* ------------------------------------------------------------------------- */
/* ------------------------------ End Of File ------------------------------ */
/* ------------------------------------------------------------------------- */
