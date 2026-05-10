#include "audioplay.h"


__audiodev g_audiodev;          /* 音乐播放控制器 */


void audio_start(void)
{
    g_audiodev.status = 3 << 0;
    i2s_trx_start();
}

void audio_stop(void)
{
    g_audiodev.status = 0;
    i2s_trx_stop();
}

/**
 * @brief       播放某个音频文件
 * @param       fname : 文件名
 */
uint8_t audio_play_song(uint8_t *fname)
{
    uint8_t res;  
    res = exfuns_file_type((char *)fname); 
    res = wav_play_song(fname);
    return res;
}
