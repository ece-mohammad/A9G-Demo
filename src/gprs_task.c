/* ------------------------------------------------------------------------- */
/* ------------------------------ gprs_task.c ------------------------------ */
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

#include "api_network.h"
#include "api_socket.h"
#include "api_hal_uart.h"

#include "../include/gprs_task.h"
#include "../include/F21.h"


HANDLE gprsTaskHandle;
HANDLE cellInfoTaskHandle;

extern HANDLE semNetworkRegisteration;

static HANDLE socketSemaphore;
static Network_PDP_Context_t sGPRS_NetworkContext = {
    .apn = GPRS_AP_NAME,
    .userName = GPRS_USERNAME,
    .userPasswd = GPRS_PASSWORD,
};

static bool bNetworkActivated = false;




void GPRS_Task(void *pData)
{
    API_Event_t *Local_psGprsEvent = NULL;
    UART_Config_t Local_sUartConfig = {0};

    Local_sUartConfig.baudRate = UART_BAUD_RATE_115200;
    Local_sUartConfig.dataBits = UART_DATA_BITS_8;
    Local_sUartConfig.parity = UART_PARITY_NONE;
    Local_sUartConfig.stopBits = UART_STOP_BITS_1;
    Local_sUartConfig.errorCallback = NULL;
    Local_sUartConfig.rxCallback = NULL;
    Local_sUartConfig.useEvent = true;

    UART_Init(UART1, Local_sUartConfig);

    Trace(GPRS_TRACE_INDEX, "[GPRS] Waiting for network registration.");
    if(OS_WaitForSemaphore(semNetworkRegisteration, OS_WAIT_FOREVER))
    {
        Trace(GPRS_TRACE_INDEX, "[GPRS] Network registration complete.");
    }

    Network_StartDetach();
    Network_StartDeactive(1);

    while (!(bNetworkActivated = GPRS_NetworkActivate()))
    {
        Trace(GPRS_TRACE_INDEX, "[GPRS] Network attach failed, retrying.");
        OS_Sleep(5000);
    }
    Trace(GPRS_TRACE_INDEX, "[GPRS] Network activated.");

    while (1)
    {
        if (OS_WaitEvent(gprsTaskHandle, (void **)&Local_psGprsEvent, OS_TIME_OUT_WAIT_FOREVER))
        {
            EventDispatch(Local_psGprsEvent);
            OS_Free(Local_psGprsEvent->pParam1);
            OS_Free(Local_psGprsEvent->pParam2);
            OS_Free(Local_psGprsEvent);
        }
    }
}

/* ------------------------------------------------------------------------- */

void GPRS_GetCellInfoTask(void *pData)
{
    CHAR Local_acNetworkIp[20] = {0};
    
    while (!bNetworkActivated)
    {
        Trace(GPRS_TRACE_INDEX, "[CELL INFO] Waiting for network activation.");
        OS_Sleep(5000);
    }

    while (1)
    {
        if (Network_GetCellInfoRequst())
        {
            Trace(GPRS_TRACE_INDEX, "[CELL INFO] Cell info request complete.");
            if(Network_GetIp(Local_acNetworkIp, sizeof(Local_acNetworkIp)))
            {
                Trace(GPRS_TRACE_INDEX, "[CELL INFO] Network IP: %s", Local_acNetworkIp);
            }
            else
            {
                Trace(GPRS_TRACE_INDEX, "[CELL INFO] Failed to get network IP.");
            }
        }
        else
        {
            Trace(GPRS_TRACE_INDEX, "[CELL INFO] Failed to get cell info.");
        }
        
        OS_Sleep(60000);
    }
}

/* ------------------------------------------------------------------------- */

bool GPRS_NetworkActivate(void)
{
    uint8_t Local_u8AttachStatus = false;
    uint8_t Local_u8ActivateStatus = false;
    bool Local_bRetStatus = false;

    Local_bRetStatus = Network_GetAttachStatus(&Local_u8AttachStatus);
    if (Local_bRetStatus)
    {
        if (Local_u8AttachStatus)
        {
            Trace(GPRS_TRACE_INDEX, "[GPRS] network already attached. Proceeding to network activation.");
        }
        else
        {
            Trace(GPRS_TRACE_INDEX, "[GPRS] Starting network attach process.");
            Local_bRetStatus = Network_StartAttach();
            if (Local_u8AttachStatus)
            {
                Trace(GPRS_TRACE_INDEX, "[GPRS] Network attach successful.");
            }
            else
            {
                Trace(GPRS_TRACE_INDEX, "[GPRS] Network attach failed.");
            }
        }

        if (Local_u8AttachStatus)
        {
            Local_bRetStatus = Network_GetActiveStatus(&Local_u8ActivateStatus);
            if (!Local_bRetStatus)
            {
                Trace(GPRS_TRACE_INDEX, "[GPRS] Failed to get network activate status.");
            }
            else
            {
                if (Local_u8ActivateStatus)
                {
                    Trace(GPRS_TRACE_INDEX, "[GPRS] Network already activated.");
                }
                else
                {
                    Trace(GPRS_TRACE_INDEX, "[GPRS] Starting Network activation process.");
                    Local_u8ActivateStatus = Network_StartActive(sGPRS_NetworkContext);
                    if (!Local_u8ActivateStatus)
                    {
                        Trace(GPRS_TRACE_INDEX, "[GPRS] Network activation failed.");
                    }
                    else
                    {
                        Trace(GPRS_TRACE_INDEX, "[GPRS] Network Activation successful.");
                    }
                }
            }
        }
    }
    else
    {
        Trace(GPRS_TRACE_INDEX, "[GPRS] Failed to get network attach status!");
    }
    return Local_u8ActivateStatus;
}

/* ------------------------------------------------------------------------- */

void GPRS_TASK_Attached_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[NETWORK ATTACHED EVENT] Network Attached.");
}

/* ------------------------------------------------------------------------- */

void GPRS_TASK_AttachFailed_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[NETWORK ATTACHED EVENT] Network Attach failed.");
}

/* ------------------------------------------------------------------------- */

void GPRS_TASK_DeAttached_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[NETWORK ATTACHED EVENT] Network De-Attached.");
}

/* ------------------------------------------------------------------------- */

void GPRS_TASK_Activated_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[NETWORK ATTACHED EVENT] Network Activated.");
}

/* ------------------------------------------------------------------------- */

void GPRS_TASK_ActivateFailed_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[NETWORK ATTACHED EVENT] Network Activate Failure.");
}

/* ------------------------------------------------------------------------- */

void GPRS_TASK_DeActivated_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[NETWORK ATTACHED EVENT] Network De-Activated.");
}

/* ------------------------------------------------------------------------- */

void GPRS_UART_RX_EventHandler(API_Event_t * pEvent)
{
    uint8_t Local_au8Buffer[1024] = {0};
    char * Local_pcDataStart = NULL;
    uint32_t Local_u32DataLen = 0;

    switch(pEvent->param1)
    {
        case UART1:
            if(pEvent->param2)
            {
                memcpy(Local_au8Buffer, pEvent->pParam1, MIN(1024, pEvent->param2));
                Local_pcDataStart = strstr((char *)Local_au8Buffer, "send:");
                Local_pcDataStart += 5;
                Local_u32DataLen = strlen(Local_pcDataStart);
                Trace(GPRS_TRACE_INDEX, "[TCP SEND] Sending TCP data: %s", Local_pcDataStart);
                GPRS_TcpSend(TEST_TCP_SERVER_IP, TEST_TCP_SERVER_PORT, Local_pcDataStart, Local_u32DataLen);
            }
            break;

        default:
            break;
    }
}

/* ------------------------------------------------------------------------- */

void GPRS_SOCKET_CONNECTED_EventHandler(API_Event_t * pEvent)
{
    Trace(TCP_TRACE_INDEX, "[SOCKET CONNECTED EVENT] Socket connected. Socket FD: %d", pEvent->param1);
}

/* ------------------------------------------------------------------------- */

void GPRS_SOCKET_CLOSED_EventHandler(API_Event_t * pEvent)
{
    Trace(TCP_TRACE_INDEX, "[SOCKET CLOSED EVENT] Socket [ %d ] closed.", pEvent->param1);
}

/* ------------------------------------------------------------------------- */

void GPRS_SOCKET_SENT_EventHandler(API_Event_t * pEvent)
{
    Trace(TCP_TRACE_INDEX, "[SOCKET SENT EVENT] Socket [ %d ] Send complete.", pEvent->param1);
}

/* ------------------------------------------------------------------------- */

void GPRS_SOCKET_RECEIVED_EventHandler(API_Event_t * pEvent)
{
    Trace(TCP_TRACE_INDEX, "[SOCKET rECEIVE EVENT] Socket [ %d ] received [ %d ] bytes", pEvent->param1, pEvent->param2);
    OS_ReleaseSemaphore(socketSemaphore);
}

/* ------------------------------------------------------------------------- */

void GPRS_SOCKET_ERROR_EventHandler(API_Event_t * pEvent)
{
    Trace(TCP_TRACE_INDEX, "[SOCKET ERROR EVENT] Socket Error on socket [%d ] Code: %d", pEvent->param1, pEvent->param2);
}

/* ------------------------------------------------------------------------- */

void GPRS_DNS_SUCCESS_EventHandler(API_Event_t * pEvent)
{

}

/* ------------------------------------------------------------------------- */

void GPRS_DNS_FAIL_EventHandler(API_Event_t * pEvent)
{

}

/* ------------------------------------------------------------------------- */

int32_t GPRS_TcpSend(const char * Copy_pcAddress, uint32_t Copy_u32PortNum, uint8_t * Copy_au8Txbuffer, uint32_t  Copy_u32BuffSize)
{
    int32_t Local_i32SocketErr = 0;
    uint32_t Local_u32SocketFD = 0;

    socketSemaphore = OS_CreateSemaphore(0);

    Local_i32SocketErr = Socket_TcpipConnect(TCP, TEST_TCP_SERVER_IP, TEST_TCP_SERVER_PORT);
    if(Local_i32SocketErr < 0)
    {
        Trace(TCP_TRACE_INDEX, "[SOCKET] Socekt connection failed. Error code [ %d ]", Local_i32SocketErr);
    }
    else
    {
        Local_u32SocketFD = Local_i32SocketErr;
        Trace(TCP_TRACE_INDEX, "[SOCKET] Created Socket. Socket FD: [ %d ]", Local_u32SocketFD);

        Trace(TCP_TRACE_INDEX, "[SOCKET] Sending data : %s [ %d bytes ]", Copy_au8Txbuffer, Copy_u32BuffSize);
        Local_i32SocketErr = Socket_TcpipWrite(Local_u32SocketFD, Copy_au8Txbuffer, Copy_u32BuffSize);
        if(Local_i32SocketErr > 0)
        {
            Trace(TCP_TRACE_INDEX, "[SOCKET] Data sent successfyully. Waiting for server's reply.");
            OS_WaitForSemaphore(socketSemaphore, OS_TIME_OUT_WAIT_FOREVER);
            Local_i32SocketErr = Socket_TcpipRead(Local_u32SocketFD, Copy_au8Txbuffer, Copy_u32BuffSize);
            if(Local_i32SocketErr > 0)
            {
                Trace(TCP_TRACE_INDEX, "[SOCKET] Server response: %s", Copy_au8Txbuffer);
            }
            else
            {
                Trace(TCP_TRACE_INDEX, "[SOCKET] Socket read failed");
            }
        }
        else
        {
            Trace(TCP_TRACE_INDEX, "[SOCKET] Socekt Write failed. Error code : [ %d ]", Local_i32SocketErr);
        }

        Socket_TcpipClose(Local_u32SocketFD);
    }
    
    return Local_i32SocketErr;
}

/* ------------------------------------------------------------------------- */

int32_t GPRS_HttpGet(const char *Copy_pcAddress, uint8_t *Copy_pu8Response, uint32_t Copy_u32BufferSize)
{
    int32_t Local_i32SocketErr = 0;
    uint32_t Local_u32SocketFD = 0;


    
    return Local_i32SocketErr;
}

/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */
/* ------------------------------ End Of File ------------------------------ */
/* ------------------------------------------------------------------------- */

