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

#include "../include/mqtt_task.h"


/* ------------------------------------------------------------------------- */
/* --------------------- Global Variables & Definitions -------------------- */
/* ------------------------------------------------------------------------- */

extern bool bNetworkRegisteration;

HANDLE mqttTaskHandle;

static const Network_PDP_Context_t MQTT_sNetworkContext = {
    .apn = MQTT_NETWORK_AP_NAME,
    .userName = MQTT_NETWORK_USERNAME,
    .userPasswd = MQTT_NETWORK_PASSWORD,
};

static bool bNetworkActivated = false;

static uint8_t DeviceIMEI [20] = {0};

static MQTT_Connect_Info_t MQTT_sConnectionInfo = {0};
static MQTT_Client_t * MQTT_psClient = NULL;

static MQTT_Err_t MQTT_eLastError = MQTT_ERR_NONE; 
static MQTT_Error_t MQTT_eErrorCause = MQTT_ERROR_NONE;
static MQTT_State_t MQTT_eCurrentState = MQTT_STATE_INIT;
static time_t MQTT_sNextPublishTime = {0};

static volatile bool MQTT_bOperationDone = false;
static uint32_t MQTT_MessageCount = 0;

/* ------------------------------------------------------------------------- */
/* ---------------------------- API Definitions ---------------------------- */
/* ------------------------------------------------------------------------- */
void MQTT_Task(void * pData)
{

    if(INFO_GetIMEI(DeviceIMEI))
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT] Device IMEI: %s", DeviceIMEI);
    }

    while(!bNetworkRegisteration)
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT] Waiting for network registration.");
        OS_Sleep(5000);
    }
    Trace(MQTT_TRACE_INDEX, "[MQTT] Network registration complete.");

    while(!bNetworkActivated)
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT] Waiting for network activation.");
        OS_Sleep(5000);
    }
    Trace(MQTT_TRACE_INDEX, "[MQTT] Network activation complete.");

    while(1)
    {
        switch(MQTT_eCurrentState)
        {
        case MQTT_STATE_INIT:
            Trace(MQTT_TRACE_INDEX, "[MQTT FSM] Current state: %s", "MQTT_STATE_INIT");
            MQTT_Init_StateHandler(NULL);
            MQTT_eCurrentState = MQTT_STATE_DISCONNECTED;
            break;

        case MQTT_STATE_DISCONNECTED:
            Trace(MQTT_TRACE_INDEX, "[MQTT FSM] Current state: %s", "MQTT_STATE_DISCONNECTED");
            MQTT_bOperationDone = false;
            MQTT_Disconnected_StateHandler(NULL);
            while(!MQTT_bOperationDone)
                OS_Sleep(1000);
                ;
            if(MQTT_eLastError == MQTT_ERR_NONE)
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
            MQTT_bOperationDone = false;
            MQTT_Connected_StateHandler(NULL);
            while(!MQTT_bOperationDone)
                ;
            if(MQTT_eLastError == MQTT_ERR_NONE)
            {
                MQTT_eCurrentState = MQTT_STATE_PUBLISH_DATA;
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
            Trace(MQTT_TRACE_INDEX, "[MQTT FSM] Current state: %s", "MQTT_STATE_IDLE");
            MQTT_Idle_StateHandler(NULL);
            break;

        case MQTT_STATE_ERROR   :
            Trace(MQTT_TRACE_INDEX, "[MQTT FSM] Current state: %s", "MQTT_STATE_ERROR");
            MQTT_Error_StateHandler(NULL);
            break;

        default:
            break;
        }
    }
}

/* ------------------------------------------------------------------------- */
/* ---------------------------- State  Handlers ---------------------------- */
/* ------------------------------------------------------------------------- */

void MQTT_Init_StateHandler(void * Copy_pvArgs)
{
    time(&MQTT_sNextPublishTime);
    MQTT_sNextPublishTime += MQTT_CLIENT_PUBLISH_INTERVAL;

    MQTT_psClient = MQTT_ClientNew();
    MQTT_sConnectionInfo.broker_hostname = MQTT_SERVER_ADDRESS;
    MQTT_sConnectionInfo.client_id = DeviceIMEI;
    MQTT_sConnectionInfo.client_user = MQTT_USER_KEY;
    MQTT_sConnectionInfo.client_pass = MQTT_USER_PASSWORD;
    MQTT_sConnectionInfo.keep_alive = 120;
    MQTT_sConnectionInfo.clean_session = 1;
    MQTT_sConnectionInfo.use_ssl = false;
}

/* ------------------------------------------------------------------------- */

void MQTT_Disconnected_StateHandler(void * Copy_pvArgs)
{
    MQTT_Error_t Local_eError = MQTT_ERROR_NONE;

    Local_eError = MQTT_Connect(MQTT_psClient, MQTT_SERVER_ADDRESS, MQTT_SERVER_PORT, MQTT_Connection_Callback, NULL, &MQTT_sConnectionInfo);
    if(Local_eError == MQTT_ERROR_NONE)
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
    MQTT_bOperationDone = true;
}

/* ------------------------------------------------------------------------- */

void MQTT_Connected_StateHandler(void * Copy_pvArgs)
{
    MQTT_Error_t Local_eError = MQTT_ERROR_NONE;

    MQTT_SetInPubCallback(MQTT_psClient, MQTT_Received_Callback, MQTT_ReceivedData_Callback, NULL);
    Local_eError = MQTT_Subscribe(MQTT_psClient, MQTT_TOPIC, MQTT_TOPIC_QOS, MQTT_Subscribed_Callback, (void *)MQTT_TOPIC);
    if(Local_eError == MQTT_ERROR_NONE)
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT CONNECTED STATE HANDLER] MQTT Client subscribed to topic: %s", MQTT_TOPIC);
        MQTT_eLastError = MQTT_ERR_NONE;
    }
    else
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT CONNECTED STATE HANDLER] MQTT Client failed to subscribe to topic: %s", MQTT_TOPIC);
        MQTT_eLastError = MQTT_ERR_SUBSCRIPTION_FAILED;
        MQTT_eErrorCause = Local_eError;
    }
    MQTT_bOperationDone = true;
}

/* ------------------------------------------------------------------------- */

void MQTT_Publish_StateHandler(void * pvArgs)
{
    MQTT_Error_t Local_eError = MQTT_ERROR_NONE;
    uint8_t Local_au8Message [50] = {0};
    uint32_t Local_u32MessageLen = snprintf(Local_au8Message, 50, MQTT_PUBLISH_PAYLOAD, DeviceIMEI, ++MQTT_MessageCount);

    Trace(MQTT_TRACE_INDEX, "[MQTT PUBLISH STATE HANDLER] publishing topic [%s] data : %s", MQTT_TOPIC, Local_au8Message);
    Local_eError = MQTT_Publish(MQTT_psClient, MQTT_TOPIC, Local_au8Message, Local_u32MessageLen, 1, MQTT_TOPIC_QOS, 0, MQTT_Published_Callback, NULL);
    if(Local_eError != MQTT_ERROR_NONE)
    {
        MQTT_eErrorCause = Local_eError;
        MQTT_eLastError = MQTT_ERR_PUBLISH_FAILED;
    }
    MQTT_eCurrentState = MQTT_STATE_IDLE;
}

/* ------------------------------------------------------------------------- */

void MQTT_Idle_StateHandler(void * pvArgs)
{
    time_t Local_sCurrentTime = {0};

    if(MQTT_eLastError == MQTT_ERR_NONE)
    {
        time(&Local_sCurrentTime);
        if(Local_sCurrentTime > MQTT_sNextPublishTime)
        {
            MQTT_sNextPublishTime = Local_sCurrentTime + MQTT_CLIENT_PUBLISH_INTERVAL;
            MQTT_eCurrentState = MQTT_STATE_PUBLISH_DATA;
        }
        OS_Sleep(1000);
    }
    else
    {
        MQTT_eCurrentState = MQTT_STATE_ERROR;
    }
}

/* ------------------------------------------------------------------------- */

void MQTT_Error_StateHandler(void * pvArgs)
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
        Trace(MQTT_TRACE_INDEX, "[MQTT ERROR STATE HANDLER] Publish failed. Trying to re-publish after %d seconds.", MQTT_CLIENT_PUBLISH_INTERVAL);
        MQTT_eCurrentState = MQTT_STATE_IDLE;
        break;

    default:
        MQTT_eCurrentState = MQTT_STATE_IDLE;
        break;
    }

    MQTT_eLastError = MQTT_ERR_NONE;
}

/* ------------------------------------------------------------------------- */
/* -------------------------- Callback  Functions -------------------------- */
/* ------------------------------------------------------------------------- */

void MQTT_Connection_Callback(MQTT_Client_t * Copy_psMqttClient, void * Copy_pvArgs, MQTT_Connection_Status_t Copy_eConnStatus)
{
    if(Copy_eConnStatus == MQTT_CONNECTION_ACCEPTED)
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT CONNECTION CALLBACK] Connection Accepted: %d", Copy_eConnStatus);
        MQTT_eLastError = MQTT_ERROR_NONE;
    }
    else
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT CONNECTION CALLBACK] Connection failed. Status: %d", Copy_eConnStatus);
        /*
        MQTT_eLastError = MQTT_ERR_CONNECTION_FAILED;
        MQTT_eErrorCause = Copy_eConnStatus;
        */
    }
    MQTT_bOperationDone = true;
}

/* ------------------------------------------------------------------------- */

void MQTT_Subscribed_Callback(void * Copy_pvArgs, MQTT_Error_t Copy_eError)
{
    if(Copy_eError == MQTT_ERROR_NONE)
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT SUBSRIBED EVENT] Client subscribed to topic: %s", (const char *)Copy_pvArgs);
    }
    else
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT SUBSRIBED EVENT] Client subcription failed. Error code: %d", Copy_eError);
        /*
        MQTT_eLastError = MQTT_ERR_SUBSCRIPTION_FAILED;
        MQTT_eErrorCause = Copy_eError;
        */
    }
}

/* ------------------------------------------------------------------------- */

void MQTT_Published_Callback(void * Copy_pvArgs, MQTT_Error_t Copy_eError)
{
    if(Copy_eError == MQTT_ERROR_NONE)
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT PUBLISHED EVENt] Client publish is successful");
    }
    else
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT PUBLISHED EVENt] Client publish is failed. Error code: %d", Copy_eError);
        /*
        MQTT_eErrorCause = Copy_eError;
        MQTT_eLastError = MQTT_ERR_PUBLISH_FAILED;
        */
    }
}

/* ------------------------------------------------------------------------- */

void MQTT_Received_Callback(void * Copy_pvArgs, const char * Copy_psTopic, uint32_t Copy_u32Len)
{
    Trace(MQTT_TRACE_INDEX, "[MQTT RECEIVED EVENT] MQTT cleint received [ %d ] data bytes for topic [ %s ] ", Copy_u32Len, Copy_psTopic);
}

/* ------------------------------------------------------------------------- */

void MQTT_ReceivedData_Callback(void * Copy_pvArgs, const uint8_t * Copy_pu8Data, uint16_t Copy_u16Len, MQTT_Flags_t Copy_eFlags)
{
    Trace(MQTT_TRACE_INDEX, "[MQTT RECEIVED DATA EVENT] MQTT cleint received [ %d ] data bytes : %s", Copy_u16Len, Copy_pu8Data);
    if(Copy_eFlags == MQTT_FLAG_DATA_LAST)
    {
        Trace(MQTT_TRACE_INDEX, "[MQTT RECEIVED DATA EVENT] Data frame is last frame.");
    }
}

/* ------------------------------------------------------------------------- */
/* ---------------------------- Event  Handlers ---------------------------- */
/* ------------------------------------------------------------------------- */

void MQTT_NetworkRegistered_EventHandler(API_Event_t * pEvent)
{
    Trace(MQTT_TRACE_INDEX, "[MQTT NETWORK REGISTERED EVENT] Network registration complete.");
    Network_StartAttach();
}

void MQTT_NetworkAttached_EventHandler(API_Event_t * pEvent)
{
    Trace(MQTT_TRACE_INDEX, "[MQTT NETWORK ATTACHED EVENT] Network Attach complete.");
    Network_StartActive(MQTT_sNetworkContext);
}

void MQTT_NetworkAttachFailed_EventHandler(API_Event_t * pEvent)
{
    Trace(MQTT_TRACE_INDEX, "[MQTT NETWORK ATTACHED FAILED EVENT] Network Attach Failed.");
}

void MQTT_NetworkDeAttached_EventHandler(API_Event_t * pEvent)
{
    Trace(MQTT_TRACE_INDEX, "[MQTT NETWORK DE-ATTACHED EVENT] Network De-Attached.");

}

void MQTT_NetworkActivated_EventHandler(API_Event_t * pEvent)
{
    Trace(MQTT_TRACE_INDEX, "[MQTT NETWORK ACTIVATED EVENT] Network Activated.");
    bNetworkActivated = true;
}

void MQTT_NetworkActivationFailed_EventHandler(API_Event_t * pEvent)
{
    Trace(MQTT_TRACE_INDEX, "[MQTT NETWORK ACTIVATE FAILED EVENT] Network Attach complete.");
}

void MQTT_NetworkDeActivated_EventHandler(API_Event_t * pEvent)
{
    Trace(MQTT_TRACE_INDEX, "[MQTT NETWORK DE-ACTIVATED EVENT] Network Attach complete.");
}

/* ------------------------------------------------------------------------- */
/* ------------------------------ End Of File ------------------------------ */
/* ------------------------------------------------------------------------- */
