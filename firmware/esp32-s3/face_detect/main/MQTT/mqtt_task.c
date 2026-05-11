#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_task.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_event.h"
#include "spilcd.h"
#include "shared_data.h"
#include "cJSON.h"
#include "door.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const char *TAG = "MQTT_TASK";

#define MQTT_BROKER_URL         "mqtt://172.20.10.2:1883" // Broker 地址
#define MQTT_PUBLISH_TOPIC      "esp32/camera/image"          // 图片上传主题
#define MQTT_SUBSCRBE_TOPIC_FACE     "esp32/face/result"          // 结果接收主题
#define MQTT_SUBSCRBE_FACE_RESULT "esp32/go/openTheDoor" // 人脸为已注册用户
#define MQTT_CLIENT_ID          "ESP32-S3-Client"              // 客户端 ID

// 全局队列句柄定义
QueueHandle_t xMqttPublishQueue = NULL;

// MQTT 客户端句柄
static esp_mqtt_client_handle_t s_mqtt_client = NULL;

// 服务器回传消息解析
void parse_face_result(char *json_payload, int len) {
    // 字符串解析为json
    cJSON *root = cJSON_ParseWithLength(json_payload, len);
    if (!root) {
        ESP_LOGE(TAG, "JSON parse failed: %s", cJSON_GetErrorPtr());
        return;
    }
    cJSON *status_item = cJSON_GetObjectItem(root, "status");
    if (cJSON_IsString(status_item)) {
        ESP_LOGI(TAG, "Receive status: %s", status_item->valuestring);

        // 进行处理
        if (strcmp(status_item->valuestring, "0") == 0) {
            // 检测到人脸
            cJSON *bbox = cJSON_GetObjectItem(root, "bbox");
            if (cJSON_IsArray(bbox) && cJSON_GetArraySize(bbox) == 4) {
                // 进行锁操作
                if (xSemaphoreTake(xBoxMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    g_face_bbox[0] = cJSON_GetArrayItem(bbox, 0)->valueint;
                    g_face_bbox[1] = cJSON_GetArrayItem(bbox, 1)->valueint;
                    g_face_bbox[2] = cJSON_GetArrayItem(bbox, 2)->valueint;
                    g_face_bbox[3] = cJSON_GetArrayItem(bbox, 3)->valueint;
                    xSemaphoreGive(xBoxMutex);
                }
            }
        } else if (strcmp(status_item->valuestring, "1") == 0) {
            ESP_LOGW(TAG, "there is no face");
            // 进行锁操作
            if (xSemaphoreTake(xBoxMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                g_face_bbox[0] = -1;
                xSemaphoreGive(xBoxMutex);
            }
        }
    }
    cJSON_Delete(root);
}



// 事件处理函数
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        spilcd_show_string(0, 200, 240, 16, 16, "MQTT_EVENT_CONNECTED       ", RED);
        // 构建专属 topic
        char subTopic[64];
        sprintf(subTopic, "%s/%s", MQTT_SUBSCRBE_TOPIC_FACE, DEVICE_ID);
        // 可以在这里订阅主题，例如：
        esp_mqtt_client_subscribe(event->client, subTopic, 0);
        // 订阅第二个主题
        sprintf(subTopic, "%s/%s", MQTT_SUBSCRBE_FACE_RESULT, DEVICE_ID);
        esp_mqtt_client_subscribe(event->client, subTopic, 0);
        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        spilcd_show_string(0, 200, 240, 16, 16, "MQTT_EVENT_DISCONNECTED", RED);
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGD(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        // ESP_LOGI(TAG, "TOPIC=%.*s \r\nDATA=%.*s", event->topic_len, event->topic, event->data_len, event->data);
        char expectedTopic[64];
        sprintf(expectedTopic, "%s/%s", MQTT_SUBSCRBE_TOPIC_FACE, DEVICE_ID);
        if (strncmp(event->topic, expectedTopic, event->topic_len) == 0) {
            // 调用解析函数
            // event->data 是 char*，event->data_len 是长度
            parse_face_result(event->data, event->data_len);
            break;
        }
        sprintf(expectedTopic, "%s/%s", MQTT_SUBSCRBE_FACE_RESULT, DEVICE_ID);
        if (strncmp(event->topic, expectedTopic, event->topic_len) == 0) {
            trigger_door_open();
            break;
        }

        break;
        
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
        break;
        
    default:
        ESP_LOGI(TAG, "Other event id:%d", (int)event->event_id);
        break;
    }

}

void mqtt_task(void *pvParameters) {
    ESP_LOGI(TAG, "MQTT Task started on core %d", xPortGetCoreID());
    vTaskDelay(pdMS_TO_TICKS(500)); 
    // 创建队列
    // 队列深度为 3，表示最多缓存 3 张图片，防止内存溢出
    // 队列元素大小为指针大小
    xMqttPublishQueue = xQueueCreate(3, sizeof(mqtt_msg_t));
    if (xMqttPublishQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create MQTT queue");
        vTaskDelete(NULL);
        return;
    }
    esp_mqtt_client_config_t mqtt_cfg = {0}; 
    // 配置 MQTT 客户端
    mqtt_cfg.broker.address.uri = MQTT_BROKER_URL;
    mqtt_cfg.credentials.client_id = MQTT_CLIENT_ID;
    mqtt_cfg.network.timeout_ms = 10000;
    mqtt_cfg.network.reconnect_timeout_ms = 10000;
    mqtt_cfg.session.keepalive = 60;
    ESP_LOGI(TAG, "Initializing MQTT Client...");
    vTaskDelay(pdMS_TO_TICKS(500)); 
    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

    // 注册事件回调
    if (s_mqtt_client != NULL) {
        esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
        esp_mqtt_client_start(s_mqtt_client);
    } else {
        ESP_LOGE(TAG, "MQTT client init failed");
        vTaskDelete(NULL);
        return;
    }

    mqtt_msg_t msg;

    while (1)
    {
        if (xQueueReceive(xMqttPublishQueue, &msg, portMAX_DELAY) == pdTRUE) {
            if (NULL != msg.data && NULL != s_mqtt_client) {
                ESP_LOGI(TAG, "Sending image, size: %d bytes", msg.len);

                char pubTopic[64];
                sprintf(pubTopic, "%s/%s", MQTT_PUBLISH_TOPIC, DEVICE_ID);

                int msg_id = esp_mqtt_client_publish(s_mqtt_client, pubTopic, (const char*)msg.data, msg.len, 1, 0);

                if (msg_id >= 0) {
                    ESP_LOGD(TAG, "Publish queued, msg_id=%d", msg_id);
                } else {
                    ESP_LOGE(TAG, "Publish failed");
                }

                // 摄像头端不需要自己释放
                free(msg.data);
            }
        }
    }
    
}
