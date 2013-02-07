#include "VGA.h"
#include <avr/pgmspace.h>
#include <avr/sleep.h>

// convenience
#define NOP asm volatile ("nop\n\t")

volatile int line_ctr;

// vsync interrupt
ISR (TIMER1_OVF_vect) {
  line_ctr = -35; // porch is 35 lines
}

// hsync interrupt (wakes us from sleep)
ISR (TIMER2_OVF_vect) {
  line_ctr++;
}

VGA::VGA() :
  _interrupt(NULL) {
}

void VGA::begin() {
  // kill timer 0
  TIMSK0 = 0;
  OCR0A  = 0;
  OCR0B  = 0;
  // timer 1 - vsync
  pinMode(10, OUTPUT);
  TCCR1A = (_BV(WGM10) | _BV(WGM11)) | _BV(COM1B1);
  TCCR1B = (_BV(WGM12) | _BV(WGM13)) | 5;
  OCR1A  = 259;           // 16666/64µS=260
  OCR1B  = 0;             // 64/64µS=1
  TIFR1  = _BV (TOV1);
  TIMSK1 = _BV (TOIE1);  // ovf int for timer 1
  // timer 2 - hsync
  pinMode(3, OUTPUT);
  TCCR2A = (_BV(WGM20) | _BV(WGM21)) | _BV(COM2B1);
  TCCR2B = (_BV(WGM22)) | 2;
  OCR2A  = 63;            // 32/0.5µS=64
  OCR2B  = 5;             // 4 /0.5µS=8
  TIFR2  = _BV (TOV2);
  TIMSK2 = _BV (TOIE2);  // ovf int for timer 2
  // pixel data
  pinMode(0, OUTPUT); pinMode(1, OUTPUT); pinMode(4, OUTPUT);
  pinMode(5, OUTPUT); pinMode(6, OUTPUT); pinMode(7, OUTPUT);
  // set sleep mode (int will wake)
  set_sleep_mode(SLEEP_MODE_IDLE);
}

void VGA::attachInterrupt(void (*interrupt)(void)) {
  _interrupt = interrupt;
}

void VGA::scanLine() {
  sleep_mode();
  if(line_ctr >= 0 && line_ctr < 480) {
    // this will draw a single scanline
    register byte i = width;
    register byte bitmap_y = line_ctr >> 4;
    register byte *p_bitmap = &pixels[bitmap_y][0];
    while (i--) {
      PORTD = *p_bitmap++;
      NOP; NOP; NOP;
    }
    NOP; NOP; NOP;
    PORTD = 0;
  } else if(line_ctr == 480) {
    // application code - executed on each frame
    // runtime must not exceed 45 lines = ~1.4mS
    if (_interrupt) {
      _interrupt();
    }
  }
}
