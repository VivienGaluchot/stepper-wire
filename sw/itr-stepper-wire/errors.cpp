#include "errors.h"

#include <Arduino.h>

// -------------------------------------------------------------
// Public services
// -------------------------------------------------------------

void errorTrap(int code) {
    Serial.print("-- error trap: ");
    Serial.println(code);
    delay(100);
    noInterrupts();
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    while (true) {
        // do nothing until reset
    }
}