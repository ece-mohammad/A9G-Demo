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
#include "api_sim.h"

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

typedef struct __buffer
{
    uint8_t buffer[GPRS_TCP_BUFFER_SIZE];
    bool is_dirty;
    uint32_t len;
} SocketBuffer_t;

/* ------------------------------------------------------------------------- */
/* ---------------------------- Global Variables --------------------------- */
/* ------------------------------------------------------------------------- */

extern bool bNetworkRegisteration;

HANDLE gprsTaskHandle;
HANDLE cellInfoTaskHandle;
HANDLE locationUpdateTaskHandle;


/* ------------------------------------------------------------------------- */
/* --------------------------- Private Variables --------------------------- */
/* ------------------------------------------------------------------------- */

/*  GPS is open flag   */
static bool GpsIsOpen = false;

/*  Network activated flag  */
static bool bNetworkActivated = false;

static const Network_PDP_Context_t sGPRS_NetworkContext = {
    .apn = GPRS_AP_NAME,
    .userName = GPRS_USERNAME,
    .userPasswd = GPRS_PASSWORD,
};

static SocketBuffer_t Socket_sTxBuffer;
static SocketBuffer_t Socket_sRxBuffer;
static int32_t Socket_i32Fd = 0;
static uint8_t DeviceICCID [21] = {0};
static uint8_t DeviceIMSI [21] = {0};

/* ------------------------------------------------------------------------- */
/* ---------------------------- API Definitions ---------------------------- */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* ---------------------------- Tasks Functions ---------------------------- */
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

    Socket_sTxBuffer.is_dirty = false;
    Socket_sTxBuffer.len = 0;
    memset(Socket_sTxBuffer.buffer, 0x00, GPRS_TCP_BUFFER_SIZE);

    Socket_sRxBuffer.is_dirty = false;
    Socket_sRxBuffer.len = 0;
    memset(Socket_sRxBuffer.buffer, 0x00, GPRS_TCP_BUFFER_SIZE);

    UART_Init(UART1, Local_sUartConfig);

    while (1)
    {
        OS_Sleep(60000);
    }
}

/* ------------------------------------------------------------------------- */

void GPRS_GetCellInfoTask(void *pData)
{
    CHAR Local_acNetworkIp[20] = {0};

    while(!bNetworkActivated)
    {
        Trace(GPRS_TRACE_INDEX, "[GPRS CELL INFO] Waiting for network activation.");
        OS_Sleep(5000);
    }
    Trace(GPRS_TRACE_INDEX, "[GPRS CELL INFO] Network activation complete.");

    SIM_GetICCID(DeviceICCID);
    SIM_GetIMSI(DeviceIMSI);

    while (1)
    {
        if (Network_GetCellInfoRequst())
        {
            Trace(GPRS_TRACE_INDEX, "[GPRS CELL INFO] Cell info request complete.");
        }
        else
        {
            Trace(GPRS_TRACE_INDEX, "[GPRS CELL INFO] Failed to get cell info.");
        }

        if (Network_GetIp(Local_acNetworkIp, sizeof(Local_acNetworkIp)))
        {
            Trace(GPRS_TRACE_INDEX, "[GPRS CELL INFO] Network IP: %s", Local_acNetworkIp);
        }
        else
        {
            Trace(GPRS_TRACE_INDEX, "[GPRS CELL INFO] Failed to get network IP.");
        }

        OS_Sleep(60000);
    }
}

/* ------------------------------------------------------------------------- */

void GPRS_LocationUpdateTask(void *pData)
{
    GPS_Info_t *Local_psGpsInfo = Gps_GetInfo();
    uint8_t Local_au8GpsBuffer[300] = {0};
    bool Local_bInitState = true;
    uint8_t Local_u8IsFixed = 0;
    CHAR *Local_pu8FixStr = NULL;
    double Local_dLatitude = 0;
    double Local_dLongitude = 0;
    int32_t Local_i32Temp = 0;

    while (!bNetworkRegisteration)
    {
        Trace(LOC_UPDATE_TRACE_INDEX, "[GPRS GPS] Waiting for network registration.");
        OS_Sleep(1000);
    }
    Trace(LOC_UPDATE_TRACE_INDEX, "[GPRS GPS] Network registration complete.");

    GPS_Init();

    if (GPS_Open(NULL))
    {
        Trace(LOC_UPDATE_TRACE_INDEX, "[GPRS GPS] GPS Configurations complete.");
    }
    else
    {
        Local_bInitState = false;
        Trace(LOC_UPDATE_TRACE_INDEX, "[GPRS GPS] GPS Configurations failed.");
    }

    if (Local_bInitState)
    {
        while (Local_psGpsInfo->rmc.latitude.value == 0)
        {
            Trace(LOC_UPDATE_TRACE_INDEX, "[GPRS GPS] Waiting for GPS to start up.");
            OS_Sleep(3000);
        }
        Trace(LOC_UPDATE_TRACE_INDEX, "[GPRS GPS] GPS has started.");

        Trace(LOC_UPDATE_TRACE_INDEX, "[GPRS GPS] Setting GPS update interval.");
        for (Local_i32Temp = 0; Local_i32Temp < 10; Local_i32Temp++)
        {
            if(GPS_SetOutputInterval(10000) == false)
            {
                Trace(LOC_UPDATE_TRACE_INDEX, "[GPRS GPS] Failed to set GPS update interval for [%d] times. Retrying in 3 seconds.", Local_i32Temp);
                OS_Sleep(5000);
            }
            else
            {
                Local_i32Temp = 10;
                Local_bInitState = true;
            }
        }
        Trace(LOC_UPDATE_TRACE_INDEX, "[GPRS GPS] Set GPS output interval to 10 seconds.");
    }

    if (GPS_GetVersion(Local_au8GpsBuffer, 150) == true)
    {
        Trace(LOC_UPDATE_TRACE_INDEX, "[GPRS GPS] GPS version %s", Local_au8GpsBuffer);
    }
    else
    {
        Local_bInitState = false;
        Trace(LOC_UPDATE_TRACE_INDEX, "[GPRS GPS] Get GPS version failed!!");
    }

    GPS_SetOutputInterval(10000);

    GpsIsOpen = Local_bInitState;

    if(Local_bInitState)
    {
        Trace(LOC_UPDATE_TRACE_INDEX, "[GPRS GPS] GPS initialization success.");
    }

    while (1)
    {

        if (GpsIsOpen == true)
        {
            Local_u8IsFixed = MAX(Local_psGpsInfo->gsa[0].fix_type, Local_psGpsInfo->gsa[1].fix_type);
            if (Local_u8IsFixed == 2)
            {
                Local_pu8FixStr = "2D Fix";
            }
            else if (Local_u8IsFixed == 3)
            {
                if (Local_psGpsInfo->gga.fix_quality == 1)
                {
                    Local_pu8FixStr = "3D Fix";
                }
                else if (Local_psGpsInfo->gga.fix_quality == 2)
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
            snprintf(Local_au8GpsBuffer, 300, "Device [%s] Coordinates (WGS84) (latitude, logitude) : %f , %f degrees", DeviceICCID, Local_dLatitude, Local_dLongitude);
            Trace(LOC_UPDATE_TRACE_INDEX, "[GPRS GPS] Sending current location to the server: %s", Local_au8GpsBuffer);
            GPRS_TcpSend(TEST_TCP_SERVER_ADDRESS, TEST_TCP_SERVER_PORT, Local_au8GpsBuffer, strlen(Local_au8GpsBuffer));
            Trace(LOC_UPDATE_TRACE_INDEX, "[GPS INFO] Fix mode: %d, BDS Fix mode: %d, fix quality: %d", Local_psGpsInfo->gsa[0].fix_type, Local_psGpsInfo->gsa[1].fix_type, Local_psGpsInfo->gga.fix_quality);
            Trace(LOC_UPDATE_TRACE_INDEX, "[GPS INFO] Satellites tracked: %d, GPS staellites total: %d, is fixed: %s", Local_psGpsInfo->gga.satellites_tracked, Local_psGpsInfo->gsv[0].total_sats, Local_pu8FixStr);
            Trace(LOC_UPDATE_TRACE_INDEX, "[GPS INFO] Coordinate System: WGS84, %f, %f degrees, altitude: %d", Local_dLatitude, Local_dLongitude, Local_psGpsInfo->gga.altitude.value);
        }
        else
        {
            Trace(LOC_UPDATE_TRACE_INDEX, "GPS is not available due to initialization error.");
        }

        OS_Sleep(10000);
    }
}

/* ------------------------------------------------------------------------- */
/* --------------------------- Helper Functions ---------------------------- */
/* ------------------------------------------------------------------------- */

int32_t GPRS_TcpSend(const char *Copy_pcAddress, uint32_t Copy_u32PortNum, uint8_t *Copy_au8Txbuffer, uint32_t Copy_u32BuffSize)
{
    int32_t Local_i32SocketErr = 0;
    uint8_t Local_au8IpAddress[20] = {0};

    if (bNetworkActivated == true)
    {
        Trace(TCP_TRACE_INDEX, "[GPRS TCP DNS] Requesting IP address for domain: %s", Copy_pcAddress);
        while ((Local_i32SocketErr = (DNS_GetHostByName2(Copy_pcAddress, Local_au8IpAddress) & 0x03) != DNS_STATUS_OK))
        {
            Trace(TCP_TRACE_INDEX, "[GPRS TCP DNS] DNS request failed. Error code: %d", Local_i32SocketErr);
            OS_Sleep(1000);
        }
        Trace(TCP_TRACE_INDEX, "[GPRS TCP DNS] DNS Lookup complete. IP address for domain %s is %s", Copy_pcAddress, (char *)Local_au8IpAddress);

        memset(Socket_sTxBuffer.buffer, 0x00, GPRS_TCP_BUFFER_SIZE);
        Copy_u32BuffSize = MIN(GPRS_TCP_BUFFER_SIZE, Copy_u32BuffSize);
        memcpy(Socket_sTxBuffer.buffer, Copy_au8Txbuffer, Copy_u32BuffSize);
        Socket_sTxBuffer.is_dirty = true;
        Socket_sTxBuffer.buffer[Copy_u32BuffSize] = '\n';
        Socket_i32Fd = Socket_TcpipConnect(TCP, Local_au8IpAddress, TEST_TCP_SERVER_PORT);
    }
    else
    {
        Trace(TCP_TRACE_INDEX, "[GPRS TCP SOCKET] can't start TCP connection while network is not connected.");
        Socket_i32Fd = -1;
    }

    return Socket_i32Fd;
}

/* ------------------------------------------------------------------------- */

uint32_t GPRS_TcpRead(void)
{
    uint32_t Local_u32RxBytes = 0;
    if(Socket_sRxBuffer.is_dirty)
    {
        Socket_sRxBuffer.is_dirty = false;
        Local_u32RxBytes = Socket_sRxBuffer.len;
        Trace(TCP_TRACE_INDEX, "[TCP SOCKET] Socket received [%d] bytes: %s", Socket_sRxBuffer.len, Socket_sRxBuffer.buffer);
    }
    return Local_u32RxBytes;
}

/* ------------------------------------------------------------------------- */

int32_t GPRS_HttpGet(const char *Copy_pcAddress, uint8_t *Copy_pu8Response, uint32_t Copy_u32BufferSize)
{
    int32_t Local_i32SocketErr = 0;

    return Local_i32SocketErr;
}

/* ------------------------------------------------------------------------- */
/* -------------------------- UART event handlers -------------------------- */
/* ------------------------------------------------------------------------- */

void GPRS_UART_RX_EventHandler(API_Event_t *pEvent)
{
    uint8_t Local_au8Buffer[1024] = {0};
    char *Local_pcDataStart = NULL;
    uint32_t Local_u32DataLen = 0;

    switch (pEvent->param1)
    {
    case UART1:
        if (pEvent->param2)
        {
            memcpy(Local_au8Buffer, pEvent->pParam1, MIN(1024, pEvent->param2));
            strip(Local_au8Buffer, MIN(1024, pEvent->param2));
            Trace(GPRS_TRACE_INDEX, "[GPRS UART RX] Received cmd: %s", pEvent->pParam1);
            if ((Local_pcDataStart = strstr((char *)Local_au8Buffer, GPRS_UART_CMD_PREFIX)))
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
/* ------------------------ Network event handlers ------------------------- */
/* ------------------------------------------------------------------------- */

void GPRS_NetworkRegistered_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[GPRS NETWORK REGISTRATION EVENT] Starting network attach process.");
    Network_StartAttach();
}

/* ------------------------------------------------------------------------- */

void GPRS_NetworkAttached_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[GPRS NETWORK ATTACHED EVENT] Network Attached.");
    Network_StartActive(sGPRS_NetworkContext);
}

/* ------------------------------------------------------------------------- */

void GPRS_NetworkAttachFailed_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[GPRS NETWORK ATTACH FAILED EVENT] Network Attach failed.");
}

/* ------------------------------------------------------------------------- */

void GPRS_NetworkDeAttached_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[GPRS NETWORK DE-ATTACHED EVENT] Network De-Attached.");
}

/* ------------------------------------------------------------------------- */

void GPRS_NetworkActivated_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[GPRS NETWORK ACTIVATED EVENT] Network Activated.");
    bNetworkActivated = true;
}

/* ------------------------------------------------------------------------- */

void GPRS_NetworkActivationFailed_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[GPRS NETWORK ACTIVATE FAILED EVENT] Network Activate Failure.");
}

/* ------------------------------------------------------------------------- */

void GPRS_NetworkDeActivated_EventHandler(API_Event_t *pEvent)
{
    Trace(GPRS_TRACE_INDEX, "[GPRS NETWORK DE-ACTIVATED EVENT] Network De-Activated.");
    bNetworkActivated = false;
}

/* ------------------------------------------------------------------------- */
/* --------------------------- GPS Event handler --------------------------- */
/* ------------------------------------------------------------------------- */

void GPRS_GPS_EventHandler(API_Event_t *pEvent)
{
    if (pEvent->param1)
    {
        GPS_Update(pEvent->pParam1, pEvent->param1);
        // Trace(TCP_TRACE_INDEX, "[GPRS GPRS GPS UPDATE EVENT] updating GPS data: %s", pEvent->pParam1);
    }
}

/* ------------------------------------------------------------------------- */
/* ------------------------- Scoket event handlers ------------------------- */
/* ------------------------------------------------------------------------- */

void GPRS_SocketConnected_EventHandler(API_Event_t *pEvent)
{
    Trace(TCP_TRACE_INDEX, "[GPRS SOCKET CONNECTED EVENT] Socket connected. Socket FD: %d", pEvent->param1);
    Socket_TcpipWrite(pEvent->param1, Socket_sTxBuffer.buffer, strlen(Socket_sTxBuffer.buffer));
}

/* ------------------------------------------------------------------------- */

void GPRS_SocketClosed_EventHandler(API_Event_t *pEvent)
{
    Trace(TCP_TRACE_INDEX, "[GPRS SOCKET CLOSED EVENT] Socket [ %d ] closed.", pEvent->param1);
}

/* ------------------------------------------------------------------------- */

void GPRS_SokcetSent_EventHandler(API_Event_t *pEvent)
{
    Socket_sTxBuffer.is_dirty = false;
    Socket_TcpipClose(pEvent->param1);
    Trace(TCP_TRACE_INDEX, "[GPRS SOCKET SENT EVENT] Socket [ %d ] Send complete.", pEvent->param1);
}

/* ------------------------------------------------------------------------- */

void GPRS_SocektReceived_EventHandler(API_Event_t *pEvent)
{
    Trace(TCP_TRACE_INDEX, "[GPRS SOCKET RECEIVE EVENT] Socket [ %d ] received [ %d ] bytes", pEvent->param1, pEvent->param2);
    Socket_TcpipRead(pEvent->param1, Socket_sRxBuffer.buffer, MIN(pEvent->param2, GPRS_TCP_BUFFER_SIZE - 1));
    Socket_sRxBuffer.is_dirty = true;
    Socket_sRxBuffer.len = MIN(pEvent->param2, GPRS_TCP_BUFFER_SIZE - 1);
    Trace(TCP_TRACE_INDEX, "[GPRS SOCKET RECEIVE EVENT] Received message: %s", Socket_sRxBuffer.buffer);
}

/* ------------------------------------------------------------------------- */

void GPRS_SocketError_EventHandler(API_Event_t *pEvent)
{
    Trace(TCP_TRACE_INDEX, "[GPRS SOCKET ERROR EVENT] Socket Error on socket [%d ] Code: %d", pEvent->param1, pEvent->param2);
}

/* ------------------------------------------------------------------------- */

void GPRS_DNS_Success_EventHandler(API_Event_t *pEvent)
{
    Trace(TCP_TRACE_INDEX, "[GPRS DNS SUCCESS EVENT HANDLER] DNS request successful. Domain: %s, IP: %s", pEvent->pParam1, pEvent->pParam2);
}

/* ------------------------------------------------------------------------- */

void GPRS_DNS_Fail_EventHandler(API_Event_t *pEvent)
{
    Trace(TCP_TRACE_INDEX, "[GPRS DNS SUCCESS EVENT HANDLER] DNS request failed.");
}

/* ------------------------------------------------------------------------- */
/* ------------------------------ End Of File ------------------------------ */
/* ------------------------------------------------------------------------- */
