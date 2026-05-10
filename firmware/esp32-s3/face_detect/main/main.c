#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "led.h"
#include "my_spi.h"
#include "myiic.h"
#include "spilcd.h"
#include "esp_log.h"
#include "camera_config.h"
#include "wifi_config.h"
#include "shared_data.h"
#include "mqtt_task.h"
#include "camera_streamer.h"
#include "esp_heap_caps.h"
#include <stdio.h>

#define TASK_STACK_SIZE 4096

/**
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void)
{
    esp_err_t ret;
    uint8_t key = 0;

    ret = nvs_flash_init();     /* 初始化NVS */
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    led_init();                 /* LED初始化 */
    my_spi_init();              /* SPI初始化 */ 
    myiic_init();               /* MYIIC初始化 */
    xl9555_init();              /* XL9555初始化 */
    spilcd_init();              /* SPILCD初始化 */
    init_camera();              /* 初始化摄像头 */
    vTaskDelay(pdMS_TO_TICKS(2000));

    wifi_sta_init();
    ESP_LOGI("main", "wifi success");

    shared_data_init();

    xTaskCreatePinnedToCore(&mqtt_task, "mqtt_task", 6144, NULL, 3, NULL, 1);

    xTaskCreatePinnedToCore(&display_task, "display_task", TASK_STACK_SIZE, NULL, 5, NULL, 0);

    xTaskCreatePinnedToCore(&upload_task, "upload_task", TASK_STACK_SIZE, NULL, 3, NULL, 1);

    spilcd_show_string(0, 200, 240, 16, 16, "Task Create success", RED);

    while(1)
    {   
        // ESP_LOGI(TAG, "main check");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
