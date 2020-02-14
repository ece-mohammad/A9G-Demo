/* ------------------------------------------------------------------------- */
/* ------------------------------ sms_task.c ------------------------------- */
/* ------------------------------------------------------------------------- */

/**!*
 * \file sms_task.c
 * \brief This file 
 * 
 * 
 *!*/

/* ------------------------------------------------------------------------- */
/* ------------------------------- Includes -------------------------------- */
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

#include "api_sms.h"
#include "api_hal_uart.h"

#include"../include/sms_task.h"
#include "../include/F21.h"

/* ------------------------------------------------------------------------- */
/* --------------------------- Global  Variables --------------------------- */
/* ------------------------------------------------------------------------- */

HANDLE smsTaskHandle;

extern HANDLE semNetworkRegisteration;

/* ------------------------------------------------------------------------- */
/* --------------------------- Private Variables --------------------------- */
/* ------------------------------------------------------------------------- */

static bool bSmsInitState = true;

/* ------------------------------------------------------------------------- */
/* ------------------------------- SMS  APIs ------------------------------- */
/* ------------------------------------------------------------------------- */

void SMS_Task(void *pData)
{
    API_Event_t *Local_psSmsEvent = NULL;
    SMS_Parameter_t Local_sSmsParam = {0};
    UART_Config_t Local_sUartConf = {0};

    Local_sSmsParam.fo = 17;
    Local_sSmsParam.vp = 167;
    Local_sSmsParam.pid = 0;
    Local_sSmsParam.dcs = 8; // For English

    Local_sUartConf.baudRate = UART_BAUD_RATE_115200;
    Local_sUartConf.dataBits = UART_DATA_BITS_8;
    Local_sUartConf.stopBits = UART_STOP_BITS_1;
    Local_sUartConf.parity = UART_PARITY_NONE;
    Local_sUartConf.errorCallback = NULL;
    Local_sUartConf.rxCallback = NULL;
    Local_sUartConf.useEvent = true;

    Trace(SMS_TRACE_INDEX, "[SMS] Waiting for network registration.");
    if(OS_WaitForSemaphore(semNetworkRegisteration, OS_WAIT_FOREVER))
    {
        Trace(SMS_TRACE_INDEX, "[SMS] Network registration conplete.");
    }

    if (SMS_SetFormat(SMS_FORMAT_TEXT, SIM0) == true)
    {
        Trace(SMS_TRACE_INDEX, "[SMS] Setting SMS format successful.");
    }
    else
    {
        Trace(SMS_TRACE_INDEX, "[SMS] Setting SMS format failed.");
        bSmsInitState = false;
    }

    if ((bSmsInitState == true) && (SMS_SetParameter(&Local_sSmsParam, SIM0) == true))
    {
        Trace(SMS_TRACE_INDEX, "[SMS] SMS paramter set successfully.");
    }
    else
    {
        Trace(SMS_TRACE_INDEX, "[SMS] SMS paramter set failed.");
        bSmsInitState = false;
    }

    if ((bSmsInitState == true) && (SMS_SetNewMessageStorage(SMS_STORAGE_SIM_CARD) == true))
    {
        Trace(SMS_TRACE_INDEX, "[SMS] Allocated memory for SMS in SIM card.");
    }
    else
    {
        Trace(SMS_TRACE_INDEX, "[SMS]  Failed to allocated memory for SMS storage.");
        bSmsInitState = false;
    }

    if (bSmsInitState == true)
    {
        Trace(SMS_TRACE_INDEX, "[SMS] SMS initialization complete.");
    }
    else
    {
        Trace(SMS_TRACE_INDEX, "[SMS] SMS initialization failed.");
    }

    UART_Init(UART1, Local_sUartConf);

    while (1)
    {
        if (OS_WaitEvent(smsTaskHandle, (void **)&Local_psSmsEvent, OS_TIME_OUT_WAIT_FOREVER) == true)
        {
            EventDispatch(Local_psSmsEvent);
            OS_Free(Local_psSmsEvent->pParam1);
            OS_Free(Local_psSmsEvent->pParam2);
            OS_Free(Local_psSmsEvent);
        }
    }
}

/* ------------------------------------------------------------------------- */

void SMS_UART_RX_EventHandler(API_Event_t *pEvent)
{
    {
        uint8_t Local_au8MessageBody[400] = {0};
        uint8_t Local_au8MessageBuffer[200] = {0};
        uint32_t Local_u32MessageLen = 0;
        uint32_t Local_u32Index = 0;
        char *Local_pcMessageStart = NULL;

        switch (pEvent->param1)
        {
        case UART1:
            Trace(SMS_TRACE_INDEX, "[UART RX EVENT] Received string: %s", pEvent->pParam1);
            if ((Local_pcMessageStart = strstr(pEvent->pParam1, SMS_CMD_PREFIX)))
            {
                Local_pcMessageStart += strlen(SMS_CMD_PREFIX);
                Local_u32MessageLen = sprintf(Local_au8MessageBuffer, "%s", Local_pcMessageStart);
                for (; Local_u32Index < Local_u32MessageLen; Local_u32Index++)
                {
                    Local_au8MessageBody[(Local_u32Index << 1) + 1] = Local_au8MessageBuffer[Local_u32Index];
                }
                SMS_SendMessage(SMS_TEST_PHONE_NUMBER, (const uint8_t *)Local_au8MessageBody, (Local_u32MessageLen << 1), SIM0);
                Trace(SMS_TRACE_INDEX, "[SMS] Sent SMS: %s", Local_pcMessageStart);
            }
            else
            {
                Trace(SMS_TRACE_INDEX, "[SMS] Invalid command prefix");
            }
            
            break;

        default:
            break;
        }
    }
}

/* ------------------------------------------------------------------------- */

void SMS_Received_EventHandler(API_Event_t *pEvent)
{
    uint8_t Local_au8messageBuffer[300] = {0};
    Trace(SMS_TRACE_INDEX, "[SMS RECEIVE EVENT] Received SMS : %s", pEvent->pParam2);
    Trace(SMS_TRACE_INDEX, "[SMS RECEIVE EVENT] SMS Encoding: unicode, message length: %d", pEvent->param2);
    Trace(SMS_TRACE_INDEX, "[SMS RECEIVE EVENT] SMS Header info: %s", pEvent->pParam1);
    UART_Write(UART1, Local_au8messageBuffer, sprintf(Local_au8messageBuffer, "RECEIVED SMS : %s\n", pEvent->pParam2));
}

/* ------------------------------------------------------------------------- */
/* ------------------------------ End Of File ------------------------------ */
/* ------------------------------------------------------------------------- */
