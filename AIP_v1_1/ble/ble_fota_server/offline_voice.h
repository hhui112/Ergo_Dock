#ifndef OFFLINE_VOICE
#define OFFLINE_VOICE

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool enabled;           // en = 1 
    uint8_t wake_word;      // 1=Hello Ergo, 2=Hello Bed
} offline_voice_ctrl_t;

extern offline_voice_ctrl_t g_offline_voice;



void offline_voice_cmdHandle(uint8_t cmd);

#endif