#include <Arduino.h>
#include <avr/interrupt.h>

// Configuration

#define LED_SWITCH_PERIOD_IN_US 1000000
#define LOG_PERIOD_IN_US 1000000

// Tools

bool periodical(unsigned long currentTime, unsigned long period, unsigned long *lastTime, unsigned long *missedPeriod = nullptr) {
    bool hasTrigger = false;
    unsigned long missedTrigger = (currentTime - *lastTime) / period;
    if (missedTrigger > 0) {
        *lastTime += missedTrigger * period;
        hasTrigger = true;
        if (missedPeriod != nullptr) {
            *missedPeriod += (missedTrigger - 1);
        }
    }
    return hasTrigger;
}

// Public services

// 16 MHz main clock frequency
#define MAIN_FREQ 16000000

#define TIMER1_PRESCALE_1 0b001     // 16 MHz
#define TIMER1_PRESCALE_8 0b010     // 2  MHz
#define TIMER1_PRESCALE_64 0b011    // 250 kHz
#define TIMER1_PRESCALE_256 0b100   // 62,5 kHz
#define TIMER1_PRESCALE_1024 0b101  // 15,6 kHz

#define TIMER2_PRESCALE_1 0b001     // 16 MHz
#define TIMER2_PRESCALE_8 0b010     // 2  MHz
#define TIMER2_PRESCALE_32 0b011    // 500 kHz
#define TIMER2_PRESCALE_64 0b100    // 250 kHz
#define TIMER2_PRESCALE_128 0b101   // 125 kHz
#define TIMER2_PRESCALE_256 0b110   // 62,5 kHz
#define TIMER2_PRESCALE_1024 0b111  // 15,6 kHz

void setup() {
    noInterrupts();
    // Timer 1 - 16 bit timer
    TCCR1A = 0;
    TCCR1B = 0;
    TIMSK1 = 0;
    TCCR1B = TCCR1B | TIMER1_PRESCALE_256;  // increment each 16us
    TIMSK1 |= (1 << OCIE1A);                // compare match interrupt
    OCR1A = 99;                             // interrupt at 99
    TCNT1 = 0;                              // counter at 0

    // Timer 2 - 8 bit timer
    TCCR2A = 0;
    TCCR2B = 0;
    TIMSK2 = 0;
    TCNT2 = 0;
    TCCR2B = TCCR2B | TIMER2_PRESCALE_256;  // increment each 16us
    TIMSK2 |= (1 << OCIE2A);                // compare match interrupt
    OCR2A = 99;                             // interrupt at 99
    TCNT2 = 0;                              // counter at 0
    interrupts();

    Serial.begin(115200);

    Serial.println("===================");
    Serial.println("Firmware: itr_demo v0");

    Serial.println("Setup done");
}

unsigned long lastLedSwitchTime = 0;
bool lastLedState = LOW;

unsigned long lastLogTime = 0;

volatile unsigned long timer1ItrCounter = 0;
volatile unsigned long timer2ItrCounter = 0;

void loop() {
    unsigned long loopTime = micros();

    // blink led
    unsigned long ledSwitchPeriod = LED_SWITCH_PERIOD_IN_US;
    if (periodical(loopTime, ledSwitchPeriod, &lastLedSwitchTime)) {
        lastLedState = !lastLedState;
        digitalWrite(LED_BUILTIN, lastLedState);
    }

    // produce logs
    if (periodical(loopTime, LOG_PERIOD_IN_US, &lastLogTime)) {
        Serial.println("---");
        Serial.print("LoopTime : ");
        Serial.println(loopTime);

        unsigned long counterValue;
        counterValue = timer1ItrCounter;
        Serial.print("timer1ItrCounter : ");
        Serial.println(counterValue);
        timer1ItrCounter = timer1ItrCounter - counterValue;

        counterValue = timer2ItrCounter;
        Serial.print("timer2ItrCounter : ");
        Serial.println(counterValue);
        timer2ItrCounter = timer2ItrCounter - counterValue;
    }
}

ISR(TIMER1_COMPA_vect) {
    noInterrupts();
    timer1ItrCounter++;
    TCNT1 = 0;
    interrupts();
}

ISR(TIMER2_COMPA_vect) {
    noInterrupts();
    timer2ItrCounter++;
    TCNT2 = 0;
    interrupts();
}