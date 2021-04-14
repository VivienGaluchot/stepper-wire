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
#define NS_PER_S 1000000000ULL
#define PERIOD_IN_NS_FOR_FREQ_IN_HZ(freq) (NS_PER_S / freq)
#define FREQ_IN_HS_FOR_PERIOD_IN_NS(period) (NS_PER_S / period)

const uint16_t TIMER1_PERIOD_IN_NS = ((NS_PER_S * TIMER1_PRESCALER) / MAIN_FREQ);
#define TIMER1_COUNT_FOR_PERIOD_IN_NS(period) (((period) / TIMER1_PERIOD_IN_NS) - 1ULL)
#define TIMER1_COUNT_FOR_FREQUENCY_IN_HZ(freq) TIMER1_COUNT_FOR_PERIOD_IN_NS(PERIOD_IN_NS_FOR_FREQ_IN_HZ(freq))

#define STATIC_CHECK_FREQ_IN_RANGE(freq)                                    \
    static_assert(TIMER1_COUNT_FOR_FREQUENCY_IN_HZ(freq) > 0 &&             \
                      TIMER1_COUNT_FOR_FREQUENCY_IN_HZ(freq) < (1UL << 16), \
                  "speed range not supported by current timer1 configuration")

// -------------------------------------------------------------
// Services
// -------------------------------------------------------------

namespace timer1 {

uint16_t countForFrequency(int32_t itrFreqInHz);

void disable();

void enable(void (*callback)(void));

void setFrequency(uint32_t itrFreqInHz);

// the interrupt frequency will be brought to itrFreqInHz
// by keeping a maximum changing rate of maxRateHzPerUs
void setRampFrequency(uint32_t itrFreqInHz, uint32_t maxRateHzPerUs);

uint32_t getFrequencyInHz();

// diagnostics

uint16_t getCmpCounter();

uint16_t popFlushedTicks();

}  // namespace timer1

#endif
