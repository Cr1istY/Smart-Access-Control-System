#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_server.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "spilcd.h"
#include "camera_streamer.h"
#include "esp_task_wdt.h"
#include "shared_data.h"
#include "mqtt_task.h"

static const char* TAG = "camera_streamer";

/**
 * @brief HTTP 视频流处理函数
 * 这是核心函数，负责建立连接并源源不断地发送 JPEG 图片
 */
static esp_err_t stream_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;

    // 1. 设置 MJPEG 响应头
    const char* content_type = "multipart/x-mixed-replace; boundary=123456789000000000000102";
    httpd_resp_set_type(req, content_type);

    // 2. 进入死循环，不断发送图片
    while (1) {
        // 获取一帧图像 (从 camera_config 中获取)
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            // 获取失败延时一下，防止死循环卡死 CPU
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        // 3. 构造 HTTP 数据包头部
        char header_buf[128];
        int header_len = snprintf(header_buf, sizeof(header_buf),
                                  "--123456789000000000000102\r\n"
                                  "Content-Type: image/jpeg\r\n"
                                  "Content-Length: %u\r\n\r\n",
                                  fb->len);

        // 4. 发送头部
        res = httpd_resp_send_chunk(req, header_buf, header_len);
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Send header failed");
            esp_camera_fb_return(fb);
            break;
        }

        // 5. 发送图片数据 (JPEG 二进制流)
        res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Send frame failed");
            esp_camera_fb_return(fb);
            break;
        }

        // 6. 发送帧结束符
        res = httpd_resp_send_chunk(req, "\r\n", 2);
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Send end frame failed");
            esp_camera_fb_return(fb);
            break;
        }

        // 7. 释放帧缓冲，归还给驱动，以便下一次采集
        esp_camera_fb_return(fb);
    }

    return res;
}

/**
 * @brief 启动 HTTP 服务器
 */
void start_camera_stream_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 2; // 我们只需要一个 /stream 接口

    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL
    };

    httpd_handle_t stream_httpd = NULL;
    
    // 启动服务器
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        ESP_LOGI(TAG, "Camera Stream Server started on http://%s/stream", "ESP32_IP");
    } else {
        ESP_LOGE(TAG, "Failed to start stream server");
    }
}

void display_task(void *pvParameters) {
    ESP_LOGI(TAG, "Display task started on core %d", xPortGetCoreID());
    int local_bbox[4];
    while (1)
    {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Failed to get frame");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        // 显示到 LED 屏幕上
        esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, fb->width, fb->height, fb->buf);
        if (xSemaphoreTake(xBoxMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // 拷贝数据，减少锁的持有时间
            memcpy(local_bbox, g_face_bbox, sizeof(local_bbox));
            xSemaphoreGive(xBoxMutex);
        }
        if (local_bbox[0] != -1) {
            spilcd_draw_rectangle(local_bbox[0], local_bbox[1], local_bbox[2], local_bbox[3], RED);
            // ESP_LOGI(TAG, "draw rectangle");
        }
        esp_camera_fb_return(fb);
        // 延时，控制帧率
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void upload_task(void *pvParameters) {
    ESP_LOGI(TAG, "Upload task starte on core %d", xPortGetCoreID());
    esp_task_wdt_add(NULL); // 注册当前任务
    const int32_t upload_interval_ms = 1000;
    int32_t last_upload_time = 0;
    while (1) {
        int32_t current_time = esp_timer_get_time() / 1000;
        if (current_time - last_upload_time >= upload_interval_ms) {
            // 每 1 秒上传
            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb) {
                ESP_LOGI(TAG, "Faile to get frame for upload");
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }
            uint8_t *jpg_buf = NULL;
            size_t jpg_buf_len = 0;

            if (frame2jpg(fb, 12, &jpg_buf, &jpg_buf_len)) {
                ESP_LOGI(TAG, "JEPG size: %d bytes", jpg_buf_len);
            }
            esp_task_wdt_reset(); // 重置看门狗

            if (jpg_buf != NULL) {
                // 加入mqtt发送队列
                mqtt_msg_t msg;
                msg.data = jpg_buf;
                msg.len = jpg_buf_len;

                if (xQueueSend(xMqttPublishQueue, &msg, pdMS_TO_TICKS(100)) != pdTRUE) {
                    ESP_LOGW("UPLOAD", "MQTT Queue is full! Dropping frame.");
                    // 队列满了，自己释放
                    free(jpg_buf);
                }
            }
            esp_camera_fb_return(fb);

            last_upload_time = current_time;
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}