#ifndef AVRCALIBRATE_H
#define AVRCALIBRATE_H

#define NOVOLT (-1)
#define NOOSCCAL (-1)

#include <Arduino.h>
#include <avr/eeprom.h>
#if !defined(__AVR_ATtiny2313__) && !defined(__AVR_ATtiny2313A__) && !defined(__AVR_ATtiny4313__)
#include <Vcc.h>
#endif

class avrCalibrate {
 public:
  static void init(void);
  static void init(int osccal, int intref);
};

#endif
