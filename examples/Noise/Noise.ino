#include <VGA64.h>

VGA64 vga;

void setup() {
  vga.begin();
  vga.attachInterrupt(tick);
}

void loop() {
  vga.scanLine();
}

void tick() {
  int x = random(width);
  int y = random(height);
  vga.pixels[y][x] = random(256);
}
