#include "pins.h"

#include <Arduino.h>

static const uint8_t PINS_DIR[CHANNEL_COUNT] = {PIN_OUT_CH1_DIR, PIN_OUT_CH2_DIR};
static const uint8_t PINS_STEP[CHANNEL_COUNT] = {PIN_OUT_CH1_STEP, PIN_OUT_CH2_STEP};
static const uint8_t PINS_MS2[CHANNEL_COUNT] = {PIN_OUT_CH1_MS2, PIN_OUT_CH2_MS2};
static const uint8_t PINS_MS1[CHANNEL_COUNT] = {PIN_OUT_CH1_MS1, PIN_OUT_CH2_MS1};
static const uint8_t PINS_ENABLE[CHANNEL_COUNT] = {PIN_OUT_CH1_ENABLE, PIN_OUT_CH2_ENABLE};

#define HAND_POT_LAST_VALUE_SIZE 32
static uint16_t handPotLastValues[HAND_POT_LAST_VALUE_SIZE];
static uint8_t handPotLastValueIndex = 0;
static uint16_t handPotStickyValue = 0;

#define FOOT_POT_LAST_VALUE_SIZE 32
static uint16_t footPotLastValues[FOOT_POT_LAST_VALUE_SIZE];
static uint8_t footPotLastValueIndex = 0;
static uint16_t footPotStickyValue = 0;

// -------------------------------------------------------------
// Public services
// -------------------------------------------------------------

void initializePins() {
    memset(handPotLastValues, 0, sizeof(handPotLastValues));
    handPotLastValueIndex = 0;
    handPotStickyValue = 0;
    memset(footPotLastValues, 0, sizeof(footPotLastValues));
    footPotLastValueIndex = 0;
    footPotStickyValue = 0;

    // input
    pinMode(PIN_IN_HAND_POT, INPUT);
    pinMode(PIN_IN_FOOT_POT, INPUT);

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

uint16_t readPot(uint8_t pin,
                 uint16_t* lastValues,
                 uint8_t lastValuesSize,
                 uint8_t* lastValuesIndex,
                 uint16_t* stickyValue,
                 uint16_t stickyRange,
                 uint16_t minClampValue,
                 uint16_t maxClampValue) {
    // raw value is returned on 10bit
    uint16_t rawValue = analogRead(pin);

    // mean on lastValuesSize values
    *lastValuesIndex = (*lastValuesIndex + 1) % lastValuesSize;
    lastValues[*lastValuesIndex] = rawValue;
    uint32_t meanValueSum = 0;
    for (uint8_t i = 0; i < lastValuesSize; i++) {
        meanValueSum += lastValues[i];
    }
    uint16_t meanValue = meanValueSum / lastValuesSize;

    // stick the mean value
    if (meanValue > *stickyValue) {
        if (meanValue - *stickyValue > stickyRange) {
            *stickyValue = meanValue;
        }
    } else if (*stickyValue > meanValue) {
        if (*stickyValue - meanValue > stickyRange) {
            *stickyValue = meanValue;
        }
    }

    // clamp min max to output range
    const uint16_t clampedRange = maxClampValue - minClampValue;
    uint16_t clamped = *stickyValue;
    clamped = max(clamped, minClampValue);
    clamped = min(clamped, maxClampValue);
    clamped = clamped - minClampValue;
    const uint32_t outRange = MAX_POT_VALUE - MIN_POT_VALUE;
    uint16_t output = ((uint32_t)clamped * (uint32_t)outRange) / (uint32_t)clampedRange + MIN_POT_VALUE;

    return output;
}

uint16_t readHandPot() {
    const uint16_t stickyRange = 5;
    const uint16_t minClampValue = 10;
    const uint16_t maxClampValue = (1 << 10) - 10;
    uint16_t value = readPot(PIN_IN_HAND_POT,
                             &handPotLastValues[0],
                             HAND_POT_LAST_VALUE_SIZE,
                             &handPotLastValueIndex,
                             &handPotStickyValue,
                             stickyRange,
                             minClampValue,
                             maxClampValue);
    return value;
}

uint16_t readFootPot() {
    const uint16_t stickyRange = 2;
    const uint16_t minClampValue = 721 + 5;
    const uint16_t maxClampValue = 874 - 5;
    uint16_t value = readPot(PIN_IN_FOOT_POT,
                             &footPotLastValues[0],
                             FOOT_POT_LAST_VALUE_SIZE,
                             &footPotLastValueIndex,
                             &footPotStickyValue,
                             stickyRange,
                             minClampValue,
                             maxClampValue);
    return value;
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