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

// current timer frequency
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
    noInterrupts();
    (*itrCallback)();

    if (rampIsEnabled) {
        rampLastPeriod += OCR1A;
        uint16_t missedRamp = (rampLastPeriod / rampPeriod);
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
            OCR1A = TIMER1_COUNT_FOR_FREQUENCY_IN_HZ(currentItrFreqInHz);
            rampLastPeriod %= rampPeriod;
        }
    }

    // flush counter if the CPU can't keep up with the interrupt frequency
    volatile uint16_t cntr = TCNT1;
    if (OCR1A < cntr) {
        flushedTicks += cntr;
        TCNT1 -= cntr;
    }

    interrupts();
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

void timer1::setRampFrequency(uint32_t itrFreqInHz, uint32_t maxRateHzPerUs) {
    // check requested frequency is in range
    countForFrequency(itrFreqInHz);

    rampIsEnabled = false;

    rampFinalInHz = itrFreqInHz;
    rampLastPeriod = 0;
    // ramp by step of 10 Hz
    rampStepInHz = 10;
    // ramp each 10 ms
    // Warning, too small values lead to CPU stall
    rampPeriod = 10000000UL / TIMER1_PERIOD_IN_NS;

    rampIsEnabled = true;

    Serial.println("setRampPeriod, options:");
    Serial.print("  - current ");
    Serial.println(currentItrFreqInHz);

    Serial.print("  - final   ");
    Serial.println(rampFinalInHz);

    Serial.print("  - period  ");
    Serial.println(rampPeriod);

    Serial.print("  - step    ");
    Serial.println(rampStepInHz);
}

uint32_t timer1::getFrequencyInHz() {
    uint32_t period = uint32_t(OCR1A + 1) * TIMER1_PERIOD_IN_NS;
    return FREQ_IN_HS_FOR_PERIOD_IN_NS(period);
}

uint16_t timer1::getCmpCounter() {
    return OCR1A;
}

uint16_t timer1::popFlushedTicks() {
    uint16_t x = flushedTicks;
    flushedTicks = 0;
    return x;
}