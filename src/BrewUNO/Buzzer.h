#ifndef Buzzer_h
#define Buzzer_h

#include <Arduino.h>

class Buzzer
{
public:
  void Ring();
  void Ring(int count);
};

#endif
