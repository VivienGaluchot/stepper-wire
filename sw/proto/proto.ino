#include "pin.h"

// Configuration

#define MAX_ROTATION_PER_MIN 300

#define LED_SWITCH_PERIOD_IN_US 1000000
#define LED_FAST_SWITCH_PERIOD_IN_US 100000
#define LOG_PERIOD_IN_US 1000000

#define SECOND_PER_MIN 60
#define STEP_PER_ROTATION 200
#define US_PER_S 1000000

const unsigned long MAX_FREQ_IN_HZ = (unsigned long)MAX_ROTATION_PER_MIN * (unsigned long)STEP_PER_ROTATION / SECOND_PER_MIN;

// Speed input

#define IN_MAX_SPEED 980

int readInSpeed() {
    int value = analogRead(PIN_IN_SPEED);
    if (value < 20) {
        value = 20;
    }
    value = value - 20;
    if (value > IN_MAX_SPEED) {
        value = IN_MAX_SPEED;
    }
    return value;
}

// Stepper drivers

void initialize(enum Channel_T channel) {
    pinMode(PIN_MAP[channel][PIN_STEP], OUTPUT);
    pinMode(PIN_MAP[channel][PIN_DIR], OUTPUT);
    pinMode(PIN_MAP[channel][PIN_EN], OUTPUT);
    reset(channel);
}

void reset(enum Channel_T channel) {
    setEnable(channel, false);
    setDir(channel, true);
    setStep(channel, false);
}

void setEnable(enum Channel_T channel, bool isEnabled) {
    digitalWrite(PIN_MAP[channel][PIN_EN], !isEnabled);
}

void setDir(enum Channel_T channel, bool isClockwise) {
    digitalWrite(PIN_MAP[channel][PIN_DIR], isClockwise);
}

void setStep(enum Channel_T channel, bool isHigh) {
    digitalWrite(PIN_MAP[channel][PIN_STEP], isHigh);
}

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

void setup() {
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIN_IN_SPEED, INPUT);

    initialize(CHANNEL_1);
    initialize(CHANNEL_2);
    setDir(CHANNEL_1, false);
    setDir(CHANNEL_2, true);
    setEnable(CHANNEL_1, false);
    setEnable(CHANNEL_2, false);

    Serial.println("===================");
    Serial.println("Firmware: proto v0");

    Serial.println("Configuration:");
    Serial.print(" - MAX_FREQ_IN_HZ : ");
    Serial.println(MAX_FREQ_IN_HZ);

    Serial.println("Setup done");
}

unsigned long lastLedSwitchTime = 0;
bool lastLedState = LOW;

unsigned long lastLogTime = 0;

unsigned long cycleCount = 0;

unsigned long lastStepSwitchTime = 0;
unsigned long missedStepSwitch = 0;
// bool lastStepState = LOW;
unsigned long stepSwitchCount = 0;

bool wasEnabled = false;
bool wasTurning = false;

void loop() {
    unsigned long loopTime = micros();

    // get speed
    int inSpeed = readInSpeed();

    // toggle EN
    bool isEnabled = (!wasEnabled && inSpeed > 10 || wasEnabled && inSpeed > 0);
    wasEnabled = isEnabled;
    setEnable(CHANNEL_1, isEnabled);
    setEnable(CHANNEL_2, isEnabled);

    // toggle STEP
    unsigned long currentFreq = (MAX_FREQ_IN_HZ * (unsigned long)inSpeed) / (unsigned long)IN_MAX_SPEED;

    unsigned long periodInUs = 0;
    if ((!wasTurning && currentFreq > 40) || (wasTurning && currentFreq > 20)) {
        periodInUs = US_PER_S / (2 * currentFreq);
        if (periodical(loopTime, periodInUs, &lastStepSwitchTime, &missedStepSwitch)) {
            // lastStepState = !lastStepState;
            setStep(CHANNEL_1, HIGH);
            setStep(CHANNEL_2, HIGH);
            setStep(CHANNEL_1, LOW);
            setStep(CHANNEL_2, LOW);
            stepSwitchCount++;
        }
        wasTurning = true;
    } else {
        lastStepSwitchTime = loopTime;
        wasTurning = false;
    }

    // blink led
    unsigned long ledSwitchPeriod = LED_SWITCH_PERIOD_IN_US;
    if (isEnabled) {
        ledSwitchPeriod = LED_FAST_SWITCH_PERIOD_IN_US;
    }
    if (periodical(loopTime, ledSwitchPeriod, &lastLedSwitchTime)) {
        lastLedState = !lastLedState;
        digitalWrite(LED_BUILTIN, lastLedState);
    }

    // produce logs
    if (periodical(loopTime, LOG_PERIOD_IN_US, &lastLogTime)) {
        Serial.println("---");
        Serial.print("LoopTime : ");
        Serial.println(loopTime);

        Serial.print("cycle : ");
        Serial.println(cycleCount);
        cycleCount = 0;

        Serial.print("inSpeed : ");
        Serial.println(inSpeed);

        Serial.print("Enabled : ");
        Serial.println(isEnabled);

        Serial.print("Step freq : ");
        Serial.println(currentFreq);

        Serial.print("Half switch period : ");
        Serial.println(periodInUs);

        Serial.print("Half switch count : ");
        Serial.println(stepSwitchCount);
        stepSwitchCount = 0;

        Serial.print("Missed half switch count : ");
        Serial.println(missedStepSwitch);
        missedStepSwitch = 0;
    }

    cycleCount += 1;
}
