#include <VGA.h>

VGA vga;

void setup() {
  vga.setup();

  int x = 0;
  int y = 0;
  for (int r = 0; r < 4; r++) {
    for (int g = 0; g < 4; g++) {
      for (int b = 0; b < 4; b++) {
        vga.bitmap[y][x] = COLOR222(r, g, b);
        x++;
      }
    }
    y++;
    x = 0;
  }
}

void loop() {
  vga.scanLine();
}

