#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>

#include "pulse.h"
#include "power_state.h"

#define CTC_COM0A_NONE  0b00000000
#define CTC_COM0A_TOG   0b01000000
#define CTC_COM0A_CLR   0b10000000
#define CTC_COM0A_SET   0b11000000

#define CTC_COM0B_NONE  0b00000000
#define CTC_COM0B_TOG   0b00010000
#define CTC_COM0B_CLR   0b00100000
#define CTC_COM0B_SET   0b00110000

#define WGM_CTC     0b00000010
#define WGMA_PCPWM  0b00000001
#define WGMB_PCPWM  0b00000100

#define CS_OFF        0b00000000
#define CS_RUN        0b00000001
#define CS_RUN_DIV8   0b00000010
#define CS_RUN_DIV64  0b00000011

#define CS1_OFF       0b00000000
#define CS1_RUN       0b00000001
#define CS1_RUN_DIV2  0b00000010
#define CS1_RUN_DIV4  0b00000011
#define CS1_RUN_DIV8  0b00000100
#define CS1_RUN_DIV16 0b00000101
#define CS1_RUN_DIV32 0b00000110
#define CS1_RUN_DIV64 0b00000111
#define CS1_RUN_DIV128 0b00001000
#define CS1_RUN_DIV256 0b00001001
#define CS1_RUN_DIV512 0b00001010
#define CS1_RUN_DIV1024 0b00001011

#define CTC_COM1A_NONE  0b00000000
#define CTC_COM1A_TOG   0b00010000
#define CTC_COM1A_CLR   0b00100000
#define CTC_COM1A_SET   0b00110000

static void sleep();
static void longDelay();
static void prePulse();
static void pulse();

void pulse_setup() {
  cli();
  // Set up Timer0
  OCR0A = 0x0C; // freq = clk_io / (2 x N x (1+OCR0A)), ~38.5 kHz = 0x0C
  TCCR0A = WGM_CTC | CTC_COM0A_TOG;

  // Set up Timer1
  TIMSK = (1 << OCIE1A); // Output compare interrupt enable 1A
  OCR1A = 0x7D; // Fires the interrupt
  OCR1C = 0x7D; // 7D/125 count, Fires the timer reset to zero

  set_sleep_mode(SLEEP_MODE_IDLE);

  sei();
}

void run_pulse_seq(void)
{
  pulse_setup();

  longDelay();
  for (int i = 0; i<3; i++) {
    prePulse();//7.5 ms low
    pulse();//0.5 ms pulsed
  }
  TCCR0A = WGM_CTC | CTC_COM0A_CLR;
  TCCR0B |= (1<<FOC0A);
}

volatile uint8_t loopCounter;

static inline void longDelay() {
  TCCR1 = (1<<CTC1) | CS1_RUN_DIV1024;
  // loopCounter = 31;
  // while (loopCounter) {
    sleep();
  // }
  // TCCR1 = (1<<CTC1) | CS1_RUN_DIV4;
  TCCR1 = 0;

}

static inline void prePulse() {
  // loopCounter = 15;
  TCCR1 = (1<<CTC1) | CS1_RUN_DIV64;

  // loopCounter = 16;
  // while (loopCounter) {
    sleep();
  TCCR1 = 0;
  // }
}

static inline void pulse() {
  TCCR1 = (1<<CTC1) | CS1_RUN_DIV4;
  TCCR0B = CS_RUN; // Activate clock with no prescaling, N=1
  sleep();
  PORTB &= ~(1<<0);
  TCCR0B = CS_OFF; // turn off and reset the pulse counter
  TCNT0 = 0;
  TCCR1 = 0;
}

static inline void sleep() {
  cli();
  sleep_enable();
  sei();
  sleep_cpu();
  sleep_disable();
  sei();
}

ISR(TIMER1_COMPA_vect) {
  loopCounter--;
}
