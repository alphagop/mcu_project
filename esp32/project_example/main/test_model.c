#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

// 定义事件位（每个位代表一个独立事件）
#define NETWORK_CONNECTED_BIT  (1 << 0)  // 位0：网络连接成功
#define SENSOR_READY_BIT       (1 << 1)  // 位1：传感器就绪
#define SD_CARD_READY_BIT      (1 << 2)  // 位2：SD卡就绪
#define ALL_READY_BITS         (NETWORK_CONNECTED_BIT | SENSOR_READY_BIT | SD_CARD_READY_BIT)



// 定义全局变量
QueueHandle_t queue_handle;
SemaphoreHandle_t semaphore_handle;
SemaphoreHandle_t mutex_handle; 
EventGroupHandle_t system_events;

int shared_counter = 0;     // 共享计数器

// 定义队列消息结构体
typedef struct 
{
    int data;
}queue_message;

// Task A: 向队列发送数据
void task_A()
{
    queue_message msg;
    memset(&msg, 0, sizeof(msg));

    while (1)
    {
        xQueueSend(queue_handle, &msg, portMAX_DELAY);
        msg.data++;
        ESP_LOGI("Task A", "Sending data: %d", msg.data);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Task B: 从队列接收数据并打印
void task_B()
{
    queue_message msg;
    memset(&msg, 0, sizeof(msg));

    while (1)
    {
        if (xQueueReceive(queue_handle, &msg, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI("Task B", "Received data: %d", msg.data);
        }
    }
}

// Task C: 定时释放信号量
void task_C(){
    while(1){
        xSemaphoreGive(semaphore_handle);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Task D: 等待信号量并打印消息
void task_D(){
    while(1){
        if(xSemaphoreTake(semaphore_handle, portMAX_DELAY) == pdTRUE){
            ESP_LOGI("Task D", "Semaphore acquired!");
        }
    }
}   

// Task E: 使用互斥锁保护共享资源的增加操作
void task_E(){
    while(1){
        if(xSemaphoreTake(mutex_handle, portMAX_DELAY) == pdTRUE){
            int temp = shared_counter+1;
            vTaskDelay(pdMS_TO_TICKS(100)); // 模拟一些处理时间
            shared_counter = temp;
            ESP_LOGI("Task E", "Shared counter: %d", shared_counter);
            xSemaphoreGive(mutex_handle);
        }
        else{
            ESP_LOGI("Task E", "Failed to acquire mutex");
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Task F: 使用互斥锁保护共享资源的减少操作
void task_F(){
    while(1){
        if(xSemaphoreTake(mutex_handle, portMAX_DELAY) == pdTRUE){
            int temp = shared_counter-1;
            vTaskDelay(pdMS_TO_TICKS(100)); // 模拟一些处理时间
            shared_counter = temp;
            ESP_LOGI("Task F", "Shared counter: %d", shared_counter);
            xSemaphoreGive(mutex_handle);
        }
        else{
            ESP_LOGI("Task F", "Failed to acquire mutex");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Task G: 仅读取共享计数器（演示无锁读取的风险）
void task_G(void *pvParameter)
{
    while(1) {
        // 注意：这里故意不使用互斥锁来读取，可能读到不一致的数据
        ESP_LOGI("Task G", "读取计数器(无保护): %d", shared_counter);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


// Task H: 安全地读取共享计数器（使用互斥锁保护读取）
void task_H(void *pvParameter)
{
    while(1) {
        // 安全读取：获取锁后读取
        if(xSemaphoreTake(mutex_handle, pdMS_TO_TICKS(500)) == pdTRUE) {
            int safe_value = shared_counter;
            xSemaphoreGive(mutex_handle);
            
            ESP_LOGI("Task H", "安全读取计数器: %d", safe_value);
        }
        
        vTaskDelay(pdMS_TO_TICKS(400));
    }
}

// 网络连接任务
void network_task(void *pvParameter)
{
    // 模拟网络连接过程
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI("Network", "Network connected!");
    
    // 设置网络连接成功事件位
    xEventGroupSetBits(system_events, NETWORK_CONNECTED_BIT);
    
    vTaskDelete(NULL);
}

// 传感器初始化任务
void sensor_task(void *pvParameter)
{
    // 模拟传感器初始化
    vTaskDelay(pdMS_TO_TICKS(1500));
    ESP_LOGI("Sensor", "Sensor ready!");
    
    // 设置传感器就绪事件位
    xEventGroupSetBits(system_events, SENSOR_READY_BIT);
    
    vTaskDelete(NULL);
}

// SD卡初始化任务
void sd_card_task(void *pvParameter)
{
    // 模拟SD卡初始化
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI("SD Card", "SD card ready!");
    
    // 设置SD卡就绪事件位
    xEventGroupSetBits(system_events, SD_CARD_READY_BIT);
    
    vTaskDelete(NULL);
}

// 主函数: 创建队列和任务
void app_main(void)
{
    // queue_handle = xQueueCreate(10, sizeof(queue_message));
    // semaphore_handle = xSemaphoreCreateBinary();
    mutex_handle = xSemaphoreCreateMutex();
    if(mutex_handle == NULL) {
        ESP_LOGE("App Main", "Failed to create mutex!");
        return;
    }


    // xTaskCreatePinnedToCore(task_A, "Task A", 2048, NULL, 1, NULL, 0);
    // xTaskCreatePinnedToCore(task_B, "Task B", 2048, NULL, 1, NULL, 1);
    // xTaskCreatePinnedToCore(task_C, "Task C", 2048, NULL, 1, NULL, 0);
    // xTaskCreatePinnedToCore(task_D, "Task D", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(task_E, "Task E", 2048, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(task_F, "Task F", 2048, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(task_G, "Task G", 2048, NULL, 1, NULL, 0);  // 优先级1
    //xTaskCreatePinnedToCore(task_H, "Task H", 2048, NULL, 1, NULL, 1);  // 优先级1

    ESP_LOGI("App Main", "所有任务创建完成，系统启动");

    // 保持主任务运行
    while(1){
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
