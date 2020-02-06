#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include <api_os.h>
#include <api_event.h>
#include <api_debug.h>

#include <api_hal_pm.h>

#include "../include/F21.h"

#if defined(ENABLE_GPIO_TASK)
#include <api_hal_gpio.h>
#endif /*   ENABLE_GPIO_TASK   */

#if defined(ENABLE_UART_TASK)
#include <api_hal_uart.h>
#endif /*  ENABLE_UART_TASK    */

static HANDLE mainTaskHandle;

#if defined(ENABLE_GPIO_TASK)
static HANDLE gpioTaskHandle;
#endif

#if defined(ENABLE_UART_TASK)
static HANDLE uartTaskHandle;
#endif

#if defined(ENABLE_CALL_RECEIVE_TASK) || defined(ENABLE_DIAL_TASK)
static HANDLE callTaskHandle;
#endif

#if defined(ENABLE_SMS_RECEIVE_TASK) || defined(ENABLE_SMS_SEND_TASK)
static HANDLE smsTaskHandle;
#endif

#if defined(ENABLE_GPRS_TASK)
static HANDLE gprsTaskHandle;
#endif

#if defined(ENABLE_GPS_TASK)
static HANDLE gpsTaskHandle;
#endif

#if defined(ENABLE_MQTT_TASK)
static HANDLE mqttTaskHandle;
#endif

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
        break;

    case API_EVENT_ID_SYSTEM_READY:
        break;

    case API_EVENT_ID_NO_SIMCARD:
        Trace(3, "No sim card detected!");
        break;

    case API_EVENT_ID_SIMCARD_DROP:
        Trace(3, "Sim card drop!");
        break;

    case API_EVENT_ID_SIGNAL_QUALITY:
        break;

    case API_EVENT_ID_NETWORK_REGISTERED_HOME:
        break;

    case API_EVENT_ID_NETWORK_REGISTERED_ROAMING:
        break;

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

    case API_EVENT_ID_NETWORK_GOT_TIME:
        break;

    case API_EVENT_ID_NETWORK_CELL_INFO:
        break;

    default:
        Trace(1, "handling event id [ %d ]", pEvent->id);
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
    statusLed.pin = STATUS_LED_PIN;
    statusLed.mode = GPIO_MODE_OUTPUT;
    statusLed.defaultLevel = GPIO_LEVEL_HIGH;

    GPIO_LEVEL statusPinLevelUpdate = GPIO_LEVEL_LOW;
    GPIO_LEVEL statusPinLevel = GPIO_LEVEL_LOW;
    GPIO_config_t statusUpdate = {0};
    statusUpdate.pin = STATUS_UPDATE_PIN;
    statusUpdate.mode = GPIO_MODE_INPUT;
    statusUpdate.defaultLevel = GPIO_LEVEL_LOW;
    statusUpdate.intConfig.type = GPIO_INT_TYPE_MAX;
    statusUpdate.intConfig.debounce = 0;
    statusUpdate.intConfig.callback = NULL;

#endif /*  ENABLE_STATUS_UPDATE    */

#if defined(ENABLE_INDICATOR_LED) || defined(ENABLE_STATUS_UPDATE)

    PM_PowerEnable(POWER_TYPE_VPAD, true);
    Trace(1, "Enabled power for gpio pins [25 : 36] and [ 0 : 7 ]");

#endif

#if defined(ENABLE_INDICATOR_LED)

    GPIO_Init(indicatorLed);
    Trace(1, "Configured indicator led...");

#endif /*  ENABLE_INDICATOR_LED    */

#if defined(ENABLE_STATUS_UPDATE)

    GPIO_Init(statusLed);
    Trace(1, "Configured status led...");

    GPIO_Init(statusUpdate);
    Trace(1, "Configured status update pin...");

#endif /*  ENABLE_STATUS_UPDATE    */

    while (1)
    {

#if defined(ENABLE_INDICATOR_LED)

        indicatorLedLevel = (indicatorLedLevel == GPIO_LEVEL_HIGH) ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH;
        GPIO_SetLevel(indicatorLed, indicatorLedLevel);
        Trace(3, "Toggling indicator led...");

#endif /*  ENABLE_INDICATOR_LED    */

#if defined(ENABLE_STATUS_UPDATE)

        GPIO_GetLevel(statusUpdate, &statusPinLevel);
        if (statusPinLevel != statusPinLevelUpdate)
        {
            statusPinLevelUpdate = statusPinLevel;
            statusLedLevel = (statusPinLevel == GPIO_LEVEL_HIGH) ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH;
            GPIO_SetLevel(statusLed, statusLedLevel);
            Trace(3, "Updating status...");
        }

        if (statusPinLevel == GPIO_LEVEL_HIGH)
        {
            Trace(3, "Current status : HIGH");
        }
        else
        {
            Trace(3, "Current status : LOW");
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

    while (1)
    {
    }
}

#endif /*   ENABLE_UART_TASK    */

/* ------------------------------------------------------------------------- */
