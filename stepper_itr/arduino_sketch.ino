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

#define PRESCALE_1 0b001     // 16 MHz
#define PRESCALE_8 0b010     // 2  MHz
#define PRESCALE_32 0b011    // 500 kHz
#define PRESCALE_64 0b100    // 250 kHz
#define PRESCALE_128 0b101   // 125 kHz
#define PRESCALE_256 0b110   // 62,5 kHz
#define PRESCALE_1024 0b111  // 15,6 kHz

void setup() {
    noInterrupts();
    TCCR2A = 0;                      // default
    TCCR2B = 0;                      // default
    TCCR2B = TCCR2B | PRESCALE_256;  // increment each 16us
    TIMSK2 = 0b00000001;             // TOIE2 interrupt
    interrupts();

    Serial.begin(115200);

    Serial.println("===================");
    Serial.println("Firware: v0");

    Serial.println("Setup done");
}

unsigned long lastLedSwitchTime = 0;
bool lastLedState = LOW;

unsigned long lastLogTime = 0;

volatile unsigned long itrCounter = 0;

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

        unsigned long counterValue = itrCounter;
        Serial.print("itrCounter : ");
        Serial.println(counterValue);
        itrCounter = itrCounter - counterValue;
    }

    delay(10);
}

ISR(TIMER2_OVF_vect) {
    noInterrupts();

    // next interrupt in 25 timer tick
    TCNT2 = (256 - 25);
    itrCounter++;

    interrupts();
}