/* ------------------------------------------------------------------------- */
/* --------------------------------- F21.h --------------------------------- */
/* ------------------------------------------------------------------------- */

#ifndef _F21_H_
#define _F21_H_

/* ------------------------------------------------------------------------- */
/* --------------------------- Tasks' Constants ---------------------------- */
/* ------------------------------------------------------------------------- */

#define MAIN_TASK_PRIORITY      0
#define MAIN_TASK_STACK_SIZE    1024
#define MAIN_TASK_NAME          "MainTask"

#define GPIO_TASK_PRIORITY      1
#define GPIO_TASK_STACK_SIZE    1024
#define GPIO_TASK_NAME          "GpioTask"

#define UART_TASK_PRIORITY      2
#define UART_TASK_STACK_SIZE    1024
#define UART_TASK_NAME          "UartTask"

#define CALL_TASK_PRIORITY      3
#define CALL_TASK_STACK_SIZE    1024
#define CALL_TASK_NAME          "CallTask"

#define SMS_TASK_PRIORITY       4
#define SMS_TASK_STACK_SIZE     1024
#define SMS_TASK_NAME           "SmsTask"

#define GPS_TASK_PRIORITY       5
#define GPS_TASK_STACK_SIZE     2048
#define GPS_TASK_NAME           "GpsTask"

#define GPRS_TASK_PRIORITY      6
#define GPRS_TASK_STACK_SIZE    1024
#define GPRS_TASK_NAME          "SmsTask"

#define MQTT_TASK_PRIORITY      7
#define MQTT_TASK_STACK_SIZE    1024
#define MQTT_TASK_NAME          "MqttTask"

/* ------------------------------------------------------------------------- */
/* ---------------------------- Tasks' Switches ---------------------------- */
/* ------------------------------------------------------------------------- */

// #define ENABLE_GPIO_TASK
// #define ENABLE_UART_TASK
// #define ENABLE_DIAL_TASK
// #define ENABLE_CALL_RECEIVE_TASK
#define ENABLE_SMS_TASK
// #define ENABLE_GPS_TASK
// #define ENABLE_GPRS_TASK
// #define ENABLE_MQTT_TASK


/* ----------------------------- GPIO Task Conf ---------------------------- */

#if defined (ENABLE_GPIO_TASK)

#define INDICATOR_LED_PIN       GPIO_PIN27
#define STATUS_LED_PIN          GPIO_PIN28
#define STATUS_UPDATE_PIN       GPIO_PIN25


#define ENABLE_INDICATOR_LED    
#define ENABLE_STATUS_UPDATE

#endif /*   ENABLE_GPIO_TASK    */

/* ----------------------------- UART Task conf ---------------------------- */

// #define ENABLE_UART_EVENTS

/* ----------------------------- SMS Task conf ----------------------------- */

#if defined(ENABLE_SMS_EVENTS)



#endif  /*  ENABLE_SMS_EVENTS   */

/* ------------------------------------------------------------------------- */
/* ------------------------------ Tasks' APIs ------------------------------ */
/* ------------------------------------------------------------------------- */

/* ------------------------------- GPIO APIs ------------------------------- */

void F21_Main(void * pData);
void F21MainTask(void * pData);
void EventDispatch(API_Event_t * pEvent);

#if defined (ENABLE_GPIO_TASK)

void GPIO_Task(void * pData);

#endif  /*  ENABLE_GPIO_TASK    */

/* ------------------------------- UART APIs ------------------------------- */

#if defined (ENABLE_UART_TASK)

void UART_Task(void * pData);
void UART_ErrorCallback(UART_Error_t error);

#if defined (ENABLE_UART_EVENTS)
void UART_RX_EventHandler(API_Event_t *pEvent);
#else
void UART_RxCallback(UART_Callback_Param_t param);
#endif /*   !ENABLE_UART_EVENTS */

#endif  /*  ENABLE_UART_TASK    */

/* ------------------------------- GPS  APIs ------------------------------- */

#if defined(ENABLE_GPS_TASK)

void GPS_Task(void *pData);

#if !defined(ENABLE_GPS_EVENTS)
void GPS_Callback(UART_Callback_Param_t param);
#endif  /*  !ENABLE_GPS_EVENTS   */

#endif  /*  ENABLE_GPS_TASK */

/* ------------------------------- SMS  APIs ------------------------------- */

#if defined(ENABLE_SMS_TASK)

void SMS_Task(void * pData);
void SMS_UART_RX_EventHandler(API_Event_t *pEvent);

#endif /*   ENABLE_SMS_TASK   */

#endif  /* _F21_H_  */

/* ------------------------------------------------------------------------- */
/* ------------------------------ End Of File ------------------------------ */
/* ------------------------------------------------------------------------- */

