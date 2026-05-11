#include "driver/uart.h"
#include "driver/uart_select.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "esp_log.h"
#include "esp_err.h"
#include "my_uart.h"


void parse_command(char *input)
{
    char *separator = strchr(input, ':');

    if (separator != NULL) {
        *separator = '\0'; // 把冒号变成字符串结束符，切断前半段
        char *key = input;       // 前半段是 "CARD"
        char *value = separator + 1; // 后半段是 "OK" (注意：可能包含换行符)

        // 去除 value 末尾可能存在的换行符 (\n) 或回车符 (\r)
        size_t len = strlen(value);
        if (len > 0 && (value[len-1] == '\n' || value[len-1] == '\r')) {
            value[len-1] = '\0';
        }

        ESP_LOGI(UART_TAG, "解析成功 -> 指令: %s, 内容: %s", key, value);

        if (strcmp(key, "CARD") == 0) {
            // 发送数据给语音模块
            if (strcmp(value, "OK") == 0) {  
                uart_write_bytes(UART_VOICE_ID, "刷卡成功", strlen("刷卡成功"));
            }
        } 
        else if (strcmp(key, "ERROR") == 0) {
             uart_write_bytes(UART_VOICE_ID, "识别失败", strlen("识别失败"));
        }
        else {
            ESP_LOGW(UART_TAG, "未知指令: %s", key);
        }
    } else {
        ESP_LOGW(UART_TAG, "格式错误，未找到冒号: %s", input);
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
    esp_err_t ret1 = uart_driver_install(UART_STM32_ID, BUF_SIZE, MIN_BUF_SIZE, 0, NULL, 0);
    if (ret1 != ESP_OK) {
        ESP_LOGE(UART_TAG, "STM32 UART 驱动安装失败: %s", esp_err_to_name(ret1));
    }


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
    uint16_t len = 0;
    ESP_LOGI(UART_TAG, "uart_stm32_task start");
    while (1)
    {
        uart_get_buffered_data_len(UART_STM32_ID, (size_t*) &len);
        if (len > 0) {
            uart_read_bytes(UART_STM32_ID, data, len, 100);
            data[len] = '\0'; // 加上字符串结束符
            ESP_LOGI(UART_TAG, "收到: %s", data);
            parse_command(data);
        }
        memset(data, 0, 10);
        vTaskDelay(pdMS_TO_TICKS(10));
        uart_write_bytes(UART_VOICE_ID, "刷卡成功", strlen("刷卡成功"));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}