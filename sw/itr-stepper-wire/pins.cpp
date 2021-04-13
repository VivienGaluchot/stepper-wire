#include "pins.h"

#include <Arduino.h>

static const uint8_t PINS_DIR[CHANNEL_COUNT] = {PIN_OUT_CH1_DIR, PIN_OUT_CH2_DIR};
static const uint8_t PINS_STEP[CHANNEL_COUNT] = {PIN_OUT_CH1_STEP, PIN_OUT_CH2_STEP};
static const uint8_t PINS_MS2[CHANNEL_COUNT] = {PIN_OUT_CH1_MS2, PIN_OUT_CH2_MS2};
static const uint8_t PINS_MS1[CHANNEL_COUNT] = {PIN_OUT_CH1_MS1, PIN_OUT_CH2_MS1};
static const uint8_t PINS_ENABLE[CHANNEL_COUNT] = {PIN_OUT_CH1_ENABLE, PIN_OUT_CH2_ENABLE};

// -------------------------------------------------------------
// Public services
// -------------------------------------------------------------

void initializePins() {
    // input
    pinMode(PIN_IN_HAND_POT, INPUT);

    // output ch1
    pinMode(PIN_OUT_CH1_DIR, OUTPUT);
    pinMode(PIN_OUT_CH1_STEP, OUTPUT);
    pinMode(PIN_OUT_CH1_MS2, OUTPUT);
    pinMode(PIN_OUT_CH1_MS1, OUTPUT);
    pinMode(PIN_OUT_CH1_ENABLE, OUTPUT);
    digitalWrite(PIN_OUT_CH1_DIR, LOW);
    digitalWrite(PIN_OUT_CH1_STEP, LOW);
    digitalWrite(PIN_OUT_CH1_MS2, LOW);
    digitalWrite(PIN_OUT_CH1_MS1, LOW);
    digitalWrite(PIN_OUT_CH1_ENABLE, LOW);

    // output ch2
    pinMode(PIN_OUT_CH2_DIR, OUTPUT);
    pinMode(PIN_OUT_CH2_STEP, OUTPUT);
    pinMode(PIN_OUT_CH2_MS2, OUTPUT);
    pinMode(PIN_OUT_CH2_MS1, OUTPUT);
    pinMode(PIN_OUT_CH2_ENABLE, OUTPUT);
    digitalWrite(PIN_OUT_CH2_DIR, LOW);
    digitalWrite(PIN_OUT_CH2_STEP, LOW);
    digitalWrite(PIN_OUT_CH2_MS2, LOW);
    digitalWrite(PIN_OUT_CH2_MS1, LOW);
    digitalWrite(PIN_OUT_CH2_ENABLE, LOW);
}

int readHandPot() {
    return analogRead(PIN_IN_HAND_POT);
}

void setDirection(Channel_T channel, bool isClockwise) {
    digitalWrite(PINS_DIR[channel], isClockwise);
}

void setStep(Channel_T channel, bool isHigh) {
    digitalWrite(PINS_STEP[channel], isHigh);
}

void setMs1(Channel_T channel, bool isHigh) {
    digitalWrite(PINS_MS1[channel], isHigh);
}

void setMs2(Channel_T channel, bool isHigh) {
    digitalWrite(PINS_MS2[channel], isHigh);
}

void setEnable(Channel_T channel, bool isDriverEnabled) {
    // driver is enabled when EN is LOW
    digitalWrite(PINS_ENABLE[channel], !isDriverEnabled);
}