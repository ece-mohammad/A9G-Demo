/* ------------------------------------------------------------------------- */
/* ------------------------------ mqtt_task.h ------------------------------ */
/* ------------------------------------------------------------------------- */

#ifndef _MQTT_TASK_H_
#define _MQTT_TASK_H_

#define MQTT_EVENT_BASED        1
#define MQTT_FSM_BASED          0
#define MQTT_CODE               MQTT_FSM_BASED


/* ------------------------------------------------------------------------- */
/* ----------------------------- Custom  Types ----------------------------- */
/* ------------------------------------------------------------------------- */

typedef enum ___mqtt_state {
    MQTT_STATE_INIT = 0,
    MQTT_STATE_DISCONNECTED,
    MQTT_STATE_CONNECTED,
    MQTT_STATE_PUBLISH_DATA,
    MQTT_STATE_IDLE,
    MQTT_STATE_ERROR,
}MQTT_State_t;

typedef enum __mqtt_error{
    MQTT_ERR_NONE,
    MQTT_ERR_NO_CLIENT,
    MQTT_ERR_CONNECTION_FAILED,
    MQTT_ERR_SUBSCRIPTION_FAILED,
    MQTT_ERR_PUBLISH_FAILED,
    MQTT_ERR_DISCONNECTED,
} MQTT_Err_t;

typedef struct __mqtt_buffer {
    uint8_t buffer[1024];
    uint32_t len;
    HANDLE lock;
    bool is_dirty;
}MQTT_Buffer_t;


/* ------------------------------------------------------------------------- */
/* --------------------------- Tasks' Constants ---------------------------- */
/* ------------------------------------------------------------------------- */

#define UART_TASK_PRIORITY              10
#define UART_TASK_STACK_SIZE            2048
#define UART_TASK_NAME                  "UarttTask"
#define UART_TRACE_INDEX                10

#define MQTT_TASK_PRIORITY              1
#define MQTT_TASK_STACK_SIZE            2048
#define MQTT_TASK_NAME                  "MqttTask"
#define MQTT_TRACE_INDEX                11

#define MQTT_SERVER_ADDRESS             "21farmer.com"
#define MQTT_SERVER_PORT                1883

#define MQTT_NETWORK_AP_NAME            "internet.vodafone.net"
#define MQTT_NETWORK_USERNAME           ""
#define MQTT_NETWORK_PASSWORD           ""

#define MQTT_USER_KEY                   "rootTest"
#define MQTT_USER_PASSWORD              "rootPass"
#define MQTT_TOPIC                      "testTopic"

#define MQTT_SERVER_KEEPALIVE           120

#define MQTT_CLIENT_RECONNECT_INTERVAL  3000
#define MQTT_CLIENT_PUBLISH_INTERVAL    10000

#define MQTT_PUBLISH_PAYLOAD            "{\"client_id\":\"%s\", \"message_id\":%d, \"test_message\":%s}"
#define MQTT_TOPIC_QOS                  2

#define UART_CMD_PREFIX                 "send:"

/* ------------------------------------------------------------------------- */
/* ------------------------------ Tasks' APIs ------------------------------ */
/* ------------------------------------------------------------------------- */

void MQTT_Task(void * pData);
void UART_Task(void * pData);

/*  state handlers  */
void MQTT_Init_StateHandler(void * Copy_pvArgs);
void MQTT_Disconnected_StateHandler(void * Copy_pvArgs);
void MQTT_Connected_StateHandler(void * Copy_pvArgs);
void MQTT_Publish_StateHandler(void * pvArgs);
void MQTT_Idle_StateHandler(void * pvArgs);
void MQTT_Error_StateHandler(void * pvArgs);

/*  MQTT events     */
void MQTT_Connection_Callback(MQTT_Client_t * Copy_psMqttClient, void * Copy_pvArgs, MQTT_Connection_Status_t Copy_eConnStatus);
void MQTT_Subscribed_Callback(void * Copy_pvArgs, MQTT_Error_t Copy_eError);
void MQTT_Published_Callback(void * Copy_pvArgs, MQTT_Error_t Copy_eError);
void MQTT_Received_Callback(void * Copy_pvArgs, const char * Copy_psTopic, uint32_t Copy_u32Len);
void MQTT_ReceivedData_Callback(void * Copy_pvArgs, const uint8_t * Copy_pu8Data, uint16_t Copy_u16Len, MQTT_Flags_t Copy_eFlags);

void UART_RX_EventHandler(API_Event_t * pEvent);

/*  Network Events     */
void MQTT_NetworkRegistered_EventHandler(API_Event_t *pEvent);
void MQTT_NetworkAttached_EventHandler(API_Event_t * pEvent);
void MQTT_NetworkAttachFailed_EventHandler(API_Event_t * pEvent);
void MQTT_NetworkDeAttached_EventHandler(API_Event_t * pEvent);
void MQTT_NetworkActivated_EventHandler(API_Event_t * pEvent);
void MQTT_NetworkActivationFailed_EventHandler(API_Event_t * pEvent);
void MQTT_NetworkDeActivated_EventHandler(API_Event_t * pEvent);

void MQTT_StartPublishTimer(uint32_t u32WaitTime, MQTT_Client_t * psClient);
void MQTT_PublishTimerCallback(void * pvArgs);

#endif /*   _MQTT_TASK_H_   */

/* ------------------------------------------------------------------------- */
/* ------------------------------ End Of File ------------------------------ */
/* ------------------------------------------------------------------------- */
