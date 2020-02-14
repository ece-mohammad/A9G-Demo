/* ------------------------------------------------------------------------- */
/* ------------------------------ call_task.c ------------------------------ */
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

#include "api_call.h"
#include "api_audio.h"
#include "api_hal_uart.h"

#include "../include/call_task.h"
#include "../include/F21.h"


HANDLE callTaskHandle;
static bool bAudioReady = false;

extern HANDLE semNetworkRegisteration;
extern HANDLE semSystemReady;

/* ------------------------------------------------------------------------- */
/* ------------------------------ Tasks' APIs ------------------------------ */
/* ------------------------------------------------------------------------- */


void CALL_Task(void *pData)
{
    API_Event_t *Local_psEvent = NULL;
    UART_Config_t Local_sUartConfig = {0};

    Local_sUartConfig.baudRate = UART_BAUD_RATE_115200;
    Local_sUartConfig.dataBits = UART_DATA_BITS_8;
    Local_sUartConfig.parity = UART_PARITY_NONE;
    Local_sUartConfig.stopBits = UART_STOP_BITS_1;
    Local_sUartConfig.rxCallback = NULL;
    Local_sUartConfig.errorCallback = NULL;
    Local_sUartConfig.useEvent = true;

    Trace(CALL_TRACE_INDEX, "[CALL] waiting for network registration...");
    if(OS_WaitForSemaphore(semNetworkRegisteration, OS_TIME_OUT_WAIT_FOREVER))
    {
        Trace(CALL_TRACE_INDEX, "[CALL] Network registration complete...");
    }

    UART_Init(UART1, Local_sUartConfig);

    while (1)
    {
        if (OS_WaitEvent(callTaskHandle, (void **)&Local_psEvent, OS_TIME_OUT_WAIT_FOREVER))
        {
            EventDispatch(Local_psEvent);
            OS_Free(Local_psEvent->pParam1);
            OS_Free(Local_psEvent->pParam2);
            OS_Free(Local_psEvent);
        }
    }
}

/* ------------------------------------------------------------------------- */

void CALL_Incoming_EventHandler(API_Event_t *pEvent)
{
    Trace(CALL_TRACE_INDEX, "[CALL INCOMING EVENT] Incoming call from: %s", pEvent->pParam1);

    OS_Sleep(10000);
    CALL_Answer();

    OS_Sleep(10000);
    CALL_HangUp();
}

/* ------------------------------------------------------------------------- */

void CALL_Dial_EventHandler(API_Event_t *pEvent)
{
    if (pEvent->param1 == true)
    {
        Trace(CALL_TRACE_INDEX, "[CALL DIAL EVENT] Dialing was successful...");
    }
    else
    {
        Trace(CALL_TRACE_INDEX, "[CALL DIAL EVENT] Dialing failed...");
    }
    CALL_AudioInit(10);
}

/* ------------------------------------------------------------------------- */

void CALL_HangUp_EventHandler(API_Event_t *pEvent)
{
    if (pEvent->param1 == true)
    {
        Trace(CALL_TRACE_INDEX, "[CALL HANGUP EVENT] Call hanged by remote release..");
    }
    else
    {
        Trace(CALL_TRACE_INDEX, "[CALL HANGUP EVENT] Call hanged..");
    }
    CALL_AudioDeInit();
}

/* ------------------------------------------------------------------------- */

void CALL_Answer_EventHandler(API_Event_t *pEvent)
{
    Trace(CALL_TRACE_INDEX, "[CALL ANSWER EVENT] Call Answered..");
    CALL_AudioInit(10);
}

/* ------------------------------------------------------------------------- */

void CALL_DTMF_EventHandler(API_Event_t *pEvent)
{
    Trace(CALL_TRACE_INDEX, "[CALL DTMF EVENT] DTMF Key: %d", pEvent->param1);
}

/* ------------------------------------------------------------------------- */

void CALL_UART_RX_EventHandler(API_Event_t *pEvent)
{
    uint8_t Local_au8buffer[100];
    uint8_t *Local_pu8NumStart = NULL;

    switch (pEvent->param1)
    {
    case UART1:
        Trace(CALL_TRACE_INDEX, "[UART RX EVENt] Received command: %s", pEvent->pParam1);
        memcpy(Local_au8buffer, pEvent->pParam1, MIN(100, pEvent->param2));
        if ((Local_pu8NumStart = strstr(pEvent->pParam1, "call:")))
        {
            Local_pu8NumStart += 5;
            if (CALL_Dial(Local_pu8NumStart))
            {
                Trace(CALL_TRACE_INDEX, "[CALL] Calling number: %s", Local_pu8NumStart);
            }
            else
            {
                Trace(CALL_TRACE_INDEX, "[CALL] Failed to call number: %s", Local_pu8NumStart);
            }
        }
        break;

    default:
        break;
    }
}

/* ------------------------------------------------------------------------- */

void CALL_AudioInit(uint32_t Copy_u32SpeakerLevel)
{
    if (bAudioReady == false)
    {
        bAudioReady = true;
        AUDIO_MicOpen();
        AUDIO_SpeakerOpen();
        AUDIO_MicSetMute(false);
        AUDIO_SpeakerSetVolume(Copy_u32SpeakerLevel & 0xF);
        Trace(CALL_TRACE_INDEX, "[AUDIO] Activating audio peripherals");
    }
}

/* ------------------------------------------------------------------------- */

void CALL_AudioDeInit(void)
{
    if (bAudioReady == true)
    {
        bAudioReady = false;
        AUDIO_MicSetMute(true);
        AUDIO_SpeakerSetVolume(0);
        AUDIO_MicClose();
        AUDIO_SpeakerClose();
        Trace(CALL_TRACE_INDEX, "[AUDIO] Deactivatin audio peripherals");
    }
}

/* ------------------------------------------------------------------------- */

bool CALL_AudioReady(void)
{
    return bAudioReady;
}

/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */
/* ------------------------------ End Of File ------------------------------ */
/* ------------------------------------------------------------------------- */

