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
const byte playfield_offset_y = (Y_PIXELS - ROW_COUNT) / 2;
const byte playfield_offset_x = (X_PIXELS - COLUMN_COUNT) / 2;

byte playfield[ROW_COUNT][COLUMN_COUNT];
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
byte current_block = BLOCK_NONE;
byte current_col = 0;
byte current_row = 0;

byte block_bag[] = {
  BLOCK_I,
  BLOCK_O,
  BLOCK_T,
  BLOCK_S,
  BLOCK_Z,
  BLOCK_J,
  BLOCK_L,
};
#define BLOCK_BAG_COUNT 7
byte block_bag_index = BLOCK_BAG_COUNT - 1; // will cause bag randomization

#define KEY_REPEAT_FRAMES 10
byte key_consecutive_press_left = 0;
byte key_consecutive_press_right = 0;
byte key_consecutive_press_rotate = 0;

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
  //
  randomSeed(analogRead(0)); // analog 0 is unconnected
  // setup playfield
  memset(playfield, BLOCK_NONE, ROW_COUNT * COLUMN_COUNT);
  draw_field();
  current_block = get_next_block();
  // control
  pinMode(11, INPUT);
  digitalWrite(11, HIGH);
  pinMode(12, INPUT);
  digitalWrite(12, HIGH);
  pinMode(8, INPUT);
  digitalWrite(8, HIGH);
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

    read_key(12, key_consecutive_press_right);
    read_key(8, key_consecutive_press_left);
    read_key(11, key_consecutive_press_rotate);

    if (key_consecutive_press_right % KEY_REPEAT_FRAMES == 1) {
      move_block(1, 0);
    }
    if (key_consecutive_press_left % KEY_REPEAT_FRAMES == 1) {
      move_block(-1, 0);
    }

    if (time == 10) {
      time = 0;
      if (!move_block(0, 1)) {
        // Block is stuck
        collapse_full_rows();

        current_row = 0;
        current_col = 0;
        current_block = get_next_block();
        update_block(current_col, current_row, current_block);
      }
    }
  }
}

void read_key(int pin, byte &key_consecutive_press) {
  if (digitalRead(pin) == LOW) {
    ++key_consecutive_press; //TODO 0xff
  } else {
    key_consecutive_press = 0;
  }
}

byte get_next_block() {
  ++block_bag_index;
  if (block_bag_index == BLOCK_BAG_COUNT) {
    block_bag_index = 0;
    // randomize bag
    for (byte i = 0; i < BLOCK_BAG_COUNT - 1; i++) {
      byte j = random(i + 1, BLOCK_BAG_COUNT);
      byte t = block_bag[j];
      block_bag[j] = block_bag[i];
      block_bag[i] = t;
    }
  }
  return block_bag[block_bag_index];
}

boolean move_block(byte dx, byte dy) {
  byte next_col = current_col + dx;
  byte next_row = current_row + dy;
  if (next_col < 0 || next_col >= COLUMN_COUNT || next_row >= ROW_COUNT) {
    return false;
  }
  if (playfield[next_row][next_col] != BLOCK_NONE) {
    return false;
  }
  update_block(current_col, current_row, BLOCK_NONE);
  current_col = next_col;
  current_row = next_row;
  update_block(current_col, current_row, current_block);
  return true;
}

void collapse_full_rows() {
  for (byte check_row_index = 0; check_row_index < ROW_COUNT; check_row_index++) {
    byte *check_row = playfield[check_row_index];
    if (full_row(check_row)) {
      // TODO consider the two invisible rows at top
      for (byte replace_row_index = check_row_index; replace_row_index > 1; replace_row_index--) {
        byte *playfield_from_row = playfield[replace_row_index - 1];
        byte *playfield_to_row = playfield[replace_row_index];
        byte *bitmap_from_row = bitmap[playfield_offset_y + replace_row_index - 1];
        byte *bitmap_to_row = bitmap[playfield_offset_y + replace_row_index];
        for (byte col = 0; col < COLUMN_COUNT; col++) {
          playfield_to_row[col] = playfield_from_row[col];
          bitmap_to_row[playfield_offset_x + col] = bitmap_from_row[playfield_offset_x + col];
        }
      }
    }
  }
}

boolean full_row(byte *row) {
  for (int col = 0; col < COLUMN_COUNT; col++) {
    if (row[col] == BLOCK_NONE) {
      return false;
    }
  }
  return true;
}

void draw_field() {
  for (byte row = 0; row < ROW_COUNT; row++) {
    for (byte col = 0; col < COLUMN_COUNT; col++) {
      const byte color = block_colors[playfield[row][col]];
      bitmap[playfield_offset_y + row][playfield_offset_x + col] = color;
    }
  }
}

void update_block(byte col, byte row, byte block) {
  playfield[row][col] = block;
  bitmap[playfield_offset_y + row][playfield_offset_x + col] = block_colors[block];
}
