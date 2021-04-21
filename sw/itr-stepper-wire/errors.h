#ifndef ERROR_H
#define ERROR_H

#include <stdint.h>

// -------------------------------------------------------------
// Types
// -------------------------------------------------------------

enum Error_Code_T {
    TIMER_1_ITR_CMP_OUT_OF_RANGE = 1,
    TIMER_1_RAMPING_RATE_OUT_OF_RANGE = 2
};

// -------------------------------------------------------------
// Services
// -------------------------------------------------------------

void errorTrap(int code);

#endif
