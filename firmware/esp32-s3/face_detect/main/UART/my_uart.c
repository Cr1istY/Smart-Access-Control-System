#include "driver/uart.h"
#include "driver/uart_select.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "shared_data.h"
#include "cJSON.h"
#include "led.h"
#include "esp_log.h"
#include "esp_err.h"
#include "door.h"
#include "my_uart.h"
#include "mqtt_task.h"


void parse_command(char *input)
{
    char *separator = strchr(input, ':');

    if (separator != NULL) {
        char *key = input;       // 前半段是 "CARD"
        char *value = separator + 1; // 后半段是 "OK" (注意：可能包含换行符)

        *separator = '\0'; 

        // 去除 value 末尾可能存在的换行符 (\n) 或回车符 (\r)
        size_t len = strlen(value);
        while (len > 0 && (value[len-1] == '\n' || value[len-1] == '\r')) {
            value[--len] = '\0'; 
        }

        ESP_LOGI(UART_TAG, "解析成功 -> 指令: %s, 内容: %s", key, value);

        // RFID
        if (strcmp(key, "CARD") == 0) {
            ESP_LOGI(UART_TAG, "刷卡");
            // 发送数据给语音模块
            if (strcmp(value, "OK") == 0) {
                ESP_LOGI(UART_TAG, "刷卡成功");  
                uart_write_bytes(UART_VOICE_ID, "<G>刷卡成功", strlen("<G>刷卡成功"));
                trigger_door_open();
            } else {
                uart_write_bytes(UART_VOICE_ID, "<G>请检查卡", strlen("<G>请检查卡"));
                error_count++;
            }
        } 
        // 指纹
        else if (strcmp(key, "FP") == 0) {
            if (strcmp(value, "OK") == 0) {  
                uart_write_bytes(UART_VOICE_ID, "<G>指纹识别成功", strlen("<G>指纹识别成功"));
                trigger_door_open();
            } else {
                uart_write_bytes(UART_VOICE_ID, "<G>请重试", strlen("<G>请重试"));
                error_count++;
            }
        }
        // 键盘密码
        else if (strcmp(key, "PASSWORD") == 0) {
            if (strcmp(value, "OK") == 0) {  
                uart_write_bytes(UART_VOICE_ID, "<G>密码正确", strlen("<G>密码正确"));
                trigger_door_open();
            } else {
                uart_write_bytes(UART_VOICE_ID, "<G>密码错误", strlen("<G>密码错误"));
                error_count++;
            }
        }
        else if (strcmp(key, "ERROR") == 0) {
             uart_write_bytes(UART_VOICE_ID, "<G>识别失败", strlen("<G>识别失败"));
             error_count++;
        }
        else {
            ESP_LOGW(UART_TAG, "未知指令: %s", key);
        }
    } else {
        ESP_LOGW(UART_TAG, "格式错误，未找到冒号: %s", input);
    }

    // 警告
    if (error_count >= 5) {
        send_alert(0);
        uart_write_bytes(UART_VOICE_ID, "<G>非法闯入报警", strlen("<G>非法闯入报警"));
        vTaskDelay(pdMS_TO_TICKS(2000));
        uart_write_bytes(UART_VOICE_ID, "<G>非法闯入报警", strlen("<G>非法闯入报警"));
        vTaskDelay(pdMS_TO_TICKS(2000));
        error_count = 0;
    }
}

// uart 初始化
void uart_init(uint32_t stm_baudrate, uint32_t voice_baudrate) {

    // stm32接口 只需要收
    const uart_config_t uart_stm32_config = {
        .baud_rate = stm_baudrate,          // 波特率需与 STM32 一致
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // 挂载
    ESP_ERROR_CHECK(uart_param_config(UART_STM32_ID, &uart_stm32_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_STM32_ID, UART_PIN_NO_CHANGE, UART_STM32_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    esp_err_t ret1 = uart_driver_install(UART_STM32_ID, RX_BUF_SIZE, MIN_BUF_SIZE, 10, &uart_queue, 0);
    if (ret1 != ESP_OK) {
        ESP_LOGE(UART_TAG, "STM32 UART 驱动安装失败: %s", esp_err_to_name(ret1));
    }
    ESP_ERROR_CHECK(uart_set_rx_timeout(UART_STM32_ID, 10));

    // 音频接口 只需要发
    const uart_config_t uart_voice_config = {
        .baud_rate = voice_baudrate,            // 语音模块通常默认 9600
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_param_config(UART_VOICE_ID, &uart_voice_config);
    uart_set_pin(UART_VOICE_ID, UART_VOICE_TX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    ret1 = uart_driver_install(UART_VOICE_ID, MIN_BUF_SIZE, BUF_SIZE, 0, NULL, 0);
    if (ret1 != ESP_OK) {
        ESP_LOGE(UART_TAG, "VOICE UART 驱动安装失败: %s", esp_err_to_name(ret1));
    }

    ESP_LOGI(UART_TAG, "初始化完成! RX=%d, TX=%d", UART_STM32_RX_PIN, UART_VOICE_TX_PIN);
}


void uart_stm32_task(void *pvParameters) {
    char *data = (char *) malloc(BUF_SIZE); // 用于接收stm32发送的消息
    if (data == NULL) {
        ESP_LOGE(UART_TAG, "Malloc failed");
        vTaskDelete(NULL);
        return;
    }
    uart_event_t event;
    ESP_LOGI(UART_TAG, "uart_stm32_task start");
    while (1) {
        if (xQueueReceive(uart_queue, (void *)&event, portMAX_DELAY)) {
            
            // 清零缓冲区，防止上次残留数据干扰
            memset(data, 0, BUF_SIZE);

            switch (event.type) {
                case UART_DATA:
                    uint16_t len = uart_read_bytes(UART_STM32_ID, data, BUF_SIZE - 1, pdMS_TO_TICKS(10));
                    
                    if (len > 0) {
                        data[len] = '\0';
                        ESP_LOGI(UART_TAG, "收到 [%d]: %s", len, (char*)data);
                        
                        parse_command((char*)data);
                    }
                    break;

                case UART_FIFO_OVF:
                    ESP_LOGW(UART_TAG, "硬件 FIFO 溢出，数据可能丢失");
                    uart_flush_input(UART_STM32_ID);
                    xQueueReset(uart_queue); // 重置队列防止错误累积
                    break;

                case UART_BUFFER_FULL:
                    ESP_LOGW(UART_TAG, "环形缓冲区满，应用层处理太慢");
                    uart_flush_input(UART_STM32_ID);
                    xQueueReset(uart_queue);
                    break;
                
                case UART_BREAK:
                    ESP_LOGI(UART_TAG, "收到 Break 信号");
                    break;
                    
                default:
                    break;
            }
        }
    }
    free(data);
    uart_driver_delete(UART_STM32_ID);
    vTaskDelete(NULL);  
}