/* ------------------------------------------------------------------------- */
/* ------------------------------ call_task.h ------------------------------ */
/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */
/* --------------------------- Tasks' Constants ---------------------------- */
/* ------------------------------------------------------------------------- */

#define CALL_TASK_PRIORITY          3
#define CALL_TASK_STACK_SIZE        1024
#define CALL_TASK_NAME              "CallTask"

#define CALL_TRACE_INDEX            4

/* ------------------------------------------------------------------------- */
/* ------------------------------ Tasks' APIs ------------------------------ */
/* ------------------------------------------------------------------------- */

void CALL_Task(void * pData);
void CALL_Incoming_EventHandler(API_Event_t * pEvent);
void CALL_Dial_EventHandler(API_Event_t * pEvent);
void CALL_HangUp_EventHandler(API_Event_t * pEvent);
void CALL_Answer_EventHandler(API_Event_t * pEvent);
void CALL_DTMF_EventHandler(API_Event_t * pEvent);
void CALL_UART_RX_EventHandler(API_Event_t * pEvent);
void CALL_AudioInit(uint32_t Copy_u32SpeakerLevel);
void CALL_AudioDeInit(void);
bool CALL_AudioReady(void);

/* ------------------------------------------------------------------------- */
/* ------------------------------ End Of File ------------------------------ */
/* ------------------------------------------------------------------------- */

