#ifndef VGA_H_INCLUDED
#define VGA_H_INCLUDED

#include <arduino.h>

// constants
#define X_PIXELS    40
#define Y_PIXELS    30

class VGA {
public:
  VGA();
  void setup();
  void attachInterrupt(void (*interrupt)(void));
  void scanLine();
  byte bitmap[Y_PIXELS][X_PIXELS];
private:
  void (*_interrupt)(void);
};

// Each pixel is a byte where each color component (red, green, blue) gets 2 bits.
// There are 64 unique colors (4*4*4).
// The bit pattern is rrggxxbb, where the xx bits are ignored.

// Create a color, each component can be 0-3.
#define COLOR222(r, g, b) \
  (r & 0b00000011) << 6 | \
  (g & 0b00000011) << 4 | \
  (b & 0b00000011)

// Create a color
// This is just a convenience function. It allows you to convert a more standard
// color (where each component can be 0-255) to the supported format.
// Even if you can define 16 million colors the result is only 64 different colors.
#define COLOR888(r, g, b) \
  COLOR222(r >> 2, g >> 2, b >> 2)

#endif //VGA_H_INCLUDED
