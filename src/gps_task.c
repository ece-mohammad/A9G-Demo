/* ------------------------------------------------------------------------- */
/* ------------------------------- gps_task.c ------------------------------ */
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

#include "api_gps.h"
#include "api_hal_uart.h"
#include "gps.h"
#include "buffer.h"
#include "gps_parse.h"

#include "../include/F21.h"
#include "../include/gps_task.h"


HANDLE gpsTaskHandle;

extern HANDLE semNetworkRegisteration;

static bool GpsIsOpen = true;



void GPS_Task(void *pData)
{
    GPS_Info_t *Local_psGpsInfo = Gps_GetInfo();
    uint8_t Local_au8GpsBuffer[300] = {0};
    bool Local_bInitState = true;
    uint8_t Local_u8IsFixed = 0;
    CHAR *Local_pu8FixStr = NULL;
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

    Trace(GPS_TRACE_INDEX, "[GPS] Waiting for network registration.");
    if(OS_WaitForSemaphore(semNetworkRegisteration, OS_WAIT_FOREVER))
    {
        Trace(GPS_TRACE_INDEX, "[GPS] Network registration complete.");
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

    if (Local_bInitState)
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

    if (Local_bInitState)
    {
        UART_Init(UART1, Local_sUartConfig);
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
            UART_Write(UART1, Local_au8GpsBuffer, snprintf(Local_au8GpsBuffer, 300, "[Coordinates (latitude, logitude)] : %f , %f degrees\n", Local_dLatitude, Local_dLongitude));
            Trace(GPS_TRACE_INDEX, "[GPS INFO] Fix mode: %d, BDS Fix mode: %d, fix quality: %d", Local_psGpsInfo->gsa[0].fix_type, Local_psGpsInfo->gsa[1].fix_type, Local_psGpsInfo->gga.fix_quality);
            Trace(GPS_TRACE_INDEX, "[GPS INFO] Satellites tracked: %d, GPS staellites total: %d, is fixed: %s", Local_psGpsInfo->gga.satellites_tracked, Local_psGpsInfo->gsv[0].total_sats, Local_pu8FixStr);
            Trace(GPS_TRACE_INDEX, "[GPS INFO] Coordinate System: WGS84, %f, %f degrees, altitude: %d", Local_dLatitude, Local_dLongitude, Local_psGpsInfo->gga.altitude.value);
        }
        else
        {
            Trace(GPS_TRACE_INDEX, "GPS is not available due to initialization error...");
        }

        OS_Sleep(10000);
    }
}

/* ------------------------------------------------------------------------- */

void GPS_UART_RX_EventHandler(API_Event_t *pEvent)
{
    if (pEvent->param1)
    {
        GPS_Update(pEvent->pParam1, pEvent->param1);
        Trace(GPS_TRACE_INDEX, "[GPS UART EVENT] Updating GPS data : %s", pEvent->pParam1);
    }
}


/* ------------------------------------------------------------------------- */
/* ------------------------------ End Of File ------------------------------ */
/* ------------------------------------------------------------------------- */