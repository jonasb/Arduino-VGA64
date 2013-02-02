#include <VGA.h>

VGA vga;

#define COLUMN_COUNT  10
#define ROW_COUNT 22
const byte playfieldOffsetX = (X_PIXELS - COLUMN_COUNT) / 2;
const byte playfieldOffsetY = (Y_PIXELS - ROW_COUNT) / 2;

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
const byte blockColors[] = {
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


byte currentBlockType = BLOCK_NONE;
byte currentBlockStructureTmp[] = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
};
byte *currentBlockStructure = currentBlockStructureTmp;
byte currentBlockSize;
byte currentCol = COLUMN_COUNT / 1;
byte currentRow = 0;

byte blockBag[] = {
  BLOCK_I,
  BLOCK_O,
  BLOCK_T,
  BLOCK_S,
  BLOCK_Z,
  BLOCK_J,
  BLOCK_L,
};
#define BLOCK_BAG_COUNT 7
byte blockBagIndex = BLOCK_BAG_COUNT - 1; // will cause bag randomization

#define KEY_REPEAT_FRAMES 10
byte keyConsecutivePressesLeft = 0;
byte keyConsecutivePressesRight = 0;
byte keyConsecutivePressesRotate = 0;

byte time = 0;

void setup() {
  vga.init();
  vga.attachInterrupt(gameTick);
  //
  randomSeed(analogRead(0)); // analog 0 is unconnected
  // setup playfield
  memset(playfield, BLOCK_NONE, ROW_COUNT * COLUMN_COUNT);
  memset(vga.bitmap, 0b00000001, Y_PIXELS * X_PIXELS);
  drawField();
  selectNextBlock();
  //debugDrawAllColors(0, 0, Y_PIXELS);
  // control
  pinMode(11, INPUT);
  digitalWrite(11, HIGH);
  pinMode(12, INPUT);
  digitalWrite(12, HIGH);
  pinMode(8, INPUT);
  digitalWrite(8, HIGH);
}

void debugDrawAllColors(byte startX, byte startY, byte height) {
  for (byte c = 0; c < 64; c++) {
    byte x = startX + c / height;
    byte y = startY + c % height;
    byte color = ((c << 2) & 0b11110000) | c & 0b00000011;
    vga.bitmap[y][x] = color;
  }
}

void loop() {
  vga.scanLine();
}

void gameTick() {
  ++time;

  readKey(12, keyConsecutivePressesRight);
  readKey(8, keyConsecutivePressesLeft);
  readKey(11, keyConsecutivePressesRotate);

  if (keyConsecutivePressesRight == 0 || keyConsecutivePressesLeft == 0) {
    if (keyConsecutivePressesRight % KEY_REPEAT_FRAMES == 1) {
      moveBlock(1, 0);
    }
    if (keyConsecutivePressesLeft % KEY_REPEAT_FRAMES == 1) {
      moveBlock(-1, 0);
    }
  }
  if (keyConsecutivePressesRotate % KEY_REPEAT_FRAMES == 1) {
    rotateBlock();
  }

  if (time == 20) {
    time = 0;
    if (!moveBlock(0, 1)) {
      // Block is stuck
      addBlockToPlayfield();
      collapseFullRows();

      currentRow = 0;
      currentCol = COLUMN_COUNT / 2;
      selectNextBlock();
      updateBlock(currentCol, currentRow, currentBlockType);
    }
  }
}

void readKey(int pin, byte &keyConsecutivePresses) {
  if (digitalRead(pin) == LOW) {
    ++keyConsecutivePresses; //TODO 0xff
  }
  else {
    keyConsecutivePresses = 0;
  }
}

void selectNextBlock() {
  ++blockBagIndex;
  if (blockBagIndex == BLOCK_BAG_COUNT) {
    blockBagIndex = 0;
    // randomize bag
    for (byte i = 0; i < BLOCK_BAG_COUNT - 1; i++) {
      byte j = random(i + 1, BLOCK_BAG_COUNT);
      byte t = blockBag[j];
      blockBag[j] = blockBag[i];
      blockBag[i] = t;
    }
  }
  currentBlockType = blockBag[blockBagIndex];
  const byte *structure;
  switch (currentBlockType) {
  default:
  case BLOCK_I:
    structure = (byte *)BLOCK_I_STRUCTURE;
    currentBlockSize = 4;
    break;
  case BLOCK_O:
    structure = (byte *)BLOCK_O_STRUCTURE;
    currentBlockSize = 2;
    break;
  case BLOCK_T:
    structure = (byte *)BLOCK_T_STRUCTURE;
    currentBlockSize = 3;
    break;
  case BLOCK_S:
    structure = (byte *)BLOCK_S_STRUCTURE;
    currentBlockSize = 3;
    break;
  case BLOCK_Z:
    structure = (byte *)BLOCK_Z_STRUCTURE;
    currentBlockSize = 3;
    break;
  case BLOCK_J:
    structure = (byte *)BLOCK_J_STRUCTURE;
    currentBlockSize = 3;
    break;
  case BLOCK_L:
    structure = (byte *)BLOCK_L_STRUCTURE;
    currentBlockSize = 3;
    break;
  }

  for (byte i = 0; i < currentBlockSize * currentBlockSize; i++) {
    byte b = structure[i];
    currentBlockStructure[i] = b;
  }
}

void addBlockToPlayfield() {
  for (byte blockY = 0; blockY < currentBlockSize; blockY++) {
    for (byte blockX = 0; blockX < currentBlockSize; blockX++) {
      if (currentBlockStructure[blockY * currentBlockSize + blockX] != 0) {
        playfield[currentRow + blockY][currentCol + blockX] = currentBlockType;
      }
    }
  }
}

boolean moveBlock(byte dx, byte dy) {
  byte nextCol = currentCol + dx;
  byte nextRow = currentRow + dy;

  if (!isValidBlockPosition(nextCol, nextRow, currentBlockStructure)) {
    return false;
  }

  updateBlock(currentCol, currentRow, BLOCK_NONE);
  currentCol = nextCol;
  currentRow = nextRow;
  updateBlock(currentCol, currentRow, currentBlockType);
  return true;
}

boolean isValidBlockPosition(byte startX, byte startY, byte *block) {
  for (byte blockY = 0; blockY < currentBlockSize; blockY++) {
    for (byte blockX = 0; blockX < currentBlockSize; blockX++) {
      if (block[blockY * currentBlockSize + blockX] != 0) {
        byte x = startX + blockX;
        byte y = startY + blockY;
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

boolean rotateBlock() {
  static byte tmpBlock[] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
  };

  for (byte y = 0; y < currentBlockSize; y++) {
    for (int x = 0; x < currentBlockSize; x++) {
      tmpBlock[y * currentBlockSize + x] = currentBlockStructure[x * currentBlockSize + (currentBlockSize - 1) - y];
    }
  }

  if (!isValidBlockPosition(currentCol, currentRow, tmpBlock)) {
    return false;
  }

  updateBlock(currentCol, currentRow, BLOCK_NONE);

  for (byte i = 0; i < currentBlockSize * currentBlockSize; i++) {
    currentBlockStructure[i] = tmpBlock[i];
  }
  updateBlock(currentCol, currentRow, currentBlockType);

  return true;
}

void collapseFullRows() {
  for (byte checkRowIndex = 0; checkRowIndex < ROW_COUNT; checkRowIndex++) {
    byte *checkRow = playfield[checkRowIndex];
    if (isFullRow(checkRow)) {
      // TODO consider the two invisible rows at top
      for (byte replaceRowIndex = checkRowIndex; replaceRowIndex > 1; replaceRowIndex--) {
        byte *playfieldFromRow = playfield[replaceRowIndex - 1];
        byte *playfieldToRow = playfield[replaceRowIndex];
        byte *bitmapFromRow = vga.bitmap[playfieldOffsetY + replaceRowIndex - 1];
        byte *bitmapToRow = vga.bitmap[playfieldOffsetY + replaceRowIndex];
        for (byte col = 0; col < COLUMN_COUNT; col++) {
          playfieldToRow[col] = playfieldFromRow[col];
          bitmapToRow[playfieldOffsetX + col] = bitmapFromRow[playfieldOffsetX + col];
        }
      }
    }
  }
}

boolean isFullRow(byte *row) {
  for (int col = 0; col < COLUMN_COUNT; col++) {
    if (row[col] == BLOCK_NONE) {
      return false;
    }
  }
  return true;
}

void drawField() {
  for (byte row = 0; row < ROW_COUNT; row++) {
    for (byte col = 0; col < COLUMN_COUNT; col++) {
      const byte color = blockColors[playfield[row][col]];
      vga.bitmap[playfieldOffsetY + row][playfieldOffsetX + col] = color;
    }
  }
}

void updateBlock(byte col, byte row, byte block) {
  byte color = blockColors[block];
  for (byte blockY = 0; blockY < currentBlockSize; blockY++) {
    for (byte blockX = 0; blockX < currentBlockSize; blockX++) {
      if (currentBlockStructure[blockY * currentBlockSize + blockX] != 0) {
        vga.bitmap[playfieldOffsetY + row + blockY][playfieldOffsetX + col + blockX] = color;
      }
    }
  }
}
