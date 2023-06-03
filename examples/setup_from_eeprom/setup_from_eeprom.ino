// This sketch uses the the method of using the values stored in EEPROM
#include <avrCalibrate.h>

void setup(void)
{
  avrCalibrate::init(); // retrieve calibration values from EEPROM and set OSCCAL and global INTREF value
}

void loop
{
}
