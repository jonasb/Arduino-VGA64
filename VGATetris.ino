#include <VGA.h>

VGA vga;

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
  0b00000000,
  0b11110000,
  0b00110011,
  0b11000010,
  0b00110001,
  0b00000011,
  0b10000000,
  0b00100011,
};
const byte BLOCK_I_STRUCTURE[4][4] = {
 0, 0, 0, 0,
 1, 1, 1, 1,
 0, 0, 0, 0,
 0, 0, 0, 0,
};
const byte BLOCK_O_STRUCTURE[2][2] = {
 1, 1,
 1, 1,
};
const byte BLOCK_T_STRUCTURE[3][3] = {
 0, 1, 0,
 1, 1, 1,
 0, 0, 0,
};
const byte BLOCK_S_STRUCTURE[3][3] = {
 0, 1, 1,
 1, 1, 0,
 0, 0, 0,
};
const byte BLOCK_Z_STRUCTURE[3][3] = {
 1, 1, 0,
 0, 1, 1,
 0, 0, 0,
};
const byte BLOCK_J_STRUCTURE[3][3] = {
 1, 0, 0,
 1, 1, 1,
 0, 0, 0,
};
const byte BLOCK_L_STRUCTURE[3][3] = {
 0, 0, 1,
 1, 1, 1,
 0, 0, 0,
};


byte current_block_type = BLOCK_NONE;
byte current_block_structure_tmp[] = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
};
byte *current_block_structure = current_block_structure_tmp;
byte current_block_size;
byte current_col = COLUMN_COUNT / 1;
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
  vga.init();
  vga.attachInterrupt(gameTick);
  //
  randomSeed(analogRead(0)); // analog 0 is unconnected
  // setup playfield
  memset(playfield, BLOCK_NONE, ROW_COUNT * COLUMN_COUNT);
  memset(vga.bitmap, 0b00000001, Y_PIXELS * X_PIXELS);
  draw_field();
  select_next_block();
  //debug_draw_all_colors(0, 0, Y_PIXELS);
  // control
  pinMode(11, INPUT);
  digitalWrite(11, HIGH);
  pinMode(12, INPUT);
  digitalWrite(12, HIGH);
  pinMode(8, INPUT);
  digitalWrite(8, HIGH);
}

void debug_draw_all_colors(byte start_x, byte start_y, byte height) {
  for (byte c = 0; c < 64; c++) {
    byte x = start_x + c / height;
    byte y = start_y + c % height;
    byte color = ((c << 2) & 0b11110000) | c & 0b00000011;
    vga.bitmap[y][x] = color;
  }
}

void loop() {
  vga.scanLine();
}

void gameTick() {
  ++time;

  read_key(12, key_consecutive_press_right);
  read_key(8, key_consecutive_press_left);
  read_key(11, key_consecutive_press_rotate);

  if (key_consecutive_press_right == 0 || key_consecutive_press_left == 0) {
    if (key_consecutive_press_right % KEY_REPEAT_FRAMES == 1) {
      move_block(1, 0);
    }
    if (key_consecutive_press_left % KEY_REPEAT_FRAMES == 1) {
      move_block(-1, 0);
    }
  }
  if (key_consecutive_press_rotate % KEY_REPEAT_FRAMES == 1) {
    rotate_block();
  }

  if (time == 20) {
    time = 0;
    if (!move_block(0, 1)) {
      // Block is stuck
      add_block_to_playfield();
      collapse_full_rows();

      current_row = 0;
      current_col = COLUMN_COUNT / 2;
      select_next_block();
      update_block(current_col, current_row, current_block_type);
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

void select_next_block() {
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
  current_block_type = block_bag[block_bag_index];
  const byte *structure;
  switch (current_block_type) {
  default:
  case BLOCK_I:
    structure = (byte *)BLOCK_I_STRUCTURE;
    current_block_size = 4;
    break;
  case BLOCK_O:
    structure = (byte *)BLOCK_O_STRUCTURE;
    current_block_size = 2;
    break;
  case BLOCK_T:
    structure = (byte *)BLOCK_T_STRUCTURE;
    current_block_size = 3;
    break;
  case BLOCK_S:
    structure = (byte *)BLOCK_S_STRUCTURE;
    current_block_size = 3;
    break;
  case BLOCK_Z:
    structure = (byte *)BLOCK_Z_STRUCTURE;
    current_block_size = 3;
    break;
  case BLOCK_J:
    structure = (byte *)BLOCK_J_STRUCTURE;
    current_block_size = 3;
    break;
  case BLOCK_L:
    structure = (byte *)BLOCK_L_STRUCTURE;
    current_block_size = 3;
    break;
  }

  for (byte i = 0; i < current_block_size * current_block_size; i++) {
    byte b = structure[i];
    current_block_structure[i] = b;
  }
}

void add_block_to_playfield() {
  for (byte block_y = 0; block_y < current_block_size; block_y++) {
    for (byte block_x = 0; block_x < current_block_size; block_x++) {
      if (current_block_structure[block_y * current_block_size + block_x] != 0) {
        playfield[current_row + block_y][current_col + block_x] = current_block_type;
      }
    }
  }
}

boolean move_block(byte dx, byte dy) {
  byte next_col = current_col + dx;
  byte next_row = current_row + dy;

  if (!valid_block_position(next_col, next_row, current_block_structure)) {
    return false;
  }

  update_block(current_col, current_row, BLOCK_NONE);
  current_col = next_col;
  current_row = next_row;
  update_block(current_col, current_row, current_block_type);
  return true;
}

boolean valid_block_position(byte start_x, byte start_y, byte *block) {
  for (byte block_y = 0; block_y < current_block_size; block_y++) {
    for (byte block_x = 0; block_x < current_block_size; block_x++) {
      if (block[block_y * current_block_size + block_x] != 0) {
        byte x = start_x + block_x;
        byte y = start_y + block_y;
        if (x < 0 || x >= COLUMN_COUNT || y >= ROW_COUNT) {
          return false;
        }
        if (playfield[y][x] != BLOCK_NONE) {
          return false;
        }
      }
    }
  }
  return true;
}

boolean rotate_block() {
  static byte tmp_block[] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
  };

  for (byte y = 0; y < current_block_size; y++) {
    for (int x = 0; x < current_block_size; x++) {
      tmp_block[y * current_block_size + x] = current_block_structure[x * current_block_size + (current_block_size - 1) - y];
    }
  }

  if (!valid_block_position(current_col, current_row, tmp_block)) {
    return false;
  }

  update_block(current_col, current_row, BLOCK_NONE);

  for (byte i = 0; i < current_block_size * current_block_size; i++) {
    current_block_structure[i] = tmp_block[i];
  }
  update_block(current_col, current_row, current_block_type);

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
        byte *bitmap_from_row = vga.bitmap[playfield_offset_y + replace_row_index - 1];
        byte *bitmap_to_row = vga.bitmap[playfield_offset_y + replace_row_index];
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
      vga.bitmap[playfield_offset_y + row][playfield_offset_x + col] = color;
    }
  }
}

void update_block(byte col, byte row, byte block) {
  byte color = block_colors[block];
  for (byte block_y = 0; block_y < current_block_size; block_y++) {
    for (byte block_x = 0; block_x < current_block_size; block_x++) {
      if (current_block_structure[block_y * current_block_size + block_x] != 0) {
        vga.bitmap[playfield_offset_y + row + block_y][playfield_offset_x + col + block_x] = color;
      }
    }
  }
}
