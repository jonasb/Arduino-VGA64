#include <avr/pgmspace.h>
#include <avr/sleep.h>

// constants
#define X_PIXELS    40
#define Y_PIXELS    30

// convenience
#define NOP asm volatile ("nop\n\t")

volatile int line_ctr;
byte bitmap[Y_PIXELS][X_PIXELS];

// vsync interrupt
ISR (TIMER1_OVF_vect) {
  line_ctr = -35; // porch is 35 lines
}

// hsync interrupt (wakes us from sleep)
ISR (TIMER2_OVF_vect) {
  line_ctr++;
}

#define COLUMN_COUNT  10
#define ROW_COUNT 22
const byte block_height = Y_PIXELS / ROW_COUNT;
const byte block_width = block_height;
const byte playfield_offset_y = (Y_PIXELS - ROW_COUNT * block_height) / 2;
const byte playfield_offset_x = (X_PIXELS - COLUMN_COUNT * block_width) / 2;

byte playfield[ROW_COUNT * COLUMN_COUNT];
enum BlockType {
  BLOCK_NONE,
  BLOCK_I,
  BLOCK_O,
  BLOCK_T,
  BLOCK_S,
  BLOCK_Z,
  BLOCK_J,
  BLOCK_L,
};
const byte block_colors[] = {
  0b00000001,
  0b11110000,
  0b00110011,
  0b11000010,
  0b00110001,
  0b00000011,
  0b11100000,
  0b00100011,
};
byte current_block = 0;
byte current_col = COLUMN_COUNT / 2;
byte current_row = 0;

byte time = 0;

void setup() {
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
  // setup playfield
  memset(playfield, BLOCK_NONE, ROW_COUNT * COLUMN_COUNT);
  draw_field();
}

void loop() {
  sleep_mode();
  if(line_ctr >= 0 && line_ctr < 480) {
    // this will draw a single scanline
    register byte i = X_PIXELS;
    register byte bitmap_y = line_ctr >> 4;
    register byte *p_bitmap = &bitmap[bitmap_y][0];
    while (i--) {
      PORTD = *p_bitmap++;
      NOP; NOP; NOP;
    }
    NOP; NOP; NOP;
    PORTD = 0;
  } else if(line_ctr == 480) {
    // application code - executed on each frame
    // runtime must not exceed 45 lines = ~1.4mS
    ++time;
    if (time == 20) {
      time = 0;
      if (current_row == ROW_COUNT - 1) {
        current_row = 0;
        current_block = 1 + (current_block % 7);
        playfield[current_col] = current_block;
        draw_block(current_col, current_row);
      } else {
        byte i = current_row * COLUMN_COUNT + current_col;
        byte block = playfield[i];
        if (block == BLOCK_NONE) // first time
          block = current_block = BLOCK_I;
        playfield[i] = BLOCK_NONE;
        draw_block(current_col, current_row);
        ++current_row;
        i += COLUMN_COUNT;
        playfield[i] = block;
        draw_block(current_col, current_row);
      }
    }
  }
}

void draw_field() {
  for (byte row = 0; row < ROW_COUNT; row++) {
    for (byte col = 0; col < COLUMN_COUNT; col++) {
      draw_block(col, row);
    }
  }
}

void draw_block(byte col, byte row) {
  const byte color = block_colors[playfield[row * COLUMN_COUNT + col]];
  const byte y_offset = playfield_offset_y + row * block_height;
  const byte x_offset = playfield_offset_x + col * block_width;
  for (byte y = 0; y < block_height; y++) {
    memset(bitmap[y_offset + y] + x_offset, color, block_width);
  }
}
