#ifndef _F21_H_
#define _F21_H_

/* --------------------------- Tasks -------------------------------- */

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
#define GPS_TASK_STACK_SIZE     1024
#define GPS_TASK_NAME           "GpsTask"

#define GPRS_TASK_PRIORITY      6
#define GPRS_TASK_STACK_SIZE    1024
#define GPRS_TASK_NAME          "SmsTask"

#define MQTT_TASK_PRIORITY      7
#define MQTT_TASK_STACK_SIZE    1024
#define MQTT_TASK_NAME          "MqttTask"

/* ------------------------ task switches ------------------------------ */
#define ENABLE_GPIO_TASK
// #define ENABLE_UART_TASK
// #define ENABLE_DIAL_TASK
// #define ENABLE_CALL_RECEIVE_TASK
// #define ENABLE_SMS_SEND_TASK
// #define ENABLE_SMS_RECEIVE_TASK
// #define ENABLE_GPS_TASK
// #define ENABLE_GPRS_TASK
// #define ENABLE_MQTT_TASK

/* --------------------------- GPIO Task -------------------------------- */

#if defined (ENABLE_GPIO_TASK)

#define INDICATOR_LED_PIN       GPIO_PIN27
#define STATUS_LED_PIN          GPIO_PIN28
#define STATUS_UPDATE_PIN       GPIO_PIN25


#define ENABLE_INDICATOR_LED    
#define ENABLE_STATUS_UPDATE

#endif /*   ENABLE_GPIO_TASK    */

/* --------------------------- public APIs -------------------------------- */
void F21_Main(void * pData);
void F21MainTask(void * pData);
void EventDispatch(API_Event_t * pEvent);

#if defined (ENABLE_GPIO_TASK)
void GPIO_Task(void * pData);
#endif  /*  ENABLE_GPIO_TASK    */

#if defined (ENABLE_UART_TASK)
void UART_Task(void * pData);
#endif  /*  ENABLE_UART_TASK    */

#endif  /* _F21_H_  */