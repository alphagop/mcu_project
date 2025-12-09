#include <stdio.h>
#include <string.h>
#include <inttypes.h>  // 添加用于PRIu32格式宏
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "app_config.h"
#include "app_events.h"
#include "app_tasks.h"

// 全局句柄定义
EventGroupHandle_t g_app_events = NULL;
QueueHandle_t g_data_queue = NULL;
SemaphoreHandle_t g_i2c_mutex = NULL;

static const char *TAG = "AppMain";

// 系统监控任务
void task_system_monitor(void *pvParameter)
{
    ESP_LOGI(TAG, "System monitor task started");
    
    while (1) {
        // 等待系统就绪事件
        EventBits_t bits = xEventGroupWaitBits(
            g_app_events,
            EVENT_SYSTEM_READY,
            pdTRUE,        // 清除等待的位
            pdTRUE,        // 等待所有位
            portMAX_DELAY
        );
        
        if ((bits & EVENT_SYSTEM_READY) == EVENT_SYSTEM_READY) {
            ESP_LOGI(TAG, "System fully ready! Starting normal operation");
            
            // 系统就绪后的主循环
            while (1) {
                // 监控系统状态 - 修复格式符
                uint32_t heap_free = esp_get_free_heap_size();
                uint32_t min_free = esp_get_minimum_free_heap_size();
                
                // 将%d改为正确的PRIu32用于uint32_t类型
                ESP_LOGI(TAG, "Heap: free=%"PRIu32", min_free=%"PRIu32, heap_free, min_free);
                
                vTaskDelay(pdMS_TO_TICKS(10000)); // 每10秒报告一次
            }
        }
    }
}

// 数据处理任务（演示队列使用）
void task_data_processor(void *pvParameter)
{
    ESP_LOGI(TAG, "Data processor task started");
    
    data_message_t received_msg;
    
    while (1) {
        if (xQueueReceive(g_data_queue, &received_msg, portMAX_DELAY) == pdTRUE) {
            // 使用互斥锁保护共享资源访问（如果需要）
            if (xSemaphoreTake(g_i2c_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                
                switch (received_msg.type) {
                    case MSG_TYPE_SENSOR_DATA:
                        // 修复：使用正确的格式符打印整数
                        ESP_LOGI(TAG, "Processing sensor data: temp=%d, hum=%d", 
                                received_msg.payload.sensor_data.temperature,
                                received_msg.payload.sensor_data.humidity);
                        break;
                        
                    case MSG_TYPE_NETWORK_CMD:
                        // 修复：使用正确的格式符打印整数
                        ESP_LOGI(TAG, "Processing network command: cmd=%d, param=%d",
                                received_msg.payload.network_cmd.command,
                                received_msg.payload.network_cmd.parameter);
                        break;
                        
                    default:
                        ESP_LOGW(TAG, "Unknown message type: %d", received_msg.type);
                        break;
                }
                
                xSemaphoreGive(g_i2c_mutex);
            }
        }
    }
}

// 传感器读取任务（演示互斥锁使用）
void task_sensor_reader(void *pvParameter)
{
    ESP_LOGI(TAG, "Sensor reader task started");
    
    data_message_t sensor_msg = {
        .type = MSG_TYPE_SENSOR_DATA,
        .timestamp = 0
    };
    
    // 模拟传感器初始化
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "Sensors initialized");
    xEventGroupSetBits(g_app_events, EVENT_SENSOR_READY);
    
    while (1) {
        if (xSemaphoreTake(g_i2c_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
            // 模拟读取传感器数据
            sensor_msg.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
            sensor_msg.payload.sensor_data.temperature = 25 + (rand() % 10);
            sensor_msg.payload.sensor_data.humidity = 40 + (rand() % 30);
            sensor_msg.payload.sensor_data.pressure = 1000 + (rand() % 50);
            
            // 发送到数据处理队列
            if (xQueueSend(g_data_queue, &sensor_msg, pdMS_TO_TICKS(100)) != pdTRUE) {
                ESP_LOGE(TAG, "Failed to send sensor data to queue");
            }
            
            xSemaphoreGive(g_i2c_mutex);
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // 每2秒读取一次
    }
}

// 网络管理任务（演示事件组使用）
void task_network_manager(void *pvParameter)
{
    ESP_LOGI(TAG, "Network manager task started");
    
    // 模拟网络连接过程
    for (int i = 3; i > 0; i--) {
        ESP_LOGI(TAG, "Network connecting in %d...", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGI(TAG, "Network connected!");
    xEventGroupSetBits(g_app_events, EVENT_NETWORK_READY);
    
    data_message_t network_msg = {
        .type = MSG_TYPE_NETWORK_CMD,
        .timestamp = 0
    };
    
    uint8_t command_counter = 0;
    
    while (1) {
        network_msg.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
        network_msg.payload.network_cmd.command = command_counter++;
        network_msg.payload.network_cmd.parameter = rand() % 100;
        
        // 发送网络命令到队列
        if (xQueueSend(g_data_queue, &network_msg, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGE(TAG, "Failed to send network command to queue");
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000)); // 每5秒发送一个命令
    }
}

// 任务初始化函数
esp_err_t app_tasks_init(void)
{
    esp_err_t ret = ESP_OK;
    
    // 创建事件组
    g_app_events = xEventGroupCreate();
    if (g_app_events == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_FAIL;
    }
    
    // 创建数据队列
    g_data_queue = xQueueCreate(QUEUE_LENGTH, sizeof(data_message_t));
    if (g_data_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create data queue");
        return ESP_FAIL;
    }
    
    // 创建互斥锁
    g_i2c_mutex = xSemaphoreCreateMutex();
    if (g_i2c_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create I2C mutex");
        return ESP_FAIL;
    }
    
    // 创建系统监控任务
    if (xTaskCreate(task_system_monitor, "SystemMonitor", 
                    TASK_STACK_DEPTH, NULL, TASK_MANAGER_PRIORITY, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create system monitor task");
        return ESP_FAIL;
    }
    
    // 创建数据处理任务
    if (xTaskCreate(task_data_processor, "DataProcessor", 
                    TASK_STACK_DEPTH, NULL, DATA_PROCESSOR_PRIORITY, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create data processor task");
        return ESP_FAIL;
    }
    
    // 创建传感器读取任务
    if (xTaskCreate(task_sensor_reader, "SensorReader", 
                    TASK_STACK_DEPTH, NULL, SENSOR_READER_PRIORITY, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sensor reader task");
        return ESP_FAIL;
    }
    
    // 创建网络管理任务
    if (xTaskCreate(task_network_manager, "NetworkManager", 
                    TASK_STACK_DEPTH, NULL, SENSOR_READER_PRIORITY, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create network manager task");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "All tasks created successfully");
    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Application starting...");
    
    // 初始化所有任务和RTOS对象
    if (app_tasks_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize application tasks");
        return;
    }
    
    ESP_LOGI(TAG, "Application started successfully");
    
    // 主任务可以进入低功耗模式或处理其他事务
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000)); // 减少主任务负载
    }
}