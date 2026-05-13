#include "door.h"
#include "shared_data.h" // 假设这里有 error_count 定义

// 1. 全局任务句柄
static TaskHandle_t door_task_handle = NULL;
static SemaphoreHandle_t s_door_mutex = NULL;


void door_system_init(void) {
    s_door_mutex = xSemaphoreCreateMutex();
    if (s_door_mutex == NULL) {
        ESP_LOGE(DOOR_TAG, "创建门锁互斥锁失败！");
        return;
    }

    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << GPIO_DOOR_LOCK),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    
    // 初始状态设为关门
    gpio_set_level(GPIO_DOOR_LOCK, LOCK_CLOSE);
    
    ESP_LOGI(DOOR_TAG, "门锁系统初始化完成");
}

void door_task(void *pvParameters) {
    ESP_LOGI(DOOR_TAG, ">>> 任务开始：正在开门");
    gpio_set_level(GPIO_DOOR_LOCK, LOCK_OPEN);
    
    // 重置错误计数
    #ifdef error_count
    error_count = 0;
    #endif

    // 延时 5 秒
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    ESP_LOGI(DOOR_TAG, "<<< 任务结束：正在关门");
    gpio_set_level(GPIO_DOOR_LOCK, LOCK_CLOSE);

    // --- 关键修复：使用不带参数的临界区 ---
    // 这会自动处理多核锁，非常安全
    if (xSemaphoreTake(s_door_mutex, portMAX_DELAY) == pdTRUE) {
        door_task_handle = NULL;
        xSemaphoreGive(s_door_mutex);
    }

    // 删除当前任务
    vTaskDelete(NULL);
}

void trigger_door_open(void) {
    ESP_LOGE(DOOR_TAG, "创建开门任务");
    bool is_running = false;
    // --- 关键修复：使用临界区保护读取 ---
    if (xSemaphoreTake(s_door_mutex, portMAX_DELAY) == pdTRUE) {
        is_running = (door_task_handle != NULL);
        xSemaphoreGive(s_door_mutex);
    }

    if (is_running) {
        ESP_LOGW(DOOR_TAG, "门正在动作中，忽略本次触发");
        return;
    }

    // 创建任务并固定在 Core 0
    // 注意：xTaskCreatePinnedToCore 内部是线程安全的，不需要加锁
    BaseType_t ret = xTaskCreatePinnedToCore(
        door_task, 
        "door_action", 
        2048, 
        NULL, 
        3,             // 优先级 3
        &door_task_handle, 
        0              // 固定在 Core 0
    );

    if (ret != pdPASS) {
        ESP_LOGE(DOOR_TAG, "创建开门任务失败！");
    }
}