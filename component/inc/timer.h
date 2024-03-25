/**
 * @file "timer.h"
 * @author Chandler Scott 
 * @brief Simple Timer 
 * @date 2024
 *
 * @copyright Copyright (c) 2024 The MITRE Corporation
 */


#ifndef TIMER_H
#define TIMER_H

#include <stdio.h>
#include "tmr.h"

#define CONT_TIMER MXC_TMR1 // Can be MXC_TMR0 through MXC_TMR5
#define CONT_FREQ 2 // (Hz)
#define CONT_CLOCK_SOURCE MXC_TMR_8M_CLK // \ref mxc_tmr_clock_t

void init_timer();
void reset_timer();
uint32_t get_timer_count();
void increment_timer_count();

#endif /* TIMER_H */
