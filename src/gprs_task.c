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

#include "gps.h"
#include "buffer.h"
#include "gps_parse.h"

#include "../include/gprs_task.h"
#include "../include/strutil.h"
#include "../include/F21.h"

/* ------------------------------------------------------------------------- */
/* ---------------------------- Global Variables --------------------------- */
/* ------------------------------------------------------------------------- */

extern HANDLE semNetworkRegisteration;

HANDLE gprsTaskHandle;
HANDLE cellInfoTaskHandle;

/* ------------------------------------------------------------------------- */
/* --------------------------- Private Variables --------------------------- */
/* ------------------------------------------------------------------------- */

static bool bNetworkActivated = false;
static bool bSocketConnected = false;
static bool bSocketWriteComplete = true;

static Network_PDP_Context_t sGPRS_NetworkContext = {
    .apn = GPRS_AP_NAME,
    .userName = GPRS_USERNAME,
    .userPasswd = GPRS_PASSWORD,
};

static uint8_t TxBuffer[1024] = {0};
static int32_t SocketFd = 0;

/* ------------------------------------------------------------------------- */
/* ---------------------------- API Definitions ---------------------------- */
/* ------------------------------------------------------------------------- */

void GPRS_Task(void *pData)
{
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

    Trace(GPRS_TRACE_INDEX, "[GPRS] Starting network activation procedure.");
    while(GPRS_NetworkActivate() == false)
    {
        OS_Sleep(2000);
    }
    Trace(GPRS_TRACE_INDEX, "[GPRS] Network activation process complete.");

    while (1)
    {
        OS_Sleep(1000);
    }
}

/* ------------------------------------------------------------------------- */

void GPRS_GetCellInfoTask(void *pData)
{
    CHAR Local_acNetworkIp[20] = {0};
    
    Trace(GPRS_TRACE_INDEX, "[CELL INFO] Waiting for network activation.");
    while(!bNetworkActivated)
    {
        OS_Sleep(1000);
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

void GPRS_LocationUpdateTask(void * pData)
{
    
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

void GPRS_TASK_NetworkRegistered_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[NETWORK REGISTERED EVENT] Starting network attach process");
    Network_StartAttach();
}

/* ------------------------------------------------------------------------- */

void GPRS_TASK_Attached_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[NETWORK ATTACHED EVENT] Network Attached.");
}

/* ------------------------------------------------------------------------- */

void GPRS_TASK_AttachFailed_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[NETWORK ATTACH FAILED EVENT] Network Attach failed.");
}

/* ------------------------------------------------------------------------- */

void GPRS_TASK_DeAttached_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[NETWORK DE-ATTACHED EVENT] Network De-Attached.");
}

/* ------------------------------------------------------------------------- */

void GPRS_TASK_Activated_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[NETWORK ACTIVATED EVENT] Network Activated.");
    bNetworkActivated = true;
}

/* ------------------------------------------------------------------------- */

void GPRS_TASK_ActivateFailed_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[NETWORK ACTIVATE FAILED EVENT] Network Activate Failure.");
    bNetworkActivated = false;
}

/* ------------------------------------------------------------------------- */

void GPRS_TASK_DeActivated_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[NETWORK DE-ACTIVATED EVENT] Network De-Activated.");
    bNetworkActivated = false;
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
                strip(Local_au8Buffer, MIN(1024, pEvent->param2));
                Trace(GPRS_TRACE_INDEX, "[GPRS UART RX] Received cmd: %s", pEvent->pParam1);
                if((Local_pcDataStart = strstr((char *)Local_au8Buffer, GPRS_UART_CMD_PREFIX)))
                {
                    Local_pcDataStart += strlen(GPRS_UART_CMD_PREFIX);
                    Local_u32DataLen = strlen(Local_pcDataStart);
                    Trace(GPRS_TRACE_INDEX, "[GPRS UART RX] Sending TCP data: %s", Local_pcDataStart);
                    GPRS_TcpSend(TEST_TCP_SERVER_ADDRESS, TEST_TCP_SERVER_PORT, Local_pcDataStart, Local_u32DataLen);
                }
                else
                {
                    Trace(GPRS_TRACE_INDEX, "[GPRS UART RX] Invalid command prefix");
                }
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
    Socket_TcpipWrite(pEvent->param1, TxBuffer, strlen(TxBuffer));
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
    bSocketWriteComplete = true;
}

/* ------------------------------------------------------------------------- */

void GPRS_SOCKET_RECEIVED_EventHandler(API_Event_t * pEvent)
{
    Trace(TCP_TRACE_INDEX, "[SOCKET RECEIVE EVENT] Socket [ %d ] received [ %d ] bytes", pEvent->param1, pEvent->param2);
}

/* ------------------------------------------------------------------------- */

void GPRS_SOCKET_ERROR_EventHandler(API_Event_t * pEvent)
{
    Trace(TCP_TRACE_INDEX, "[SOCKET ERROR EVENT] Socket Error on socket [%d ] Code: %d", pEvent->param1, pEvent->param2);
    bSocketConnected = false;
}

/* ------------------------------------------------------------------------- */

void GPRS_DNS_SUCCESS_EventHandler(API_Event_t * pEvent)
{
    Trace(TCP_TRACE_INDEX, "[DNS SUCCESS EVENT HANDLER] DNS request successful. Domain: %s, IP: %s", pEvent->pParam1, pEvent->pParam2);
}

/* ------------------------------------------------------------------------- */

void GPRS_DNS_FAIL_EventHandler(API_Event_t * pEvent)
{
    Trace(TCP_TRACE_INDEX, "[DNS SUCCESS EVENT HANDLER] DNS request failed.");
}

/* ------------------------------------------------------------------------- */

int32_t GPRS_TcpSend(const char * Copy_pcAddress, uint32_t Copy_u32PortNum, uint8_t * Copy_au8Txbuffer, uint32_t  Copy_u32BuffSize)
{
    int32_t Local_i32SocketErr = 0;
    uint32_t Local_u32SocketFD = 0;
    uint8_t Local_au8IpAddress[20] = {0};
    CHAR Local_acTxbuffer[1024] = {0};

    if(bNetworkActivated == true)
    {
        if(bSocketWriteComplete == true)
        {
            bSocketWriteComplete = false;
            Trace(TCP_TRACE_INDEX, "[TCP DNS] Requesting IP address for domain: %s", Copy_pcAddress);
            while( (Local_i32SocketErr = (DNS_GetHostByName2(Copy_pcAddress, Local_au8IpAddress) & 0x03) != DNS_STATUS_OK) )
            {
                Trace(TCP_TRACE_INDEX, "[TCP DNS] DNS request failed. Error code: %d", Local_i32SocketErr);
                OS_Sleep(1000);
            }
            Trace(TCP_TRACE_INDEX, "[TCP DNS] DNS Lookup complete. IP address for domain %s is %s", Copy_pcAddress, (char *)Local_au8IpAddress);

            memset(TxBuffer, 0x00, sizeof(TxBuffer));
            memcpy(TxBuffer, Copy_au8Txbuffer, Copy_u32BuffSize);
            TxBuffer[Copy_u32BuffSize] = '\n';
            SocketFd = Socket_TcpipConnect(TCP, Local_au8IpAddress, TEST_TCP_SERVER_PORT);
        }
        else
        {
            Trace(TCP_TRACE_INDEX, "[TCP SOCKET] A write operation is already pending.");
            SocketFd = -2;
        }
        
    }
    else
    {
        Trace(TCP_TRACE_INDEX, "[TCP SOCKET] can't start TCP connection while network is not connected.");
        SocketFd = -1;
    }
    
    return SocketFd;
}

/* ------------------------------------------------------------------------- */

int32_t GPRS_HttpGet(const char *Copy_pcAddress, uint8_t *Copy_pu8Response, uint32_t Copy_u32BufferSize)
{
    int32_t Local_i32SocketErr = 0;


    
    return Local_i32SocketErr;
}


/* ------------------------------------------------------------------------- */
/* ------------------------------ End Of File ------------------------------ */
/* ------------------------------------------------------------------------- */

