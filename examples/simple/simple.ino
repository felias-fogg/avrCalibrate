// This sketch uses the method of using the values stored in EEPROM

// It will wait 10 seconds and then it will call in init

#include <avrCalibrate.h>

void setup(void)
{
  delay(10000);
  avrCalibrate::init();
}

void loop(void) { }

