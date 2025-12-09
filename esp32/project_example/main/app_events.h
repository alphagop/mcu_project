#ifndef __APP_EVENTS_H__
#define __APP_EVENTS_H__

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

// 消息类型枚举
typedef enum {
    MSG_TYPE_SENSOR_DATA = 0,
    MSG_TYPE_NETWORK_CMD,
    MSG_TYPE_SYSTEM_EVENT,
    MSG_TYPE_USER_INPUT
} message_type_t;

// 通用消息结构
typedef struct {
    message_type_t type;
    uint32_t timestamp;
    union {
        struct {
            int16_t temperature;
            int16_t humidity;
            uint16_t pressure;
        } sensor_data;
        
        struct {
            uint8_t command;
            uint8_t parameter;
        } network_cmd;
        
        struct {
            uint32_t event_id;
            void* event_data;
        } system_event;
    } payload;
} data_message_t;

// 全局事件组句柄声明
extern EventGroupHandle_t g_app_events;

#endif // __APP_EVENTS_H__