#ifndef __POWER_STATE_H__
#define __POWER_STATE_H__

#define PWR_OFF       0b00
#define PWR_ON        0b01
#define PWR_STARTUP   0b10
#define PWR_SHUTDOWN  0b11

#define PWR_TIMER_OFF   0b0000
#define PWR_TIMER_INIT  0b0100
#define PWR_TIMER_ON    0b1000

volatile extern uint8_t powerState;

#endif // __POWER_STATE_H__
