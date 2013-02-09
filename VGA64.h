#ifndef VGA64_H_INCLUDED
#define VGA64_H_INCLUDED

#include <arduino.h>

// Schematic
// Arduino pin 0  (blue low)----1.5kΩ-+-HD15 pin 3  (BLUE)
// Arduino pin 1  (blue high)---690Ω-/
// Arduino pin 3  (hsync)---------------HD15 pin 13 (HSync)
// Arduino pin 4  (green low)---1.5kΩ-+-HD15 pin 2  (GREEN)
// Arduino pin 5  (green high)--690Ω-/
// Arduino pin 6  (red low)-----1.5kΩ-+-HD15 pin 1  (RED)
// Arduino pin 7  (red high)----690Ω-/
// Arduino pin 10 (vsync)---------------HD15 pin 14 (VSync)
// Arduino GND--------------------------HD15 pin 5  (GND)

// Constants
#define width 40
#define height 30

class VGA64 {
public:
  VGA64();
  void begin();
  void attachInterrupt(void (*interrupt)(void));
  void scanLine();
  byte pixels[height][width];
private:
  void (*_interrupt)(void);
};

// Each pixel is a byte where each color component (red, green, blue) gets 2 bits.
// There are 64 unique colors (4*4*4).
// The bit pattern is rrggxxbb, where the xx bits are ignored.

// Create a color, each component can be 0-3.
#define color222(r, g, b) \
  (r & 0b00000011) << 6 | \
  (g & 0b00000011) << 4 | \
  (b & 0b00000011)

// Create a color
// This is just a convenience function. It allows you to convert a more standard
// color (where each component can be 0-255) to the supported format.
// Even if you can define 16 million colors the result is only 64 different colors.
#define color888(r, g, b) \
  color222(r >> 2, g >> 2, b >> 2)

#endif //VGA64_H_INCLUDED
