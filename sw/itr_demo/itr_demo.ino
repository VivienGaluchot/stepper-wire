#include <Arduino.h>
#include <avr/interrupt.h>

// -------------------------------------------------------------
// Configuration
// -------------------------------------------------------------

// Led

#define LED_SWITCH_PERIOD_IN_US 1000000
#define LOG_PERIOD_IN_US 1000000

// Motor

// one motor step is 2 driver step according to the MS1 and MS2
// pin configurations
#define DRIVER_MICROSTEPPING 2UL
// number of step per turn in the motor
#define MOTOR_STEP_PER_ROTATION 200UL

// minimal speed in rpm
#define MIN_SPEED_MOTOR_RPM 6UL
// maximal speed in rpm
#define MAX_SPEED_MOTOR_RPM 1200UL

#define SEC_PER_MIN 60UL
#define ITR_PER_STEP 2UL

#define RPM_TO_ITR_FREQ_IN_HZ(x) (x * DRIVER_MICROSTEPPING * ITR_PER_STEP * MOTOR_STEP_PER_ROTATION / SEC_PER_MIN)
#define FREQ_IN_HZ_TO_PERIOD_IN_NS(x) (1000000000ULL / x)

const uint32_t MIN_SPEED_ITR_FREQ_IN_HZ = RPM_TO_ITR_FREQ_IN_HZ(MIN_SPEED_MOTOR_RPM);
const uint32_t MAX_SPEED_ITR_FREQ_IN_HZ = RPM_TO_ITR_FREQ_IN_HZ(MAX_SPEED_MOTOR_RPM);

const uint32_t MIN_SPEED_ITR_PERIOD_IN_NS = FREQ_IN_HZ_TO_PERIOD_IN_NS(MIN_SPEED_ITR_FREQ_IN_HZ);
const uint32_t MAX_SPEED_ITR_PERIOD_IN_NS = FREQ_IN_HZ_TO_PERIOD_IN_NS(MAX_SPEED_ITR_FREQ_IN_HZ);

// -------------------------------------------------------------
// Timers
// -------------------------------------------------------------

// 16 MHz main clock frequency
#define MAIN_FREQ 16000000UL

#define TIMER1_PRESCALER_REG_1 (1 << CS10)                 // 16 MHz
#define TIMER1_PRESCALER_REG_8 (1 << CS11)                 // 2  MHz
#define TIMER1_PRESCALER_REG_64 (1 << CS11 | 1 << CS10)    // 250 kHz
#define TIMER1_PRESCALER_REG_256 (1 << CS12)               // 62,5 kHz
#define TIMER1_PRESCALER_REG_1024 (1 << CS12 | 1 << CS10)  // 15,6 kHz

#define TIMER1_PRESCALER_REG TIMER1_PRESCALER_REG_8
#define TIMER1_PRESCALER 8

const uint32_t TIMER1_PERIOD_IN_NS = (1000000000ULL * TIMER1_PRESCALER) / MAIN_FREQ;

#define TIMER1_COUNT_FOR_PERIOD_IN_NS(x) (x / TIMER1_PERIOD_IN_NS)
const uint32_t MIN_SPEED_TIMER1_COUNT = TIMER1_COUNT_FOR_PERIOD_IN_NS(MIN_SPEED_ITR_PERIOD_IN_NS);
const uint32_t MAX_SPEED_TIMER1_COUNT = TIMER1_COUNT_FOR_PERIOD_IN_NS(MAX_SPEED_ITR_PERIOD_IN_NS);

static_assert(MIN_SPEED_TIMER1_COUNT < (1UL << 16), "speed range not supported by timer");
static_assert(MAX_SPEED_TIMER1_COUNT < (1UL << 16), "speed range not supported by timer");
static_assert(MIN_SPEED_TIMER1_COUNT > 0, "speed range not supported by timer");
static_assert(MAX_SPEED_TIMER1_COUNT > 0, "speed range not supported by timer");

// -------------------------------------------------------------
// Private services
// -------------------------------------------------------------

bool periodical(uint32_t currentTime, uint32_t period, uint32_t *lastTime, uint32_t *missedPeriod = nullptr) {
    bool hasTrigger = false;
    uint32_t missedTrigger = (currentTime - *lastTime) / period;
    if (missedTrigger > 0) {
        *lastTime += missedTrigger * period;
        hasTrigger = true;
        if (missedPeriod != nullptr) {
            *missedPeriod += (missedTrigger - 1);
        }
    }
    return hasTrigger;
}

// -------------------------------------------------------------
// Public services
// -------------------------------------------------------------

void setup() {
    noInterrupts();
    // Timer 1 - 16 bit timer
    TCCR1A = 0;
    TCCR1B = 0;
    TIMSK1 = 0;
    TCCR1B |= TIMER1_PRESCALER_REG;  // set the selected prescaler
    TCCR1B |= (1 << WGM12);          // CTC mode with top value OCR1A
    TIMSK1 |= (1 << OCIE1A);         // compare match interrupt
    OCR1A = MAX_SPEED_TIMER1_COUNT;  // interrupt at MIN_SPEED_TIMER1_COUNT
    TCNT1 = 0;                       // counter at 0
    interrupts();

    Serial.begin(115200);

    Serial.println("===================");
    Serial.println("Firmware: itr_demo v0");

    Serial.println("Configuration:");
    Serial.print(" - DRIVER_MICROSTEPPING    ");
    Serial.println(DRIVER_MICROSTEPPING);
    Serial.print(" - MOTOR_STEP_PER_ROTATION ");
    Serial.println(MOTOR_STEP_PER_ROTATION);
    Serial.print(" - MIN_SPEED_MOTOR_RPM     ");
    Serial.println(MIN_SPEED_MOTOR_RPM);
    Serial.print(" - MAX_SPEED_MOTOR_RPM     ");
    Serial.println(MAX_SPEED_MOTOR_RPM);

    Serial.print(" - ITR_FREQ_IN_HZ          ");
    Serial.print(MIN_SPEED_ITR_FREQ_IN_HZ);
    Serial.print(" .. ");
    Serial.println(MAX_SPEED_ITR_FREQ_IN_HZ);
    Serial.print(" - ITR_PERIOD_IN_NS        ");
    Serial.print(MIN_SPEED_ITR_PERIOD_IN_NS);
    Serial.print(" .. ");
    Serial.println(MAX_SPEED_ITR_PERIOD_IN_NS);

    Serial.print(" - TIMER1_COUNT            ");
    Serial.print(MIN_SPEED_TIMER1_COUNT);
    Serial.print(" .. ");
    Serial.println(MAX_SPEED_TIMER1_COUNT);

    Serial.println("Setup done");
}

uint32_t lastLedSwitchTime = 0;
bool lastLedState = LOW;

uint32_t lastLogTime = 0;

volatile uint32_t timer1ItrCounter = 0;

void loop() {
    uint32_t loopTime = micros();

    // blink led
    uint32_t ledSwitchPeriod = LED_SWITCH_PERIOD_IN_US;
    if (periodical(loopTime, ledSwitchPeriod, &lastLedSwitchTime)) {
        lastLedState = !lastLedState;
        digitalWrite(LED_BUILTIN, lastLedState);
    }

    // produce logs
    if (periodical(loopTime, LOG_PERIOD_IN_US, &lastLogTime)) {
        Serial.println("---");
        Serial.print("LoopTime : ");
        Serial.println(loopTime);

        uint32_t counterValue;
        counterValue = timer1ItrCounter;
        timer1ItrCounter -= counterValue;
        Serial.print("timer1ItrCounter : ");
        Serial.println(counterValue);
    }
}

ISR(TIMER1_COMPA_vect) {
    timer1ItrCounter++;
}