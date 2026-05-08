#include "shared_data.h"

// 定义互斥锁句柄
SemaphoreHandle_t xBoxMutex = NULL;

// 定义全局坐标数组
int g_face_bbox[4] = {-1, -1, -1, -1};

// 实现初始化函数
void shared_data_init(void) {
    xBoxMutex = xSemaphoreCreateMutex();
    if (xBoxMutex == NULL) {
        // ESP_LOGE(TAG, "互斥锁创建失败");
    }
}