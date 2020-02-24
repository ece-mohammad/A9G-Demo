/* ------------------------------------------------------------------------- */
/* ------------------------------ gprs_task.h ------------------------------ */
/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */
/* --------------------------- Tasks' Constants ---------------------------- */
/* ------------------------------------------------------------------------- */

#define NETWORK_ACTIVATE_TASK_PRIORITY      6
#define NETWORK_ACTIVATE_TASK_STACK_SIZE    256
#define NETWORK_ACTIVATE_TASK_NAME          "NetActvTask"

#define GPRS_TASK_PRIORITY                  7
#define GPRS_TASK_STACK_SIZE                1024
#define GPRS_TASK_NAME                      "GprsTask"

#define LOCATION_UPDATE_TASK_PRIORITY       8
#define LOCATION_UPDATE_TASK_STACK_SIZE     1024
#define LOCATION_UPDATE_TASK_NAME           "locaUpdateTask"

#define CELL_INFO_TASK_PRIORITY             9
#define CELL_INFO_TASK_STACK_SIZE           1024
#define CELL_INFO_TASK_NAME                 "CellInfoTask"

#define GPRS_TRACE_INDEX                    6
#define TCP_TRACE_INDEX                     7
#define LOC_UPDATE_TRACE_INDEX              8
#define HTTP_TRACE_INDEX                    9

#define GPRS_AP_NAME                        "internet.vodafone.net"
#define GPRS_USERNAME                       ""
#define GPRS_PASSWORD                       ""

#define TEST_TCP_SERVER_ADDRESS             "0.tcp.ngrok.io"
#define TEST_TCP_SERVER_PORT                16309

#define GPRS_UART_CMD_PREFIX                "send:"

#define GPRS_TCP_BUFFER_SIZE                1024

/* ------------------------------------------------------------------------- */
/* ------------------------------ Tasks' APIs ------------------------------ */
/* ------------------------------------------------------------------------- */

void GPRS_Task(void * pData);
void GPRS_GetCellInfoTask(void * pData);
void GPRS_LocationUpdateTask(void * pData);

int32_t GPRS_TcpSend(const char * Copy_pcAddress, uint32_t Copy_u32PortNum, uint8_t * Copy_au8Txbuffer, uint32_t  Copy_u32BuffSize);
int32_t GPRS_HttpGet(const char *Copy_pcAddress, uint8_t *Copy_pu8Response, uint32_t Copy_u32BufferSize);
int32_t GPRS_HttpPost(const char *Copy_pcAddress, uint8_t *Copy_pu8Response, uint32_t Copy_u32BufferSize);
int32_t GPRS_MqttSend(const char *Copy_pcAddress, uint8_t *Copy_pu8Response, uint32_t Copy_u32BufferSize);

/* GPRS Events   */
void GPRS_NetworkRegistered_EventHandler(API_Event_t *pEvent);
void GPRS_NetworkAttached_EventHandler(API_Event_t * pEvent);
void GPRS_NetworkAttachFailed_EventHandler(API_Event_t * pEvent);
void GPRS_NetworkDeAttached_EventHandler(API_Event_t * pEvent);
void GPRS_NetworkActivated_EventHandler(API_Event_t * pEvent);
void GPRS_NetworkActivationFailed_EventHandler(API_Event_t * pEvent);
void GPRS_NetworkDeActivated_EventHandler(API_Event_t * pEvent);
void GPRS_UART_RX_EventHandler(API_Event_t * pEvent);

/* DNS Events   */
void GPRS_DNS_Success_EventHandler(API_Event_t * pEvent);
void GPRS_DNS_Fail_EventHandler(API_Event_t * pEvent);

/* Socket Events   */
void GPRS_SocketConnected_EventHandler(API_Event_t * pEvent);
void GPRS_SocketClosed_EventHandler(API_Event_t * pEvent);
void GPRS_SokcetSent_EventHandler(API_Event_t * pEvent);
void GPRS_SocektReceived_EventHandler(API_Event_t * pEvent);
void GPRS_SocketError_EventHandler(API_Event_t * pEvent);

/*  GPS events  */
void GPRS_GPS_EventHandler(API_Event_t * pEvent);

/* ------------------------------------------------------------------------- */
/* ------------------------------ End Of File ------------------------------ */
/* ------------------------------------------------------------------------- */
