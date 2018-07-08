// #include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>

#include "power_state.h"
#include "pwm_ramp.h"

volatile uint16_t pwm_cycles_per_step;
volatile uint16_t cycle_count;

// right now this will take over execution. Can/should it be rewritten to work asynchronously?
void pwm_ramp(uint8_t start, uint8_t end, uint16_t time_ms) {
  cli();

  uint8_t dir_bits = 0b10;

  if (end < start) {
    end = 255-end;
    start = 255-start;
    dir_bits = 0b11;
  }

  // DDRB = (1 << PB1); // enable output of the ramp signal on PB0/OC0A/pin 5
  OCR0A = 250; // set TOP=250, so that the cycles per step works out evenly
  OCR0B = start;

  TCCR0A = (dir_bits << COM0B0) | (0b01); // set up, clear down count; wgm[1:0]
  TCCR0B = (1<<WGM02);//WGM bit 02, WGM is mode 5
  TIMSK = (1<<TOIE0);// enable overflow interrupt

  uint16_t ms_per_step = time_ms / (end-start);
  /*  512 clocks per pwm cycle
      1,000 clocks per millisecond
      1000/512*ms_per_step clocks per step
      e.g., 100 steps over 1 second, each step takes 10 ms. (1000 clocks) each pwm cycle takes
      approx. 2 ms, total cycles in the step: 10 ms / step  / 1000/512 ms / cycle = 5 cycles per step

      cycles per ms = F_CPU / 500 / 1000 =
  */
  pwm_cycles_per_step = ms_per_step * (F_CPU / 1000 / 500); // 250x2, 500 per full cycle
  cycle_count = pwm_cycles_per_step;

  set_sleep_mode(SLEEP_MODE_IDLE);

  TCCR0B |= 0b1;//run the clock
  while (OCR0B < end) {
    sei();
    sleep_cpu();
    // cli();
  }
  TIMSK = 0; // disable overflow interrupt

  // _delay_ms(1000);

  // while (1) { }
  TCCR0A = 0;
  TCCR0B = 0;
  OCR0A = 0;
  OCR0B = 0;

  sei();
}

ISR(TIMER0_OVF_vect) {
  if (powerState & 0b10) {
    cli();
    if (!--cycle_count) { // decrement, only enter the case when zero
      cycle_count = pwm_cycles_per_step;
      OCR0B++;
    }
  }
  // sei();
}
