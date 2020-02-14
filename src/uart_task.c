/* ------------------------------------------------------------------------- */
/* ------------------------------ uart_task.h ------------------------------ */
/* ------------------------------------------------------------------------- */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "api_os.h"
#include "api_event.h"
#include "api_debug.h"

#include "api_hal_gpio.h"
#include "api_hal_uart.h"
#include "../include/uart_task.h"


HANDLE uartTaskHandle;

/* ------------------------------------------------------------------------- */
/* ------------------------------- UART APIs ------------------------------- */
/* ------------------------------------------------------------------------- */

void UART_Task(void *pData)
{

#if defined(ENABLE_UART_EVENTS)
    API_Event_t *Local_psUartEvent = NULL;
#endif /*  ENABLE_UART_EVENTS  */

    UART_Config_t uartConfig = {0};

    uartConfig.baudRate = UART_BAUD_RATE_115200;
    uartConfig.dataBits = UART_DATA_BITS_8;
    uartConfig.errorCallback = UART_ErrorCallback;
    uartConfig.parity = UART_PARITY_NONE;
    uartConfig.stopBits = UART_STOP_BITS_1;

#if defined(ENABLE_UART_EVENTS)
    uartConfig.rxCallback = NULL;
    uartConfig.useEvent = true;
#else
    uartConfig.rxCallback = UART_RxCallback;
    uartConfig.useEvent = false;
#endif /*  !ENABLE_UART_EVENTS  */

    if (UART_Init(UART1, uartConfig))
    {
        Trace(UART_TRACE_INDEX, "[UART] UART 1 configuration complete...");
    }
    else
    {
        Trace(UART_TRACE_INDEX, "[UART] Failed to configure UART 1");
    }

    if (UART_Init(UART2, uartConfig))
    {
        Trace(UART_TRACE_INDEX, "[UART] UART 2 configuration complete...");
    }
    else
    {
        Trace(UART_TRACE_INDEX, "[UART] Failed to configure UART 1");
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

#if defined(ENABLE_UART_EVENTS)

void UART_RX_EventHandler(API_Event_t *pEvent)
{
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
}

#else

void UART_RxCallback(UART_Callback_Param_t param)
{
    switch (param.port)
    {
    case UART1:
        Trace(UART_TRACE_INDEX, "[UART] UART 1 Received [%d] bytes : %s", param.length, param.buf);
        UART_Write(UART1, param.buf, param.length);
        break;

    case UART2:
        Trace(UART_TRACE_INDEX, "[UART] UART 2 Received [%d] bytes : %s", param.length, param.buf);
        UART_Write(UART2, param.buf, param.length);
        break;

    default:
        break;
    }
}

/* ------------------------------------------------------------------------- */

void UART_ErrorCallback(UART_Error_t error)
{
    Trace(UART_TRACE_INDEX, "UART Error callback");
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
/* ------------------------------ End Of File ------------------------------ */
/* ------------------------------------------------------------------------- */