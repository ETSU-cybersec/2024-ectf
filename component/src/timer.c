/**
 * @file "timer.c"
 * @author Chandler Scott 
 * @brief Simple hardware timer implementation 
 * @date 2024
 *
 * @copyright Copyright (c) 2024 The MITRE Corporation
 */

#include "timer.h"


void init_timer() {
    // Initialize the timer configuration
    uint32_t period_ticks = MXC_TMR_GetPeriod(CONT_TIMER, CONT_CLOCK_SOURCE, 128, CONT_FREQ);

    // Additional configuration if needed
    mxc_tmr_cfg_t tmr = {
        .pres = TMR_PRES_128,
        .mode = TMR_MODE_COUNTER,
        .bitMode = TMR_BIT_MODE_32,
        .clock = CONT_CLOCK_SOURCE,
        .cmp_cnt = period_ticks,
    };

    // Checking Timer Initialization
    if (MXC_TMR_Init(CONT_TIMER, &tmr, true) != E_NO_ERROR) {
        printf("Failed Counter Mode timer Initialization.\n");
    }

    // reset timer to 0
    reset_timer();
}

void set_timer(uint8_t count) {
    MXC_TMR_SetCount(CONT_TIMER, count);
}


// reset the timer count
void reset_timer() {
    MXC_TMR_SetCount(CONT_TIMER, 0);
}

// return the current count of the timer
uint32_t get_timer_count() {
    return MXC_TMR_GetCount(CONT_TIMER);
}

// increment the timer count
void increment_timer_count() {
    MXC_TMR_SetCount(CONT_TIMER, get_timer_count() + 1);
}
