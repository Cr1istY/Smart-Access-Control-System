#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <stdint.h> // 用于 uint16_t 等类型

// 1. 声明外部的互斥锁句柄
extern SemaphoreHandle_t xBoxMutex;

extern SemaphoreHandle_t xFaceDetectedSignal;

// 2. 声明外部的全局坐标数组
extern int g_face_bbox[4];

extern uint8_t error_count;

// 3. 声明初始化函数
void shared_data_init(void);

#endif // SHARED_DATA_H