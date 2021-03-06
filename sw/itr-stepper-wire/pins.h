#ifndef PIN_H
#define PIN_H

#include <stdint.h>

#define PIN_IN_HAND_POT A0
#define PIN_IN_FOOT_POT A1

#define PIN_OUT_CH1_DIR 2
#define PIN_OUT_CH1_STEP 3
#define PIN_OUT_CH1_MS2 4
#define PIN_OUT_CH1_MS1 5
#define PIN_OUT_CH1_ENABLE 6

#define PIN_OUT_CH2_DIR 8
#define PIN_OUT_CH2_STEP 9
#define PIN_OUT_CH2_MS2 10
#define PIN_OUT_CH2_MS1 11
#define PIN_OUT_CH2_ENABLE 12

enum Channel_T {
    CHANNEL_1 = 0,
    CHANNEL_2 = 1,
    CHANNEL_COUNT
};

#define MIN_POT_VALUE 0
#define MAX_POT_VALUE 1023

void initializePins();

// return in range MIN_POT_VALUE .. MAX_POT_VALUE
uint16_t readHandPot();

// return in range MIN_POT_VALUE .. MAX_POT_VALUE
uint16_t readFootPot();

void setDirection(Channel_T channel, bool isClockwise);

void setStep(Channel_T channel, bool isHigh);

void setMs1(Channel_T channel, bool isHigh);

void setMs2(Channel_T channel, bool isHigh);

void setEnable(Channel_T channel, bool isDriverEnabled);

#endif
