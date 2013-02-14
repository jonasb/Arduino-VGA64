#include <VGA64.h>

VGA64 vga;

struct Star {
  float x;
  float y;
  uint16_t c;
};

#define starCount 20
Star stars[starCount];

const float centerX = width / 2.f;
const float centerY = height / 2.f;
const float minX = -centerX;
const float maxX = centerX;
const float minY = -centerY;
const float maxY = centerY;

void setup() {
  vga.begin();
  vga.attachInterrupt(tick);
  for(uint8_t i = 0; i < starCount; i++) {
    createStar(i);
  }
}

inline void createStar(uint8_t i) {
  Star &star = stars[i];
  do {
    star.x = width * ((float) rand() / RAND_MAX) - centerX;
  } while (star.x > -1 && star.x < 1);
  do {
    star.y = height * ((float) rand() / RAND_MAX) - centerY;
  } while (star.y > -1 && star.y < 1);
  star.c = 0;
}

void loop() {
  vga.scanLine();
}

void tick() {
  for (uint8_t i = 0; i < starCount; i++) {
    Star &star = stars[i];
    drawStar(star.x, star.y, 0);
    star.x *= 1.03;
    star.y *= 1.03;
    star.c++;
    if (star.x < minX || star.x > maxX || star.y < minY || star.y > maxY) {
      createStar(i);
    }
  }
  for(uint8_t i = 0; i < starCount; i++) {
    Star &star = stars[i];
    uint16_t color = star.c >> 3;
    drawStar(star.x, star.y, color == 0 ? color222(0, 0, 1) :
                             color == 1 ? color222(1, 1, 2) :
                                          color222(3, 3, 3));
  }
}

inline void drawStar(float x, float y, byte color) {
  vga.pixels[(int) (y + centerY)][(int) (x + centerX)] = color;
}
