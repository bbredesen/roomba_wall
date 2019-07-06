// #include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>

#include "power_state.h"
#include "pwm_ramp.h"

volatile uint16_t pwm_cycles_per_step;
volatile uint16_t cycle_count;

// right now this will take over execution. Can/should it be rewritten to work asynchronously?
/* Using phase+frequency correct PWM mode, we will use an 8x prescaler and count
up to TOP=255. The current ramp setting is set as TCNT0. For example, if ramp=200
then OC0B is high while the counter is between 0 and 200, then low for counter
from 201 to 255 and back to 201, then high down to zero and back up, etc.

The number of PWM cycles is determined by the ms per step on the ramp. Ramp cycle
count is (2 * F_CPU / TOP) -> 1 / (38400 / 255 / 2) = 13.3 ms - Only 75 Hz, that
will not work for LED blinking. Setting TOP lower seems like the only good option.
Setting it at 64 will give 300 Hz PWM frequency, which is about the lowest I think
you would want to go, and it works out nicely with counts on a 256 scale.

Or: Fast PWM mode, TOP = 128. TCNT0 = step / 2, output is ~OCR0B. At this low
of a freq, maybe it changes to a step increment ("jump") if ms_per_step is less than one
count cycle? One count cycle is 3.3 ms, 909 count cycles over 3000 ms, 7.27 ramps
before TCNT0 jumps by 2. In other words, at the start of each count, we should
look at the number of counts and where TCNT0 should be set.

TOP=128
F_CPU=38.4kHz
n_count = 0;
max_count = (F_CPU / TOP) * time_ms / 1000 -> (38400 / 128) * 3000 / 1000 = 900
> at bottom interupt, ++n_count;
TCNT0 = (end-start) / time_ms / 2; // 3000 ms / 250 steps = 250 steps
900 cycles / 250 steps = 3.6 cycles / step
At cycle count x, we are on step n (TCNT0 = n):
x = 900 * n / 250
n = 125 * x / 900 => TCNT0 = (end-start) * TOP / 256 * n_count / max_count <= 256 constant because that is the range of start/end inputs
x:10, n = 2.778 = 2
x:11, n = 3.056 = 3
x:12, n = 3.333 = 3
x:13, n = 3.611 = 3
x:14, n = 3.889 = 3
x:15, n = 4.167 = 4

Main issue: the calculation will take several clock cycles on each interrupt, which means we will miss low counts.
(i.e., the ramp will jump from zero to something like 4)
The clock can be temporarily stopped with TCCR0B, but that will extend the total ramp time by the number of
interupt cycles.

*/
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
  /*  2 * ORC0A/TOP clocks per pwm cycle
      F_CPU / 1,000 clocks per millisecond
      1000/512*ms_per_step clocks per step
      e.g., 100 steps over 1 second, each step takes 10 ms. (1000 clocks) each pwm cycle takes
      approx. 2 ms, total cycles in the step: 10 ms / step  / 1000/512 ms / cycle = 5 cycles per step

      cycles per ms = F_CPU / 500 / 1000 =
  */
  // This doesn't work for 38.4 khz...calculates to 0.8448, which will truncate to zero with integer math
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
  if (powerState & 0b10) { // 0xb10 = PWR_STARTUP or PWR_SHUTDOWN mode
    cli();
    if (!--cycle_count) { // decrement, only enter the case when zero
      cycle_count = pwm_cycles_per_step;
      OCR0B++;
    }
  }
  // sei();
}
