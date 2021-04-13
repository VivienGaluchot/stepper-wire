#include "timer1.h"

#include <Arduino.h>
#include <avr/interrupt.h>

#include "errors.h"

// -------------------------------------------------------------
// Private variables
// -------------------------------------------------------------

static void (*itrCallback)(void) = 0;

// -------------------------------------------------------------
// Private services
// -------------------------------------------------------------

ISR(TIMER1_COMPA_vect) {
    (*itrCallback)();
}

// -------------------------------------------------------------
// Public services
// -------------------------------------------------------------

void disableTimer1() {
    noInterrupts();
    TCCR1A = 0;
    TCCR1B = 0;
    TIMSK1 = 0;
    interrupts();
}

void setPeriodTimer1(uint32_t itrPeriodInNs) {
    int32_t cmp = TIMER1_COUNT_FOR_PERIOD_IN_NS(itrPeriodInNs) - 1;
    if (cmp >= (1L << 16) || cmp < 1) {
        errorTrap(TIMER_1_ITR_CMP_OUT_OF_RANGE);
    }
    noInterrupts();
    OCR1A = cmp;  // set timer compare register
    interrupts();
}

void enableTimer1(void (*callback)(void)) {
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