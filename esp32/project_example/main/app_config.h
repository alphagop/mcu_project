#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 调试配置
#define APP_DEBUG_ENABLED       1
#define APP_LOG_LEVEL           ESP_LOG_INFO

// 任务配置
#define TASK_MANAGER_PRIORITY    (tskIDLE_PRIORITY + 3)
#define DATA_PROCESSOR_PRIORITY  (tskIDLE_PRIORITY + 2)
#define SENSOR_READER_PRIORITY   (tskIDLE_PRIORITY + 1)

#define TASK_STACK_DEPTH        4096

// 队列配置
#define QUEUE_LENGTH             10
#define QUEUE_ITEM_SIZE         sizeof(data_message_t)

// 事件组位定义
typedef enum {
    EVENT_NETWORK_READY     = (1 << 0),
    EVENT_SENSOR_READY      = (1 << 1),
    EVENT_SDCARD_READY      = (1 << 2),
    EVENT_SYSTEM_READY      = (EVENT_NETWORK_READY | EVENT_SENSOR_READY),
    EVENT_ALL_READY         = (EVENT_NETWORK_READY | EVENT_SENSOR_READY | EVENT_SDCARD_READY)
} app_event_bits_t;

#endif // __APP_CONFIG_H__