#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_task.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_event.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "MQTT_TASK";

#define MQTT_BROKER_URL         "mqtt://172.20.10.2:1883" // Broker 地址
#define MQTT_PUBLISH_TOPIC      "esp32/camera/image"          // 图片上传主题
#define MQTT_SUBSCRBE_TOPIC     "esp32/face/result"          // 结果接收主题
#define MQTT_CLIENT_ID          "ESP32-S3-Client"           // 客户端 ID

// 全局队列句柄定义
QueueHandle_t xMqttPublishQueue = NULL;

// MQTT 客户端句柄
static esp_mqtt_client_handle_t s_mqtt_client = NULL;

// 事件处理函数
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // 可以在这里订阅主题，例如：
        // msg_id = esp_mqtt_client_subscribe(client, "esp32/cmd", 0);
        // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
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
        ESP_LOGI(TAG, "TOPIC=%.*s \r\nDATA=%.*s", event->topic_len, event->topic, event->data_len, event->data);
        // 在这里处理接收到的指令
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
    // 创建队列
    // 队列深度为 3，表示最多缓存 3 张图片，防止内存溢出
    // 队列元素大小为指针大小
    xMqttPublishQueue = xQueueCreate(3, sizeof(mqtt_msg_t));
    if (xMqttPublishQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create MQTT queue");
        vTaskDelete(NULL);
        return;
    }
    // 配置 MQTT 客户端
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URL,
        .credentials.client_id = MQTT_CLIENT_ID,
        // .credentials.username = "username",
        // .credentials.authentication.password = "password",
        .network.timeout_ms = 10000, // 网络超时
        .session.keepalive = 60      // 心跳包间隔
    };

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
                int msg_id = esp_mqtt_client_publish(s_mqtt_client, MQTT_PUBLISH_TOPIC, (const char*)msg.data, msg.len, 1, 0);

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
