/* ------------------------------------------------------------------------- */
/* ------------------------------ gprs_task.h ------------------------------ */
/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */
/* --------------------------- Tasks' Constants ---------------------------- */
/* ------------------------------------------------------------------------- */

#define GPRS_TASK_PRIORITY          6
#define GPRS_TASK_STACK_SIZE        1024
#define GPRS_TASK_NAME              "GprsTask"

#define GPRS_TRACE_INDEX            6
#define TCP_TRACE_INDEX             7
#define HTTP_TRACE_INDEX            8

#define GPRS_AP_NAME                "internet.vodafone.net"
#define GPRS_USERNAME               ""
#define GPRS_PASSWORD               ""

#define TEST_TCP_SERVER_ADDRESS     "0.tcp.ngrok.io"
#define TEST_TCP_SERVER_PORT        10443

#define GPRS_UART_CMD_PREFIX        "send:"

/* ------------------------------------------------------------------------- */
/* ------------------------------ Tasks' APIs ------------------------------ */
/* ------------------------------------------------------------------------- */

void GPRS_Task(void * pData);
void GPRS_GetCellInfoTask(void * pData);
void GPRS_LocationUpdateTask(void * pData);
int32_t GPRS_TcpSend(const char * Copy_pcAddress, uint32_t Copy_u32PortNum, uint8_t * Copy_au8Txbuffer, uint32_t  Copy_u32BuffSize);
int32_t GPRS_HttpGet(const char *Copy_pcAddress, uint8_t *Copy_pu8Response, uint32_t Copy_u32BufferSize);
bool GPRS_NetworkActivate(void);

/* GPRS Events   */
void GPRS_TASK_NetworkRegistered_EventHandler(API_Event_t *pEvent);
void GPRS_TASK_Attached_EventHandler(API_Event_t * pEvent);
void GPRS_TASK_AttachFailed_EventHandler(API_Event_t * pEvent);
void GPRS_TASK_DeAttached_EventHandler(API_Event_t * pEvent);
void GPRS_TASK_Activated_EventHandler(API_Event_t * pEvent);
void GPRS_TASK_ActivateFailed_EventHandler(API_Event_t * pEvent);
void GPRS_TASK_DeActivated_EventHandler(API_Event_t * pEvent);
void GPRS_UART_RX_EventHandler(API_Event_t * pEvent);

/* DNS Events   */
void GPRS_DNS_SUCCESS_EventHandler(API_Event_t * pEvent);
void GPRS_DNS_FAIL_EventHandler(API_Event_t * pEvent);

/* Socket Events   */
void GPRS_SOCKET_CONNECTED_EventHandler(API_Event_t * pEvent);
void GPRS_SOCKET_CLOSED_EventHandler(API_Event_t * pEvent);
void GPRS_SOCKET_SENT_EventHandler(API_Event_t * pEvent);
void GPRS_SOCKET_RECEIVED_EventHandler(API_Event_t * pEvent);
void GPRS_SOCKET_ERROR_EventHandler(API_Event_t * pEvent);

/* ------------------------------------------------------------------------- */
/* ------------------------------ End Of File ------------------------------ */
/* ------------------------------------------------------------------------- */
