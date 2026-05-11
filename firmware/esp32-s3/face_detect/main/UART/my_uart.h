#ifndef MY_UART_H
#define MY_UART_H

static const char *UART_TAG = "UART";

#define UART_STM32_RX_PIN   GPIO_NUM_3  // 接收 STM32 指令
#define UART_VOICE_TX_PIN   GPIO_NUM_46   // 发送文字给语音模块

#define UART_STM32_ID       UART_NUM_1
#define UART_VOICE_ID       UART_NUM_2

#define BUF_SIZE            1024 // 给 STM32 用
#define RX_BUF_SIZE (BUF_SIZE * 2)
#define MIN_BUF_SIZE 512 // 最小字节

static QueueHandle_t uart_queue;

void parse_command(char *input);
void uart_init(uint32_t stm_baudrate, uint32_t voice_baudrate);
void uart_stm32_task(void *pvParameters);


#endif