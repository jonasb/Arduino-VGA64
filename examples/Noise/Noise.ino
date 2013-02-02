#include <VGA.h>

VGA vga;

void setup() {
  vga.setup();
  vga.attachInterrupt(tick);
}

void loop() {
  vga.scanLine();
}

void tick() {
  int x = random(X_PIXELS);
  int y = random(Y_PIXELS);
  vga.bitmap[y][x] = random(256);
}
