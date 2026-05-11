#ifndef CAMERA_STREAMER_H
#define CAMERA_STREAMER_H

// 启动视频流服务器
void start_camera_stream_server(void);
void display_task(void *pvParameters);
void upload_task(void *pvParameters);
void audio_monitor_task(void *pvParameters);

#endif