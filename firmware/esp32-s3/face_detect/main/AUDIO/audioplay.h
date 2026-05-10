#ifndef __AUDIOPLAY_H
#define __AUDIOPLAY_H

#include "wavplay.h"
#include "exfuns.h"


typedef struct
{
    uint8_t *tbuf;      /* 零时数组,仅在24bit解码的时候需要用到 */
    FIL *file;          /* 音频文件指针 */

    uint8_t status;     /* bit0:0,暂停播放;1,继续播放 */
                        /* bit1:0,结束播放;1,开启播放 */
} __audiodev;

extern __audiodev g_audiodev;

/* 函数声明 */
void audio_start(void);
void audio_stop(void);
uint8_t audio_play_song(uint8_t *fname);                                /* 播放某个音频文件 */

#endif
