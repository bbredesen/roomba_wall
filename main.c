#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/sleep.h>

#include "power_state.h"
#include "pulse.h"
#include "pwm_ramp.h"

#define disableInt0() GIMSK &= ~(1<<INT0)
#define enableInt0() GIMSK |= (1<<INT0)

#define PULSE_PIN 0
#define INDICATOR_PIN 1
#define INT0_PIN 2

void loop(void);

volatile uint8_t powerState = PWR_STARTUP;
volatile uint8_t timerState = PWR_TIMER_OFF;

volatile uint8_t pollCount;

int main(void) {
  cli();

  power_adc_disable(); // power saving setting

  DDRB = (1<<PULSE_PIN) | (1 << INDICATOR_PIN); // set pins for output
  // Pull up unused pins PB3/4/5 (see attiny85 datasheet, section 10.2.6) plus the INT0 switch
  // plus INT0
  PORTB = (1<<PB3) | (1<<PB4) | (1<<PB5) | (1<<INT0_PIN);

  _NOP();// nop to sync the pullups before enabling INT0+other interrupts

  enableInt0();

  sei();
  loop();
}

void loop(void) {
  while (1) {
    if (timerState == PWR_TIMER_INIT) {
      // Initialize the timer and enter the polling loop state.
      cli();

      timerState = PWR_TIMER_ON;

      TIMSK = (1<<TOIE1); // enable overflow interrupt
      TCCR1 = (0b1010 << CS10); // Run clock, prescale 512
      pollCount = 22; // 3 sec: 3e6 clocks / 512 prescale / 256 reg size = 22.89
      // Timer interrupt polls for button release ~8 times / second, or every 131 ms

      if (powerState == PWR_ON)
        PORTB |= (1<<INDICATOR_PIN); // turn on the led if we are currently powered up

      sei();
    }
    else if (timerState == PWR_TIMER_ON) {
      // If we are polling for button release, just go to sleep and wait for the next interrupt
      set_sleep_mode(SLEEP_MODE_IDLE);
      sleep_enable();
      sei();
      sleep_cpu();
      sleep_disable();
    }
    else if (timerState == PWR_TIMER_OFF) {
      switch (powerState & 0b11) {
        case PWR_OFF:
        // need to turn off adc, brown out detector to reduce power further
          cli();
          set_sleep_mode(SLEEP_MODE_PWR_DOWN);
          sleep_enable();
          sei();
          sleep_cpu();
          sleep_disable();
          break;

        case PWR_STARTUP:
          disableInt0();
          pwm_ramp(0, 252, 3000);
          powerState = PWR_ON;
          enableInt0();
          break;

        case PWR_ON:
          run_pulse_seq();
          break;

        case PWR_SHUTDOWN:
          disableInt0();
          pwm_ramp(192, 10, 3000);
          powerState = PWR_OFF;
          enableInt0();
          break;
      }
    }
  }
}

ISR(INT0_vect) {
  cli();
  disableInt0();
  timerState = PWR_TIMER_INIT;
  sei();
}

ISR(TIMER1_OVF_vect) {
  cli();

  if (PINB & (1<<INT0_PIN)) { // Has the INT0 button been released?
    TIMSK = 0;
    TCCR1 = 0; // turn off the timer
    PORTB &= ~(1<<INDICATOR_PIN); // turn off indicator

    timerState = PWR_TIMER_OFF;
    enableInt0();
  } else if (!--pollCount) { // countdown complete
    TIMSK = 0;
    TCCR1 = 0; // turn off the timer

    PORTB &= ~(1<<INDICATOR_PIN); // turn off indicator

    if (powerState == PWR_OFF) {
      powerState = PWR_STARTUP;
    } else if (powerState == PWR_ON){
      powerState = PWR_SHUTDOWN;
    }
    timerState = PWR_TIMER_OFF;
  }
  sei();
}
