#ifndef DOOR_H
#define DOOR_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h" // 包含信号量/互斥锁头文件
#include "driver/gpio.h"
#include "esp_log.h"

#define GPIO_DOOR_LOCK GPIO_NUM_10
#define LOCK_OPEN      1
#define LOCK_CLOSE     0

static const char *DOOR_TAG = "DOOR_CTRL";

void door_system_init(void);
void door_task(void *pvParameters);
void trigger_door_open(void);

#endif