#include <Arduino.h>
#include <avr/interrupt.h>

#include "pins.h"
#include "timer1.h"

// -------------------------------------------------------------
// Configuration
// -------------------------------------------------------------

// Led

#define IDLE_LED_SWITCH_PERIOD_IN_US 1000000
#define ENABLED_LED_SWITCH_PERIOD_IN_US 200000
#define ROTATING_LED_SWITCH_PERIOD_IN_US 50000

// Monitoring

#define IS_SERIAL_LOG_ENABLED true
#define SERIAL_LOG_PERIOD_IN_US 1000000

// Drivers

#define DRIVER_MS1_STATE HIGH
#define DRIVER_MS2_STATE LOW
// one motor step is 2 driver step according to the MS1 and MS2
// pin configurations
#define DRIVER_MICROSTEPPING 2UL

// Motor

// number of step per turn in the motor
#define MOTOR_STEP_PER_ROTATION 200UL

// minimal speed in rpm
#define MIN_SPEED_MOTOR_RPM 6UL
// maximal speed in rpm
#define MAX_SPEED_MOTOR_RPM 300UL

// -------------------------------------------------------------
// ITR period computation
// -------------------------------------------------------------

#define SEC_PER_MIN 60ULL
#define ITR_PER_STEP 1ULL

#define RPM_TO_ITR_FREQ_IN_HZ(x) (x * DRIVER_MICROSTEPPING * ITR_PER_STEP * MOTOR_STEP_PER_ROTATION / SEC_PER_MIN)
#define FREQ_IN_HZ_TO_PERIOD_IN_NS(x) (1000000000ULL / x)

const uint32_t MIN_SPEED_ITR_FREQ_IN_HZ = RPM_TO_ITR_FREQ_IN_HZ(MIN_SPEED_MOTOR_RPM);
const uint32_t MAX_SPEED_ITR_FREQ_IN_HZ = RPM_TO_ITR_FREQ_IN_HZ(MAX_SPEED_MOTOR_RPM);

STATIC_CHECK_FREQ_IN_RANGE(MIN_SPEED_ITR_FREQ_IN_HZ);
STATIC_CHECK_FREQ_IN_RANGE(MAX_SPEED_ITR_FREQ_IN_HZ);

const uint32_t MIN_SPEED_ITR_PERIOD_IN_NS = FREQ_IN_HZ_TO_PERIOD_IN_NS(MIN_SPEED_ITR_FREQ_IN_HZ);
const uint32_t MAX_SPEED_ITR_PERIOD_IN_NS = FREQ_IN_HZ_TO_PERIOD_IN_NS(MAX_SPEED_ITR_FREQ_IN_HZ);

const uint32_t MIN_SPEED_TIMER1_COUNT = TIMER1_COUNT_FOR_PERIOD_IN_NS(MIN_SPEED_ITR_PERIOD_IN_NS);
const uint32_t MAX_SPEED_TIMER1_COUNT = TIMER1_COUNT_FOR_PERIOD_IN_NS(MAX_SPEED_ITR_PERIOD_IN_NS);

// -------------------------------------------------------------
// Private variables
// -------------------------------------------------------------

uint32_t timer1ItrCounter = 0;
uint32_t speedAdjustCounter = 0;

uint32_t lastLedSwitchTime = 0;
bool lastLedState = LOW;

uint32_t lastLogTime = 0;

bool lastCycleEnabled = false;
bool lastCycleRotating = false;
uint16_t lastFreqHandPot = 0;

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

void timerCallback() {
    setStep(CHANNEL_1, true);
    setStep(CHANNEL_2, true);
    setStep(CHANNEL_1, false);
    setStep(CHANNEL_2, false);
    timer1ItrCounter++;
}

// -------------------------------------------------------------
// Public services
// -------------------------------------------------------------

void setup() {
    initializePins();
    setEnable(CHANNEL_1, false);
    setEnable(CHANNEL_2, false);
    timer1::disable();

    Serial.begin(115200);
    Serial.println("===================");
    Serial.println("Firmware: itr-stepper-wire v0.1");

    Serial.println("Configuration:");
    Serial.print(" - DRIVER_MICROSTEPPING    ");
    Serial.println(DRIVER_MICROSTEPPING);
    Serial.print(" - MOTOR_STEP_PER_ROTATION ");
    Serial.println(MOTOR_STEP_PER_ROTATION);
    Serial.print(" - SPEED_MOTOR_RPM         ");
    Serial.print(MIN_SPEED_MOTOR_RPM);
    Serial.print(" .. ");
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

    // sleep to wait for power suply stabilisation
    delay(500);

    setDirection(CHANNEL_1, true);
    setDirection(CHANNEL_2, false);
    setMs1(CHANNEL_1, DRIVER_MS1_STATE);
    setMs1(CHANNEL_2, DRIVER_MS1_STATE);
    setMs2(CHANNEL_1, DRIVER_MS2_STATE);
    setMs2(CHANNEL_2, DRIVER_MS2_STATE);

    // Serial.println("Setup done");
}

void loop() {
    uint32_t loopTime = micros();

    // read inputs
    uint16_t handPot = readHandPot();

    // compute cycle state
    bool isEnabled = handPot > (lastCycleEnabled ? 0 : 100);
    bool isRotating = handPot > (lastCycleRotating ? 100 : 200);

    // set driver enable
    if (isEnabled != lastCycleEnabled) {
        setEnable(CHANNEL_1, isEnabled);
        setEnable(CHANNEL_2, isEnabled);
    }

    // set timer period
    if (isRotating && !lastCycleRotating) {
        timer1::setFrequency(MIN_SPEED_ITR_FREQ_IN_HZ);
        timer1::enable(&timerCallback);
    } else if (!isRotating && lastCycleRotating) {
        timer1::disable();
    }

    if (isRotating && lastFreqHandPot != handPot) {
        const uint32_t maxSpeed = MAX_POT_VALUE - 200;
        uint32_t speed = handPot >= 200 ? handPot - 200 : 0;
        uint32_t frequency = speed * (MAX_SPEED_ITR_FREQ_IN_HZ - MIN_SPEED_ITR_FREQ_IN_HZ) / maxSpeed + MIN_SPEED_ITR_FREQ_IN_HZ;
        timer1::setRampFrequency(frequency, 2000);
        speedAdjustCounter++;
    }

    lastCycleEnabled = isEnabled;
    lastCycleRotating = isRotating;
    lastFreqHandPot = handPot;

    // blink led
    uint32_t ledSwitchPeriod = IDLE_LED_SWITCH_PERIOD_IN_US;
    if (isEnabled) {
        ledSwitchPeriod = ENABLED_LED_SWITCH_PERIOD_IN_US;
    }
    if (isRotating) {
        ledSwitchPeriod = ROTATING_LED_SWITCH_PERIOD_IN_US;
    }
    if (periodical(loopTime, ledSwitchPeriod, &lastLedSwitchTime)) {
        lastLedState = !lastLedState;
        digitalWrite(LED_BUILTIN, lastLedState);
    }

    // produce logs
    if (IS_SERIAL_LOG_ENABLED && periodical(loopTime, SERIAL_LOG_PERIOD_IN_US, &lastLogTime)) {
        Serial.println("---");
        Serial.print("time        : ");
        Serial.println(loopTime);

        Serial.print("speed adjust: ");
        Serial.println(speedAdjustCounter);
        speedAdjustCounter = 0;

        Serial.print("hand pot    : ");
        Serial.println(handPot);

        uint32_t counterValue = timer1ItrCounter;
        timer1ItrCounter -= counterValue;
        Serial.print("itr count   : ");
        Serial.println(counterValue);

        Serial.print("timer1 freq : ");
        Serial.println(timer1::getFrequencyInHz());

        int32_t flushed = timer1::popFlushedTicks();
        if (flushed != 0) {
            Serial.print("warning, timer1 flushed ticks: ");
            Serial.println(flushed);
        }
    }
}
