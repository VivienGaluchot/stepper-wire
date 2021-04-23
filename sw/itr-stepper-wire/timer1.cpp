#include "timer1.h"

#include <Arduino.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#include "errors.h"

// -------------------------------------------------------------
// Private variables
// -------------------------------------------------------------

static void (*itrCallback)(void) = 0;

static volatile uint16_t flushedTicks = 0;

// current timer state
static volatile uint16_t currentItrFreqInHz = 0;

// when true the period ramp is enabled
static volatile bool rampIsEnabled = false;
// total number of itr interrupt ellapsed since the last ramp
static volatile uint16_t rampLastPeriod = 0;
// total number of itr interrupt between each ramp
static volatile uint16_t rampPeriod = 0;
// change applyed to the timer frequency each `rampPeriod`
static volatile uint16_t rampStepInHz = 0;
// final timer frequency of the ramp
static volatile uint16_t rampFinalInHz = 0;

// -------------------------------------------------------------
// Private services
// -------------------------------------------------------------

ISR(TIMER1_COMPA_vect) {
    (*itrCallback)();

    if (rampIsEnabled) {
        rampLastPeriod += OCR1A;
        uint16_t missedRamp = rampLastPeriod / rampPeriod;
        if (missedRamp > 0) {
            uint16_t freqStep = rampStepInHz * missedRamp;
            if (rampFinalInHz < currentItrFreqInHz) {
                // |----rampFinalInHz--------currentItrFreqInHz------>
                if (freqStep < (currentItrFreqInHz - rampFinalInHz)) {
                    currentItrFreqInHz -= freqStep;
                } else {
                    currentItrFreqInHz = rampFinalInHz;
                    rampIsEnabled = false;
                }
            } else if (currentItrFreqInHz < rampFinalInHz) {
                // |----currentItrFreqInHz--------rampFinalInHz------>
                if (freqStep < (rampFinalInHz - currentItrFreqInHz)) {
                    currentItrFreqInHz += freqStep;
                } else {
                    currentItrFreqInHz = rampFinalInHz;
                    rampIsEnabled = false;
                }
            } else {
                rampIsEnabled = false;
            }
            rampLastPeriod -= missedRamp * rampPeriod;
            OCR1A = TIMER1_COUNT_FOR_FREQUENCY_IN_HZ(currentItrFreqInHz);
        }
    }
}

// -------------------------------------------------------------
// Public services
// -------------------------------------------------------------

uint16_t timer1::countForFrequency(int32_t itrFreqInHz) {
    int32_t cmp = (PERIOD_IN_NS_FOR_FREQ_IN_HZ(itrFreqInHz) / TIMER1_PERIOD_IN_NS) - 1;
    if (cmp >= (1L << 16) || cmp < 1) {
        Serial.print("out of range frequency ");
        Serial.println(itrFreqInHz);
        errorTrap(TIMER_1_ITR_CMP_OUT_OF_RANGE);
    }
    return cmp;
}

void timer1::disable() {
    noInterrupts();
    TCCR1A = 0;
    TCCR1B = 0;
    TIMSK1 = 0;
    interrupts();
}

void timer1::enable(void (*callback)(void)) {
    noInterrupts();
    TCCR1A = 0;
    TCCR1B = 0;
    TIMSK1 = 0;
    TCNT1 = 0;                       // counter at 0
    TCCR1B |= TIMER1_PRESCALER_REG;  // enable with the selected prescaler
    TCCR1B |= (1 << WGM12);          // CTC mode with top value OCR1A
    TIMSK1 |= (1 << OCIE1A);         // compare match interrupt

    itrCallback = callback;
    interrupts();
}

void timer1::setFrequency(uint32_t itrFreqInHz) {
    volatile uint32_t cmp = countForFrequency(itrFreqInHz);
    noInterrupts();
    currentItrFreqInHz = itrFreqInHz;
    OCR1A = cmp;  // set timer compare register
    interrupts();
}

void timer1::setRampFrequency(uint32_t itrFreqInHz, uint32_t rateInHzPerS) {
    // minimal ramp period to prevent main loop stall: 1ms
    const uint32_t safeRampPeriodInUs = 1000;

    // check requested frequency is in range
    countForFrequency(itrFreqInHz);

    // compute optimal period and step for a smooth / precise ramp
    uint32_t periodInUs = 1000000UL;
    uint32_t stepInHz = rateInHzPerS;
    const uint32_t maxScale = periodInUs / safeRampPeriodInUs;
    uint32_t scale = min(stepInHz, maxScale);
    periodInUs /= scale;
    stepInHz /= scale;

    uint32_t periodInTicks = TIMER1_COUNT_FOR_PERIOD_IN_NS(periodInUs * 1000);
    if (periodInTicks >= (1UL << 16)) {
        Serial.print("periodInTicks ");
        Serial.println(periodInTicks);
        errorTrap(TIMER_1_RAMPING_RATE_OUT_OF_RANGE);
    }

    bool hasChanged = false;
    rampIsEnabled = false;

    // taget frequency
    hasChanged = hasChanged || rampFinalInHz != itrFreqInHz;
    rampFinalInHz = itrFreqInHz;
    // ramp by step of X Hz
    hasChanged = hasChanged || rampStepInHz != stepInHz;
    rampStepInHz = stepInHz;
    // ramp period in timer ticks
    hasChanged = hasChanged || rampPeriod != periodInTicks;
    rampPeriod = periodInTicks;

    // don't reset the last period if the conf is not changed to prevent the ramp from
    // executing when setRampFrequency is called in loop with the same parameters
    if (hasChanged) {
        rampLastPeriod = 0;
    }

    rampIsEnabled = true;

    Serial.println("setRampPeriod, options:");
    Serial.print("  - current ");
    Serial.println(currentItrFreqInHz);

    Serial.print("  - final   ");
    Serial.println(rampFinalInHz);

    Serial.print("  - period in us ");
    Serial.println(periodInUs);

    Serial.print("  - period in ticks ");
    Serial.println(rampPeriod);

    Serial.print("  - step    ");
    Serial.println(rampStepInHz);
}

uint16_t timer1::getFrequencyInHz() {
    return currentItrFreqInHz;
}

uint16_t timer1::popFlushedTicks() {
    uint16_t x = flushedTicks;
    flushedTicks = 0;
    return x;
}