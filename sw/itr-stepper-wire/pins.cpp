#include "pins.h"

#include <Arduino.h>

static const uint8_t PINS_DIR[CHANNEL_COUNT] = {PIN_OUT_CH1_DIR, PIN_OUT_CH2_DIR};
static const uint8_t PINS_STEP[CHANNEL_COUNT] = {PIN_OUT_CH1_STEP, PIN_OUT_CH2_STEP};
static const uint8_t PINS_MS2[CHANNEL_COUNT] = {PIN_OUT_CH1_MS2, PIN_OUT_CH2_MS2};
static const uint8_t PINS_MS1[CHANNEL_COUNT] = {PIN_OUT_CH1_MS1, PIN_OUT_CH2_MS1};
static const uint8_t PINS_ENABLE[CHANNEL_COUNT] = {PIN_OUT_CH1_ENABLE, PIN_OUT_CH2_ENABLE};

static uint16_t handPotStickyValue = 0;

#define HAND_POT_LAST_VALUE_SIZE 32
static uint16_t handPotLastValues[HAND_POT_LAST_VALUE_SIZE] = {
    0,
};
static uint8_t handPotLastValueIndex = 0;

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

uint16_t readHandPot() {
    // raw value is returned on 10bit
    uint16_t rawValue = analogRead(PIN_IN_HAND_POT);

    // mean on HAND_POT_LAST_VALUE_SIZE values
    handPotLastValueIndex = (handPotLastValueIndex + 1) % HAND_POT_LAST_VALUE_SIZE;
    handPotLastValues[handPotLastValueIndex] = rawValue;
    uint32_t meanValueSum = 0;
    for (uint8_t i = 0; i < HAND_POT_LAST_VALUE_SIZE; i++) {
        meanValueSum += handPotLastValues[i];
    }
    uint16_t meanValue = meanValueSum / HAND_POT_LAST_VALUE_SIZE;

    // stick the mean value
    const uint16_t stickyRange = 5;
    if (meanValue > handPotStickyValue) {
        if (meanValue - handPotStickyValue > stickyRange) {
            handPotStickyValue = meanValue;
        }
    } else if (handPotStickyValue > meanValue) {
        if (handPotStickyValue - meanValue > stickyRange) {
            handPotStickyValue = meanValue;
        }
    }

    // clamp min max to output range
    const uint16_t minClampValue = 0;
    const uint16_t maxClampValue = (1 << 10) - 10;
    const uint16_t clampedRange = maxClampValue - minClampValue;
    uint16_t clamped = handPotStickyValue;
    clamped = max(clamped, minClampValue);
    clamped = min(clamped, maxClampValue);
    clamped = clamped - minClampValue;
    const uint32_t outRange = MAX_POT_VALUE - MIN_POT_VALUE;
    uint16_t output = ((uint32_t)clamped * (uint32_t)outRange) / (uint32_t)clampedRange + MIN_POT_VALUE;

    return output;
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