#ifndef __pwm_ramp_h__
#define __pwm_ramp_h__

extern volatile uint16_t cycle_count;
extern volatile uint16_t pwm_cycles_per_step;

void pwm_ramp(uint8_t start, uint8_t end, uint16_t time_ms);

#endif // __pwm_ramp_h__
