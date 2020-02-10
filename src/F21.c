#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "api_os.h"
#include "api_event.h"
#include "api_debug.h"

#include "api_hal_pm.h"

#include "../include/F21.h"

#if defined(ENABLE_GPIO_TASK)
#include "api_hal_gpio.h"
#endif /*   ENABLE_GPIO_TASK   */

#if defined(ENABLE_UART_TASK)
#include "api_hal_uart.h"
#endif /*  ENABLE_UART_TASK    */

#if defined(ENABLE_GPS_TASK)

#include "api_gps.h"
#include "api_hal_uart.h"
#include "gps.h"
#include "buffer.h"
#include "gps_parse.h"

#endif /*  ENABLE_GPS_TASK */

/* ------------------------------------------------------------------------- */
/* --------------------- Global Variables & Definitions -------------------- */
/* ------------------------------------------------------------------------- */

/* ---------------------------- Main task handle --------------------------- */

static HANDLE mainTaskHandle;


/* ---------------------------- GPIO task handle --------------------------- */

#if defined(ENABLE_GPIO_TASK)

static HANDLE gpioTaskHandle;

#define GPIO_TRACE_INDEX 1

#endif /*  ENABLE_GPIO_TASK    */


/* ---------------------------- UART task handle --------------------------- */

#if defined(ENABLE_UART_TASK)

static HANDLE uartTaskHandle;

#define UART_TRACE_INDEX 2

#endif /*  ENABLE_UART_TASK    */


/* ---------------------------- GPS task handle --------------------------- */

#if defined(ENABLE_GPS_TASK)

static HANDLE gpsTaskHandle;

#define GPS_TRACE_INDEX 3

bool GpsIsRegistered = false;
bool GpsIsOpen = true;

#endif /*   ENABLE_GPS_TASK     */


/* ---------------------------- Call task handle --------------------------- */

#if defined(ENABLE_CALL_RECEIVE_TASK) || defined(ENABLE_DIAL_TASK)

static HANDLE callTaskHandle;

#define CALL_TRACE_INDEX 4

#endif /*  ENABLE_CALL_RECEIVE_TASK || ENABLE_DIAL_TASK    */

/* ---------------------------- SMS task handle ---------------------------- */
#if defined(ENABLE_SMS_RECEIVE_TASK) || defined(ENABLE_SMS_SEND_TASK)

static HANDLE smsTaskHandle;

#define SMS_TRACE_INDEX 5

#endif /*  ENABLE_SMS_RECEIVE_TASK || ENABLE_SMS_SEND_TASK */


/* ---------------------------- GPRS task handle --------------------------- */

#if defined(ENABLE_GPRS_TASK)

static HANDLE gprsTaskHandle;

#define GPRS_TRACE_INDEX 6

#endif /*  ENABLE_GPRS_TASK    */

/* ---------------------------- MQTT task handle --------------------------- */
#if defined(ENABLE_MQTT_TASK)

static HANDLE mqttTaskHandle;

#define MQTT_TRACE_INDEX 7

#endif /*  ENABLE_MQTT_TASK    */

/* ----------------------------- Trace Indices ----------------------------- */

#define BATTERY_INFO_TRACE_INDEX    15
#define SYS_INFO_TRACE_INDEX        16


/* ------------------------------------------------------------------------- */
/* ---------------------------- API Definitions ---------------------------- */
/* ------------------------------------------------------------------------- */

void F21_Main(void *pData)
{
    mainTaskHandle = OS_CreateTask(F21MainTask, NULL, NULL, MAIN_TASK_STACK_SIZE, MAIN_TASK_PRIORITY, 0, 0, MAIN_TASK_NAME);
    OS_SetUserMainHandle(&mainTaskHandle);
}

/* ------------------------------------------------------------------------- */

void F21MainTask(void *pData)
{
    API_Event_t *Local_sEvent = NULL;

#if defined(ENABLE_GPIO_TASK)
    gpioTaskHandle = OS_CreateTask(GPIO_Task, NULL, NULL, GPIO_TASK_STACK_SIZE, GPIO_TASK_PRIORITY, 0, 0, GPIO_TASK_NAME);
#endif /*   ENABLE_GPIO_TASK  */

#if defined(ENABLE_UART_TASK)
    uartTaskHandle = OS_CreateTask(UART_Task, NULL, NULL, UART_TASK_STACK_SIZE, UART_TASK_PRIORITY, 0, 0, UART_TASK_NAME);
#endif /*   ENABLE_UART_TASK    */

#if defined(ENABLE_GPS_TASK)
    gpsTaskHandle = OS_CreateTask(GPS_Task, NULL, NULL, GPS_TASK_STACK_SIZE, GPS_TASK_PRIORITY, 0, 0, GPS_TASK_NAME);
#endif /*   ENABLE_UART_TASK    */

    while (1)
    {
        if (OS_WaitEvent(mainTaskHandle, (void **)&Local_sEvent, OS_TIME_OUT_WAIT_FOREVER))
        {
            EventDispatch(Local_sEvent);
            OS_Free(Local_sEvent->pParam1);
            OS_Free(Local_sEvent->pParam2);
            OS_Free(Local_sEvent);
        }
    }
}

/* ------------------------------------------------------------------------- */

void EventDispatch(API_Event_t *pEvent)
{
    switch (pEvent->id)
    {
    case API_EVENT_ID_POWER_ON:
        Trace(SYS_INFO_TRACE_INDEX, "[POWER ON EVENT] Pudding powered on...");
        break;

    case API_EVENT_ID_SYSTEM_READY:
        Trace(SYS_INFO_TRACE_INDEX, "[SYSTEM READY EVENT] Pudding system is ready...");
        break;

    case API_EVENT_ID_NO_SIMCARD:
        Trace(SYS_INFO_TRACE_INDEX, "[NO SIM CARD EVENT] No sim card detected!");
        break;

    case API_EVENT_ID_SIMCARD_DROP:
        Trace(SYS_INFO_TRACE_INDEX, "[SIM CARD DROP EVENT] Sim card drop!");
        break;

    case API_EVENT_ID_SIGNAL_QUALITY:
        Trace(SYS_INFO_TRACE_INDEX, "[SIGNAL QUALITY EVENT] Signal Quality: %d", pEvent->param1);
        Trace(SYS_INFO_TRACE_INDEX, "[SIGNAL QUALITY EVENT] RSSI: %d", pEvent->param1 * 2 - 113);
        Trace(SYS_INFO_TRACE_INDEX, "[SIGNAL QUALITY EVENT] RX Qual: %d", pEvent->param2);
        break;
    
    case API_EVENT_ID_NETWORK_GOT_TIME:
        {
            RTC_Time_t * Local_psCurrentTime = (RTC_Time_t *) pEvent->pParam1;
            Trace(SYS_INFO_TRACE_INDEX, "[NETWORK TIME EVENT] Netowrk Date: %02d/%02d/%04d Time : %02d:%02d:%02d", Local_psCurrentTime->day, Local_psCurrentTime->month, Local_psCurrentTime->year, Local_psCurrentTime->hour, Local_psCurrentTime->minute, Local_psCurrentTime->second );
        }
        break;
    
    case API_EVENT_ID_NETWORK_CELL_INFO:
        {
            Trace(SYS_INFO_TRACE_INDEX, "[NETWORK TIME EVENT] Current cell number: %d", pEvent->param1 );
        }
        break;


#if defined(ENABLE_GPS_TASK)

    case API_EVENT_ID_NETWORK_REGISTERED_HOME:
    case API_EVENT_ID_NETWORK_REGISTERED_ROAMING:
        Trace(GPS_TRACE_INDEX, "[Network Registration Event] Network registration complete...");
        GpsIsRegistered = true;
        break;
        
#endif  /*  ENABLE_GPS_TASK     */

    case API_EVENT_ID_NETWORK_REGISTER_SEARCHING:
        break;

    case API_EVENT_ID_NETWORK_REGISTER_DENIED:
        break;

    case API_EVENT_ID_NETWORK_REGISTER_NO:
        break;

    case API_EVENT_ID_NETWORK_DEREGISTER:
        break;

    case API_EVENT_ID_NETWORK_DETACHED:
        break;

    case API_EVENT_ID_NETWORK_ATTACH_FAILED:
        break;

    case API_EVENT_ID_NETWORK_ATTACHED:
        break;

    case API_EVENT_ID_NETWORK_DEACTIVED:
        break;

    case API_EVENT_ID_NETWORK_ACTIVATE_FAILED:
        break;

    case API_EVENT_ID_NETWORK_ACTIVATED:
        break;

#if defined(ENABLE_UART_TASK) && defined(ENABLE_UART_EVENTS)

    case API_EVENT_ID_UART_RECEIVED:
        switch (pEvent->param1)
        {
        case UART1:
        {
            CHAR buffer[100] = {0};
            memcpy(buffer, pEvent->pParam1, pEvent->param2);
            UART_Write(UART1, buffer, pEvent->param2);
            Trace(UART_TRACE_INDEX, "[UART RX EVENT] Received [ %d ] bytes on UART 1 : %s", pEvent->param2, buffer);
        }
        break;

        case UART2:
        {
            CHAR buffer[100] = {0};
            memcpy(buffer, pEvent->pParam1, pEvent->param2);
            UART_Write(UART2, buffer, pEvent->param2);
            Trace(UART_TRACE_INDEX, "[UART RX EVENT] Received [ %d ] bytes on UART 2 : %s", pEvent->param2, buffer);
        }
        break;

        case API_EVENT_ID_GPS_UART_RECEIVED:
            break;

        default:
            Trace(UART_TRACE_INDEX, "[UART RX EVENT] UART number Error!!");
            break;
        }

#endif /*  ENABLE_UART_TASK || ENABLE_UART_EVENTS  */

#if defined(ENABLE_GPS_TASK)


    case API_EVENT_ID_GPS_UART_RECEIVED:
        if(pEvent->param1)
        {
            GPS_Update(pEvent->pParam1, pEvent->param1);
        }
        break;


#endif   /*  ENABLE_GPS_TASK    */

    case API_EVENT_ID_POWER_INFO:
        Trace(BATTERY_INFO_TRACE_INDEX, "[POWER INFO EVENT] Battery charger state: %d", ((pEvent->param1 >> 16) & 0xFFFFu));
        Trace(BATTERY_INFO_TRACE_INDEX, "[POWER INFO EVENT] Battery charge level: %d %%", (pEvent->param1 & 0xFFFFu));
        Trace(BATTERY_INFO_TRACE_INDEX, "[POWER INFO EVENT] Battery state: %d", ((pEvent->param2 >> 16) & 0xFFFFu));
        Trace(BATTERY_INFO_TRACE_INDEX, "[POWER INFO EVENT] Battery voltage: %d mv", (pEvent->param2 & 0xFFFFu));
        break;

    default:
        Trace(SYS_INFO_TRACE_INDEX, "handling event id [ %d ]", pEvent->id);
        break;
    }
}

/* ------------------------------------------------------------------------- */

#if defined(ENABLE_GPIO_TASK)

void GPIO_Task(void *pData)
{

#if defined(ENABLE_INDICATOR_LED)

    GPIO_LEVEL indicatorLedLevel = GPIO_LEVEL_LOW;
    GPIO_config_t indicatorLed = {0};
    indicatorLed.pin = INDICATOR_LED_PIN;
    indicatorLed.mode = GPIO_MODE_OUTPUT;
    indicatorLed.defaultLevel = GPIO_LEVEL_LOW;

#endif /*  ENABLE_INDICATOR_LED    */

#if defined(ENABLE_STATUS_UPDATE)

    GPIO_LEVEL statusLedLevel = GPIO_LEVEL_LOW;
    GPIO_config_t statusLed = {0};
    GPIO_LEVEL statusPinLevelUpdate = GPIO_LEVEL_LOW;
    GPIO_LEVEL statusPinLevel = GPIO_LEVEL_LOW;
    GPIO_config_t statusUpdate = {0};

    statusLed.pin = STATUS_LED_PIN;
    statusLed.mode = GPIO_MODE_OUTPUT;
    statusLed.defaultLevel = GPIO_LEVEL_HIGH;

    statusUpdate.pin = STATUS_UPDATE_PIN;
    statusUpdate.mode = GPIO_MODE_INPUT;
    statusUpdate.defaultLevel = GPIO_LEVEL_LOW;

    statusUpdate.intConfig.type = GPIO_INT_TYPE_MAX;
    statusUpdate.intConfig.debounce = 0;
    statusUpdate.intConfig.callback = NULL;

#endif /*  ENABLE_STATUS_UPDATE    */

#if defined(ENABLE_INDICATOR_LED) || defined(ENABLE_STATUS_UPDATE)

    PM_PowerEnable(POWER_TYPE_VPAD, true);
    Trace(GPIO_TRACE_INDEX, "Enabled power for gpio pins [25 : 36] and [ 0 : 7 ]");

#endif

#if defined(ENABLE_INDICATOR_LED)

    GPIO_Init(indicatorLed);
    Trace(GPIO_TRACE_INDEX, "Configured indicator led...");

#endif /*  ENABLE_INDICATOR_LED    */

#if defined(ENABLE_STATUS_UPDATE)

    GPIO_Init(statusLed);
    Trace(GPIO_TRACE_INDEX, "Configured status led...");

    GPIO_Init(statusUpdate);
    Trace(GPIO_TRACE_INDEX, "Configured status update pin...");

#endif /*  ENABLE_STATUS_UPDATE    */

    while (1)
    {

#if defined(ENABLE_INDICATOR_LED)

        indicatorLedLevel = (indicatorLedLevel == GPIO_LEVEL_HIGH) ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH;
        GPIO_SetLevel(indicatorLed, indicatorLedLevel);
        Trace(GPIO_TRACE_INDEX, "Toggling indicator led...");

#endif /*  ENABLE_INDICATOR_LED    */

#if defined(ENABLE_STATUS_UPDATE)

        GPIO_GetLevel(statusUpdate, &statusPinLevel);
        if (statusPinLevel != statusPinLevelUpdate)
        {
            statusPinLevelUpdate = statusPinLevel;
            statusLedLevel = (statusPinLevel == GPIO_LEVEL_HIGH) ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH;
            GPIO_SetLevel(statusLed, statusLedLevel);
            Trace(GPIO_TRACE_INDEX, "Updating status...");
        }

        if (statusPinLevel == GPIO_LEVEL_HIGH)
        {
            Trace(GPIO_TRACE_INDEX, "Current status : HIGH");
        }
        else
        {
            Trace(GPIO_TRACE_INDEX, "Current status : LOW");
        }

#endif /*  ENABLE_STATUS_UPDATE    */

        OS_Sleep(1000);
    }
}

#endif /*   ENABLE_GPIO_TASK  */

/* ------------------------------------------------------------------------- */

#if defined(ENABLE_UART_TASK)

void UART_Task(void *pData)
{

#if defined(ENABLE_UART_EVENTS)
    API_Event_t *Local_psUartEvent = NULL;
#endif /*  ENABLE_UART_EVENTS  */

    CHAR uartTxBuffer[100] = {0};
    CHAR uartRxBuffer[100] = {0};
    uint32_t uartTxCounter = 0;
    uint32_t uartReadBytes = 0;
    uint32_t uartSentBytes = 0;

    UART_Config_t uartConfig = {0};

    uartConfig.baudRate = UART_BAUD_RATE_115200;
    uartConfig.dataBits = UART_DATA_BITS_8;
    uartConfig.errorCallback = UART_ErrorCallback;
    uartConfig.parity = UART_PARITY_NONE;
    uartConfig.stopBits = UART_STOP_BITS_1;

#if defined(ENABLE_UART_EVENTS)
    uartConfig.rxCallback = UART_RxCallback;
    uartConfig.useEvent = true;
#endif /*  ENABLE_UART_EVENTS  */

#if !defined(ENABLE_UART_EVENTS)
    uartConfig.rxCallback = NULL;
    uartConfig.useEvent = false;
#endif /*  !ENABLE_UART_EVENTS  */

    if (UART_Init(UART1, uartConfig))
    {
        Trace(UART_TRACE_INDEX, "UART 1 configuration complete...");
    }
    else
    {
        Trace(UART_TRACE_INDEX, "Failed to configure UART 1");
    }

    if (UART_Init(UART2, uartConfig))
    {
        Trace(UART_TRACE_INDEX, "UART 2 configuration complete...");
    }
    else
    {
        Trace(UART_TRACE_INDEX, "Failed to configure UART 1");
    }

    while (1)
    {

#if defined(ENABLE_UART_EVENTS)

        if (OS_WaitEvent(uartTaskHandle, (void **)&Local_psUartEvent, OS_TIME_OUT_WAIT_FOREVER))
        {
            EventDispatch(Local_psUartEvent);
            OS_Free(Local_psUartEvent->pParam1);
            OS_Free(Local_psUartEvent->pParam2);
            OS_Free(Local_psUartEvent);
        }

#endif /*   ENABLE_UART_EVENTS  */

        OS_Sleep(1000);
    }
}

/* ------------------------------------------------------------------------- */

#if !defined(ENABLE_UART_EVENTS)

void UART_RxCallback(UART_Callback_Param_t param)
{
    switch (param.port)
    {
    case UART1:
        Trace(UART_TRACE_INDEX, "Received [%d] bytes on UART 1 : %s", param.length, param.buf);
        UART_Write(UART2, param.buf, param.length);
        break;

    case UART2:
        Trace(UART_TRACE_INDEX, "Received [%d] bytes on UART 2 : %s", param.length, param.buf);
        UART_Write(UART2, param.buf, param.length);
        break;

    default:
        break;
    }
}

#endif /*   ENABLE_UART_EVENTS */

/* ------------------------------------------------------------------------- */

void UART_ErrorCallback(UART_Error_t error)
{
    Trace(3, "UART Error callback");
    switch (error)
    {
    case UART_ERROR_RX_LINE_ERROR:
        Trace(UART_TRACE_INDEX, "UART RX Line error...");
        break;

    case UART_ERROR_RX_OVER_FLOW_ERROR:
        Trace(UART_TRACE_INDEX, "UART RX overflow error...");
        break;

    case UART_ERROR_RX_PARITY_ERROR:
        Trace(UART_TRACE_INDEX, "UART RX parity error...");
        break;

    case UART_ERROR_RX_BREAK_INT_ERROR:
        Trace(UART_TRACE_INDEX, "UART RX break interrupt error...");
        break;

    case UART_ERROR_RX_FRAMING_ERROR:
        Trace(UART_TRACE_INDEX, "UART RX framing error...");
        break;

    case UART_ERROR_TX_OVER_FLOW_ERROR:
        Trace(UART_TRACE_INDEX, "UART TX overflow error...");
        break;

    default:
        Trace(UART_TRACE_INDEX, "UART unknown error...");
        break;
    }
}

#endif /*   ENABLE_UART_TASK    */

/* ------------------------------------------------------------------------- */

#if defined(ENABLE_GPS_TASK)

void GPS_Task(void *pData)
{
    GPS_Info_t *Local_psGpsInfo = Gps_GetInfo();
    uint8_t Local_au8GpsBuffer[300] = {0};
    bool Local_bInitState = true;
    uint8_t Local_u8IsFixed = 0;
    CHAR * Local_pu8FixStr = NULL;
    double Local_dLatitude = 0;
    double Local_dLongitude = 0;
    int32_t Local_i32Temp = 0;

    UART_Config_t Local_sUartConfig = {0};
    Local_sUartConfig.baudRate = UART_BAUD_RATE_115200;
    Local_sUartConfig.dataBits = UART_DATA_BITS_8;
    Local_sUartConfig.parity = UART_PARITY_NONE;
    Local_sUartConfig.stopBits = UART_STOP_BITS_1;
    Local_sUartConfig.useEvent = false;
    Local_sUartConfig.errorCallback = NULL;
    Local_sUartConfig.rxCallback = NULL;

    while(GpsIsRegistered == false)
    {
        Trace(GPS_TRACE_INDEX, "Waiting for network registration..");
        OS_Sleep(2000);
    }

    GPS_Init();

    if (GPS_Open(NULL))
    {
        Trace(GPS_TRACE_INDEX, "GPS Configurations complete...");
    }
    else
    {
        Local_bInitState = false;
        Trace(GPS_TRACE_INDEX, "GPS Configurations failed...");
    }

    if(Local_bInitState)
    {
        while (Local_psGpsInfo->rmc.latitude.value == 0)
        {
            Trace(GPS_TRACE_INDEX, "Waiting for GPS to start up...");
            OS_Sleep(2000);
        }
        Trace(GPS_TRACE_INDEX, "GPS has started...");

        while (GPS_SetOutputInterval(10000) == false)
        {
            OS_Sleep(1000);
        }
        Trace(GPS_TRACE_INDEX, "Set GPS output interval to 10 seconds...");
    }

    if (GPS_GetVersion(Local_au8GpsBuffer, 150) == true)
    {
        Trace(GPS_TRACE_INDEX, "GPS version %s", Local_au8GpsBuffer);
    }
    else
    {
        Local_bInitState = false;
        Trace(GPS_TRACE_INDEX, "Get GPS version failed!!");
    }

    Trace(GPS_TRACE_INDEX, "GPS initialization success...");

    GpsIsOpen = Local_bInitState;

    if(Local_bInitState)
    {
        UART_Init(UART1, Local_sUartConfig);
    }


    while (1)
    {

        if (GpsIsOpen == true)
        {
            Local_u8IsFixed = MAX(Local_psGpsInfo->gsa[0].fix_type, Local_psGpsInfo->gsa[1].fix_type);
            if(Local_u8IsFixed == 2)
            {
                Local_pu8FixStr = "2D Fix";
            }
            else if(Local_u8IsFixed == 3)
            {
                if(Local_psGpsInfo->gga.fix_quality == 1)
                {
                    Local_pu8FixStr = "3D Fix";
                }
                else if(Local_psGpsInfo->gga.fix_quality == 2)
                {
                    Local_pu8FixStr = "3D/DGPS Fix";
                }
            }
            else
            {
                Local_pu8FixStr = "No Fix";
            }

            Local_i32Temp = (int32_t)(Local_psGpsInfo->rmc.latitude.value / Local_psGpsInfo->rmc.latitude.scale / 100);
            Local_dLatitude = Local_i32Temp + (double)(Local_psGpsInfo->rmc.latitude.value - Local_i32Temp * Local_psGpsInfo->rmc.latitude.scale * 100) / Local_psGpsInfo->rmc.latitude.scale / 60;
            Local_i32Temp = (int32_t)(Local_psGpsInfo->rmc.longitude.value / Local_psGpsInfo->rmc.longitude.scale / 100);
            Local_dLongitude = Local_i32Temp + (double)(Local_psGpsInfo->rmc.longitude.value - Local_i32Temp * Local_psGpsInfo->rmc.longitude.scale * 100) / Local_psGpsInfo->rmc.longitude.scale / 60;

            memset(Local_au8GpsBuffer, 0x00, 300);
            UART_Write(UART1, Local_au8GpsBuffer, snprintf(Local_au8GpsBuffer, 300, "[Coordinates] Lat: %f degrees, Long: %f degrees\n", Local_dLatitude, Local_dLongitude));
            Trace(GPS_TRACE_INDEX, "[Coordinates] Lat: %f degrees, Long: %f degrees", Local_dLatitude, Local_dLongitude);
            
        }
        else
        {
            Trace(GPS_TRACE_INDEX, "GPS is not available due to initialization error...");
        }

        OS_Sleep(5000);
    }
}


#endif /*   ENABLE_GPS_TASK     */

/* ------------------------------------------------------------------------- */
/* ------------------------------ End Of File ------------------------------ */
/* ------------------------------------------------------------------------- */
