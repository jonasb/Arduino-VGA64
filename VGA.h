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

#endif //VGA_H_INCLUDED
