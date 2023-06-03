#ifndef AVRCALIBRATE_H
#define AVRCALIBRATE_H

#include <Arduino.h>
#include <avr/eeprom.h>
#if !defined(__AVR_ATtiny2313__) && !defined(__AVR_ATtiny2313A__) && !defined(__AVR_ATtiny4313__)
#include <Vcc.h>
#endif

class avrCalibrate {
 public:
  static void init(void);
  static void init(byte osccal, int intref);
}

#endif
