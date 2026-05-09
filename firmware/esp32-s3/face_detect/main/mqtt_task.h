#ifndef MQTT_TASK_H
#define MQTT_TASK_H

#define DEVICE_ID "device_001" 

typedef struct {
    uint8_t *data;
    size_t len;
} mqtt_msg_t;

// 定义用于传递 JPEG 数据指针的队列
// 队列中存放的是 mqtt_msg_t
extern QueueHandle_t xMqttPublishQueue;

void mqtt_task(void *pvParameters);

#endif