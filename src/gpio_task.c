/* ------------------------------------------------------------------------- */
/* ------------------------------ gpio_Task.c ------------------------------ */
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

#include "api_hal_pm.h"
#include "api_hal_gpio.h"
#include "../include/gpio_Task.h"


HANDLE gpioTaskHandle;



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
    Trace(GPIO_TRACE_INDEX, "[GPIO] Enabled power for gpio pins [25 : 36] and [ 0 : 7 ]");

#endif

#if defined(ENABLE_INDICATOR_LED)

    GPIO_Init(indicatorLed);
    Trace(GPIO_TRACE_INDEX, "[GPIO] Configured indicator led...");

#endif /*  ENABLE_INDICATOR_LED    */

#if defined(ENABLE_STATUS_UPDATE)

    GPIO_Init(statusLed);
    Trace(GPIO_TRACE_INDEX, "[GPIO] Configured status led...");

    GPIO_Init(statusUpdate);
    Trace(GPIO_TRACE_INDEX, "[GPIO] Configured status update pin...");

#endif /*  ENABLE_STATUS_UPDATE    */

    while (1)
    {

#if defined(ENABLE_INDICATOR_LED)

        indicatorLedLevel = (indicatorLedLevel == GPIO_LEVEL_HIGH) ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH;
        GPIO_SetLevel(indicatorLed, indicatorLedLevel);
        Trace(GPIO_TRACE_INDEX, "[GPIO] Toggling indicator led...");

#endif /*  ENABLE_INDICATOR_LED    */

#if defined(ENABLE_STATUS_UPDATE)

        GPIO_GetLevel(statusUpdate, &statusPinLevel);
        if (statusPinLevel != statusPinLevelUpdate)
        {
            statusPinLevelUpdate = statusPinLevel;
            statusLedLevel = (statusPinLevel == GPIO_LEVEL_HIGH) ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH;
            GPIO_SetLevel(statusLed, statusLedLevel);
            Trace(GPIO_TRACE_INDEX, "[GPIO] Updating status...");
        }

        if (statusPinLevel == GPIO_LEVEL_HIGH)
        {
            Trace(GPIO_TRACE_INDEX, "[GPIO] Current status : HIGH");
        }
        else
        {
            Trace(GPIO_TRACE_INDEX, "[GPIO] Current status : LOW");
        }

#endif /*  ENABLE_STATUS_UPDATE    */

        OS_Sleep(1000);
    }
}


/* ------------------------------------------------------------------------- */
/* ------------------------------ End Of File ------------------------------ */
/* ------------------------------------------------------------------------- */
