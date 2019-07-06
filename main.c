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

#define PULSE_PIN PB3
#define INDICATOR_PIN PB0
#define INT0_PIN PB2
// PB1 is used as CLKI external clock input

void loop(void);

volatile uint8_t powerState = PWR_STARTUP;
volatile uint8_t timerState = PWR_TIMER_OFF;

volatile uint8_t pollCount;

int main(void) {
  cli();

  power_adc_disable(); // power saving setting
  // TODO: check for other software power saving settings

  CCP = 0xD8; // Signature to disable change protection for four clock cycles
  CLKPS = 0; // Set clock prescale divider to 1
  CLKMSR = 0b10; // Enable the external clock at 38.4 kHz
  _NOP();
  _NOP();

// TODO: TCCR0B - Set to the external clock - Why?
// TODO: Disable internal clocks to save power

  DDRB = (1<<PULSE_PIN) | (1 << INDICATOR_PIN); // set pins for output
  // Pull up INT0
  PORTB = (1<<INT0_PIN);

  _NOP();// nop to sync the pullups before enabling INT0+other interrupts

  enableInt0();

  sei();
  loop();
}

/*
About timer and power states:

timerState is a flag indicating the state of the power-on/off timer. the _INIT
mode is used to start a power-on/off sequence and the flag is immediately flipped to
TIMER_ON. Once we are in TIMER_ON, we simply go to sleep and get woken up about
10 times per second to poll the power button state. If the button has been released,
then we set the state to TIMER_OFF, stopping the power sequence. If the button is
held down for 28 counts (3 seconds), then we set the power flag (NOT the timer flag)
to startup or shutdown mode, as appropriate.

*/
void loop(void) {
  while (1) {
    if (timerState == PWR_TIMER_INIT) {
      // Initialize the timer and enter the polling loop state.
      cli();

      timerState = PWR_TIMER_ON;

      /*
      Run timer clock polling ~8-10 times per second for button release
      38400 clock speed, need to poll every 3840 to 4800 clocks

      Poll counts by prescaler:
      1x    -> 3840 to 4800
      8x    -> 480 to 600
      64x   -> 60 to 75
      256x  -> 15 to 18.75
      1024x -> 3.75 to 4.69
      Total Poll Cycles: 30 to 24 in all cases
      Use 1024 prescaller, count to 4, 28 total polling checks = 2.98 seconds
      */

      // clear and set WGM02/03 to set CTC mode
      TCCR0A = TCCR0B = 0;
      TCCR0B &= ~(0b11 << WGM02);
      TCCR0B |= 0b01 << WGM02;

      TIMSK0 = 1<<OCIE0A; // Enable output compare interrupt A

      // Set CTC mode, OCR0A = 4 to reset the counter to zero after 4 counts, and fire the interupt
      OCR0AH = 0;
      OCR0AL = 4;
      // We will use OCR0B for the pulse train. see pulse.c

      pollCount = 28; // button poll count

      // Run clock, prescale 1024
      TCCR0B |= 0b101;

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
          // CPU will stay asleep at this point until INT0 is triggered
          sleep_disable();
          break;

        case PWR_STARTUP:
          disableInt0();
          pwm_ramp(0, 252, 3000); // why not 250? 252 causes rounding error in ms_per_step
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

/* Interupt handling power on/off button polling */
ISR(TIM0_COMPA_vect) {
  cli();

  if (PINB & (1<<INT0_PIN)) { // Has the INT0 button been released?
    // TODO
    // TIMSK = 0;
    // TCCR1 = 0; // turn off the timer
    PORTB &= ~(1<<INDICATOR_PIN); // turn off indicator

    timerState = PWR_TIMER_OFF;
    enableInt0();
  } else if (!--pollCount) { // countdown complete
    // TODO
    // TIMSK = 0;
    // TCCR1 = 0; // turn off the timer

    PORTB &= ~(1<<INDICATOR_PIN); // turn off indicator

    powerState |= PWR_STARTUP;
    // Equivalent to the following, see flags in power_state.h
    // if (powerState == PWR_OFF) {
    //   powerState = PWR_STARTUP;
    // } else if (powerState == PWR_ON){
    //   powerState = PWR_SHUTDOWN;
    // }
    timerState = PWR_TIMER_OFF;
  }
  sei();
}
