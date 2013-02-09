#include <VGA64.h>

VGA64 vga;

void setup() {
  vga.begin();

  int x = 0;
  int y = 0;
  for (int r = 0; r < 4; r++) {
    for (int g = 0; g < 4; g++) {
      for (int b = 0; b < 4; b++) {
        vga.pixels[y][x] = color222(r, g, b);
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

