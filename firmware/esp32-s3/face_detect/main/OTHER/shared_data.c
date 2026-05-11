#include "shared_data.h"

// 定义互斥锁句柄
SemaphoreHandle_t xBoxMutex = NULL;

// 定义全局坐标数组
int g_face_bbox[4] = {-1, -1, -1, -1};

uint8_t error_count = 0;

SemaphoreHandle_t xFaceDetectedSignal = NULL;

// 实现初始化函数
void shared_data_init(void) {
    xBoxMutex = xSemaphoreCreateMutex();
    if (xBoxMutex == NULL) {
        ESP_LOGE("SHARED_DATA", "互斥锁创建失败");
    }
}