#include <Arduino.h>
#include <avr/interrupt.h>

#include "pins.h"
#include "timer1.h"

// -------------------------------------------------------------
// Time
// -------------------------------------------------------------

#define SEC_PER_MIN 60ULL

#define USEC_PER_SEC 1000000UL

// -------------------------------------------------------------
// Configuration
// -------------------------------------------------------------

// Led

#define IDLE_LED_SWITCH_PERIOD_IN_US 1000000
#define ENABLED_LED_SWITCH_PERIOD_IN_US 100000

// Monitoring

#define IS_SERIAL_LOG_ENABLED
#define SERIAL_LOG_PERIOD_IN_US 1000000

// Drivers

#define DRIVER_MS1_STATE HIGH
#define DRIVER_MS2_STATE LOW
// one motor step is 2 driver step according to the MS1 and MS2
// pin configurations
#define DRIVER_MICROSTEPPING 2UL

#define DRIVER_KEEP_ENABLED_IN_US (5 * USEC_PER_SEC)

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
uint32_t cycles = 0;

uint32_t lastLedSwitchTime = 0;
bool lastLedState = LOW;

uint32_t lastLogTime = 0;

bool isLastCycleFootPressed = false;
bool isLastCycleEnabled = false;
bool isLastCycleRotating = false;

bool isLastRotatingCycleSet = false;
uint32_t lastRotatingCycle = 0;

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
#ifdef IS_SERIAL_LOG_ENABLED
    Serial.println("===================");
    Serial.println("Firmware: itr-stepper-wire v0.2");

    Serial.println("Configuration:");
    Serial.print(" - DRIVER_MICROSTEPPING      ");
    Serial.println(DRIVER_MICROSTEPPING);
    Serial.print(" - MOTOR_STEP_PER_ROTATION   ");
    Serial.println(MOTOR_STEP_PER_ROTATION);
    Serial.print(" - SPEED_MOTOR_RPM           ");
    Serial.print(MIN_SPEED_MOTOR_RPM);
    Serial.print(" .. ");
    Serial.println(MAX_SPEED_MOTOR_RPM);
    Serial.print(" - DRIVER_KEEP_ENABLED_IN_US ");
    Serial.println(DRIVER_KEEP_ENABLED_IN_US);

    Serial.print(" - ITR_FREQ_IN_HZ            ");
    Serial.print(MIN_SPEED_ITR_FREQ_IN_HZ);
    Serial.print(" .. ");

    Serial.println(MAX_SPEED_ITR_FREQ_IN_HZ);
    Serial.print(" - ITR_PERIOD_IN_NS          ");
    Serial.print(MIN_SPEED_ITR_PERIOD_IN_NS);
    Serial.print(" .. ");
    Serial.println(MAX_SPEED_ITR_PERIOD_IN_NS);

    Serial.print(" - TIMER1_COUNT              ");
    Serial.print(MIN_SPEED_TIMER1_COUNT);
    Serial.print(" .. ");
    Serial.println(MAX_SPEED_TIMER1_COUNT);
#endif

    // sleep to wait for power supply stabilization
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
    uint16_t footPot = readFootPot();
    bool isFootPressed = footPot > (isLastCycleFootPressed ? 0 : 50);

    // compute cycle state
    bool isTimerStillRunning = timer1::getFrequencyInHz() > MIN_SPEED_ITR_FREQ_IN_HZ;
    bool isDriverKeepEnabled = isLastRotatingCycleSet && ((loopTime - lastRotatingCycle) < DRIVER_KEEP_ENABLED_IN_US);
    bool isEnabled = isFootPressed || isTimerStillRunning || isDriverKeepEnabled;
    bool isRotating = isFootPressed || isTimerStillRunning;

    // set driver enable
    if (isEnabled != isLastCycleEnabled) {
        setEnable(CHANNEL_1, isEnabled);
        setEnable(CHANNEL_2, isEnabled);
    }

    // set timer period
    if (isRotating && !isLastCycleRotating) {
        timer1::setFrequency(MIN_SPEED_ITR_FREQ_IN_HZ);
        timer1::enable(&timerCallback);
    } else if (!isRotating && isLastCycleRotating) {
        timer1::disable();
    } else if (isRotating) {
        uint32_t maxSpeed = MAX_POT_VALUE;
        uint32_t frequency = footPot * (MAX_SPEED_ITR_FREQ_IN_HZ - MIN_SPEED_ITR_FREQ_IN_HZ) / maxSpeed + MIN_SPEED_ITR_FREQ_IN_HZ;
        timer1::setRampFrequency(frequency, 2000);
    }

    // blink led
    uint32_t ledSwitchPeriod = IDLE_LED_SWITCH_PERIOD_IN_US;
    if (isEnabled) {
        ledSwitchPeriod = ENABLED_LED_SWITCH_PERIOD_IN_US;
    }
    if (periodical(loopTime, ledSwitchPeriod, &lastLedSwitchTime)) {
        lastLedState = !lastLedState;
        digitalWrite(LED_BUILTIN, lastLedState);
    }

#ifdef IS_SERIAL_LOG_ENABLED
    // produce logs
    if (periodical(loopTime, SERIAL_LOG_PERIOD_IN_US, &lastLogTime)) {
        Serial.println("---");
        Serial.print("time        : ");
        Serial.println(loopTime);

        Serial.print("cycles      : ");
        Serial.println(cycles);
        cycles = 0;

        Serial.print("foot pot    : ");
        Serial.println(footPot);

        Serial.print("enabled     : ");
        Serial.println(isEnabled);

        Serial.print("rotating    : ");
        Serial.println(isRotating);

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
    cycles++;
#endif

    // end of cycle
    isLastCycleFootPressed = isFootPressed;
    isLastCycleEnabled = isEnabled;
    isLastCycleRotating = isRotating;
    if (isRotating) {
        lastRotatingCycle = loopTime;
        isLastRotatingCycleSet = true;
    }
}
