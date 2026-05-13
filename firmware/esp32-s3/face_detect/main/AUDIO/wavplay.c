#include "wavplay.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"


#define MUSIC_PRIO      4                   /* 任务优先级 */
#define MUSIC_STK_SIZE  5*1024              /* 任务堆栈大小 */
TaskHandle_t            MUSICTask_Handler;  /* 任务句柄 */
void music(void *pvParameters);             /* 任务函数 */

static portMUX_TYPE my_spinlock = portMUX_INITIALIZER_UNLOCKED;


__wavctrl wavctrl;                          /* WAV音频文件解码参数结构体 */
UINT bytes_write = 0;                       /* 写一次I2S大小 */
volatile long long int i2s_table_size = 0;  /* 积累每次发送音频数据总大小 */
esp_err_t i2s_play_end = ESP_FAIL;          /* 播放结束标志位 */
esp_err_t i2s_play_next_prev = ESP_FAIL;    /* 下一首或者上一首标志位 */
FSIZE_t file_read_pos = 0;                  /* 记录当前WAV读取位置 */

/**
 * @brief       WAV解析初始化
 * @param       fname : 文件路径+文件名
 * @param       wavx  : 信息存放结构体指针
 * @retval      0,打开文件成功
 *              1,打开文件失败
 *              2,非WAV文件
 *              3,DATA区域未找到
 */
uint8_t wav_decode_init(uint8_t *fname, __wavctrl *wavx)
{
    FIL *ftemp;
    uint8_t *buf; 
    uint32_t br = 0;
    uint8_t res = 0;

    ChunkRIFF *riff;
    ChunkFMT *fmt;
    ChunkFACT *fact;
    ChunkDATA *data;
    
    ftemp = (FIL*)malloc(sizeof(FIL));
    buf = malloc(512);
    
    if (ftemp && buf)
    {
        res = f_open(ftemp, (TCHAR*)fname, FA_READ);            /* 打开文件 */
        
        if (res == FR_OK)
        {
            f_read(ftemp, buf, 512, (UINT *)&br);               /* 读取512字节在数据 */
            riff = (ChunkRIFF *)buf;
            
            if (riff->Format == 0x45564157)                     /* 是WAV文件 */
            {
                fmt = (ChunkFMT *)(buf + 12);
                fact = (ChunkFACT *)(buf + 12 + 8 + fmt->ChunkSize);
                
                if (fact->ChunkID == 0x74636166 || fact->ChunkID == 0x5453494C)
                {
                    wavx->datastart = 12 + 8 + fmt->ChunkSize + 8 + fact->ChunkSize;
                }
                else
                {
                    wavx->datastart = 12 + 8 + fmt->ChunkSize;
                }
                
                data = (ChunkDATA *)(buf + wavx->datastart);
                
                if (data->ChunkID == 0x61746164)                /* 解析成功 */
                {
                    wavx->audioformat = fmt->AudioFormat;       /* 音频格式 */
                    wavx->nchannels = fmt->NumOfChannels;       /* 通道数 */
                    wavx->samplerate = fmt->SampleRate;         /* 采样率 */
                    wavx->bitrate = fmt->ByteRate * 8;
                    wavx->blockalign = fmt->BlockAlign;
                    wavx->bps = fmt->BitsPerSample;
                    
                    wavx->datasize = data->ChunkSize;
                    wavx->datastart = wavx->datastart + 8;
                     
                    printf("wavx->audioformat:%d\r\n", wavx->audioformat);
                    printf("wavx->nchannels:%d\r\n", wavx->nchannels);
                    printf("wavx->samplerate:%ld\r\n", wavx->samplerate);
                    printf("wavx->bitrate:%ld\r\n", wavx->bitrate);
                    printf("wavx->blockalign:%d\r\n", wavx->blockalign);
                    printf("wavx->bps:%d\r\n", wavx->bps);
                    printf("wavx->datasize:%ld\r\n", wavx->datasize);
                    printf("wavx->datastart:%ld\r\n", wavx->datastart);  
                }
                else
                {
                    res = 3;
                }
            }
            else
            {
                res = 2;
            }
        }
    }
    
    if (res == FR_OK)
    {
        f_close(ftemp);
    }
    free(ftemp);
    free(buf); 
    
    return res;
}


/**
 * @brief       music任务
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void music(void *pvParameters)
{
    uint32_t bytes_write = 0;
    
    // 1. 硬件初始化与配置
    // 打开 DAC，关闭 ADC (省电且减少干扰)
    es8388_adda_cfg(1, 0);                          
    es8388_input_cfg(0);                            
    es8388_output_cfg(1, 1);                        
    es8388_hpvol_set(25);   // 设置音量 (0-63)，建议适中
    es8388_spkvol_set(25);                          
    xl9555_pin_write(SPK_EN_IO, 0); // 打开喇叭使能 (低电平有效)
    
    vTaskDelay(pdMS_TO_TICKS(20)); // 等待芯片稳定

    // 2. 【关键技巧】发送静音数据，消除“砰”的爆音
    // 将缓冲区清零，发送几帧，让 I2S 时钟先跑起来
    memset(g_audiodev.tbuf, 0, WAV_TX_BUFSIZE);
    for(int i=0; i<4; i++) { // 发送 4 次缓冲数据
         i2s_tx_write(g_audiodev.tbuf, WAV_TX_BUFSIZE);
    }

    while(1)
    {
        // 检查播放状态 (0x03 = 播放中)
        if ((g_audiodev.status & 0x0F) == 0x03)
        {
            // 循环读取文件，直到播放完整个文件
            while (i2s_table_size < wavctrl.datasize && i2s_play_next_prev != ESP_OK)
            {
                // --- 暂停逻辑 ---
                if ((g_audiodev.status & 0x0F) == 0x00)
                {
                    file_read_pos = f_tell(g_audiodev.file); // 记录位置
                    
                    // 死等恢复播放，但要给看门狗留活路
                    while ((g_audiodev.status & 0x0F) == 0x00) 
                    {
                        vTaskDelay(pdMS_TO_TICKS(10)); 
                    }
                    
                    f_lseek(g_audiodev.file, file_read_pos); // 回到记录的位置
                }

                // --- 读取与发送 ---
                // 注意：f_read 可能会读取不足 WAV_TX_BUFSIZE (比如文件末尾)
                FRESULT fr = f_read(g_audiodev.file, g_audiodev.tbuf, WAV_TX_BUFSIZE, (UINT*)&bytes_write);
                
                if (fr != FR_OK || bytes_write == 0) 
                {
                    i2s_play_end = ESP_OK; // 文件结束
                    break; 
                }

                // 发送实际读取到的字节数
                int sent_bytes = i2s_tx_write(g_audiodev.tbuf, bytes_write);
                i2s_table_size += sent_bytes;
                
                // --- 【关键】释放 CPU ---
                // 这里的延时非常重要！
                // 5ms 是一个平衡点：既不会让音频断流，又能让 SPI 屏幕刷新有机会插入
                vTaskDelay(pdMS_TO_TICKS(5)); 
            }
            
            // --- 播放结束清理 ---
            // 确保发送完最后一点数据
            vTaskDelay(pdMS_TO_TICKS(10));
            audio_stop();
            // i2s_deinit();
            i2s_play_end = ESP_OK;
            
            // 清理资源
            if(g_audiodev.file) { f_close(g_audiodev.file); heap_caps_free(g_audiodev.file); g_audiodev.file = NULL; }
            if(g_audiodev.tbuf) { heap_caps_free(g_audiodev.tbuf); g_audiodev.tbuf = NULL; }
            
            MUSICTask_Handler = NULL;
            ESP_LOGI("AUDIO", "Music task finished");
            vTaskDelete(NULL); // 任务结束，自杀
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/**
 * @brief       播放某个wav文件
 * @param       fname : 文件路径+文件名
 * @retval      KEY0_PRES : 下一首
 *              KEY1_PRES : 上一首
 *              KEY2_PRES : 停止/启动
 *              其他,非WAV文件
 */
uint8_t wav_play_song(uint8_t *fname)
{
    uint8_t res = 0;
    
    // 1. 检查是否正在播放 (防止重复触发)
    if (MUSICTask_Handler != NULL) {
        ESP_LOGW("AUDIO", "Audio is already playing!");
        return 1;
    }

    // 状态重置
    i2s_play_end = ESP_FAIL;
    i2s_play_next_prev = ESP_FAIL;
    i2s_table_size = 0;
    
    // 内存分配
    if (g_audiodev.file == NULL) g_audiodev.file = (FIL*)malloc(sizeof(FIL)); 
    if (g_audiodev.tbuf == NULL) g_audiodev.tbuf = heap_caps_malloc(WAV_TX_BUFSIZE, MALLOC_CAP_DMA);

    if (!g_audiodev.file || !g_audiodev.tbuf) {
        ESP_LOGE("AUDIO", "Memory Alloc Failed");
        return -2;
    }

    memset(g_audiodev.file, 0, sizeof(FIL));
    memset(g_audiodev.tbuf, 0, WAV_TX_BUFSIZE);
    memset(&wavctrl, 0, sizeof(__wavctrl));

    // 解码初始化
    res = wav_decode_init(fname, &wavctrl);

    if (res == 0)
    {
        // 配置采样率 (这是动态配置，不是初始化)
        if (wavctrl.bps == 16) i2s_set_samplerate_bits_sample(wavctrl.samplerate, I2S_BITS_PER_SAMPLE_16BIT);
        else if (wavctrl.bps == 24) i2s_set_samplerate_bits_sample(wavctrl.samplerate, I2S_BITS_PER_SAMPLE_24BIT);

        res = f_open(g_audiodev.file, (TCHAR*)fname, FA_READ);

        if (res == FR_OK)
        {
            audio_start(); // 开启通道
            
            // 创建任务
            // 因为上面已经检查过 NULL 了，这里直接创建
            xTaskCreatePinnedToCore(music, "music", 4096, NULL, 5, &MUSICTask_Handler, 0); 
            
            return 0; 
        }
    }
    
    // 错误处理
    if (g_audiodev.file) { heap_caps_free(g_audiodev.file); g_audiodev.file = NULL; }
    if (g_audiodev.tbuf) { heap_caps_free(g_audiodev.tbuf); g_audiodev.tbuf = NULL; }
    return res;
}