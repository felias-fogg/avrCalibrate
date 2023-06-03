// library for setting calibration values at setup
#include <avrCalibrate.h>

void avrCalibrate::init(byte osccal, int intref)
{
  if (osccal <= 0x7F) // only legal OSCCAL values!
    OSCCAL = osccal;
#if !defined(__AVR_ATtiny2313__) && !defined(__AVR_ATtiny2313A__) && !defined(__AVR_ATtiny4313__)
  if (intref > 0) // only legal values
    Vcc::setIntref(intref);
}

void avrCalibrate::init(void)
{
  init(eeprom_read_byte(E2END-2), (int)eeprom_read_word(E2END-1));
}
