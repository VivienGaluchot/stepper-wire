#ifndef PIN_H
#define PIN_H

#include <stdint.h>

#define PIN_IN_SPEED A0

enum Channel_T
{
    CHANNEL_1 = 0,
    CHANNEL_2 = 1,
    CH_COUNT
};

enum Channel_Pin_T
{
    PIN_DIR = 0,
    PIN_STEP = 1,
    PIN_EN = 2,
    PIN_COUNT
};

const int PIN_MAP[CH_COUNT][PIN_COUNT] = {
    // CHANEL_1
    {
        // PIN_DIR
        2,
        // PIN_STEP
        3,
        // PIN_EN
        4,
    },

    // CHANNEL_2
    {
        // PIN_DIR
        8,
        // PIN_STEP
        9,
        // PIN_EN
        10,
    },
};

#endif
