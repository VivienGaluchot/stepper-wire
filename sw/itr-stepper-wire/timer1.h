#ifndef TIMER1_H
#define TIMER1_H

#include <stdint.h>

// -------------------------------------------------------------
// Macros
// -------------------------------------------------------------

// Main clock frequency
#define MAIN_FREQ 16000000ULL

// Prescaler register values
#define TIMER1_PRESCALER_REG_1 (1 << CS10)                 // 16 MHz
#define TIMER1_PRESCALER_REG_8 (1 << CS11)                 // 2  MHz
#define TIMER1_PRESCALER_REG_64 (1 << CS11 | 1 << CS10)    // 250 kHz
#define TIMER1_PRESCALER_REG_256 (1 << CS12)               // 62,5 kHz
#define TIMER1_PRESCALER_REG_1024 (1 << CS12 | 1 << CS10)  // 15,6 kHz

// Selected prescaler
#define TIMER1_PRESCALER_REG TIMER1_PRESCALER_REG_8
#define TIMER1_PRESCALER 8ULL

// Helpers
#define TIMER1_PERIOD_IN_NS ((1000000000ULL * TIMER1_PRESCALER) / MAIN_FREQ)
#define TIMER1_COUNT_FOR_PERIOD_IN_NS(x) (x / TIMER1_PERIOD_IN_NS)

// -------------------------------------------------------------
// Services
// -------------------------------------------------------------

void disableTimer1();

void setPeriodTimer1(uint32_t itrPeriodInNs);

void enableTimer1(void (*callback)(void));

#endif
