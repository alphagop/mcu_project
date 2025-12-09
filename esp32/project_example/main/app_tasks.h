#ifndef __APP_TASKS_H__
#define __APP_TASKS_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 任务函数声明
void task_system_monitor(void *pvParameter);
void task_data_processor(void *pvParameter);
void task_sensor_reader(void *pvParameter);
void task_network_manager(void *pvParameter);

// 任务初始化函数
esp_err_t app_tasks_init(void);

#endif // __APP_TASKS_H__