// library for setting calibration values at setup
#include <avrCalibrate.h>

void avrCalibrate::init(int osccal,
#if !defined(__AVR_ATtiny2313__) && !defined(__AVR_ATtiny2313A__) && !defined(__AVR_ATtiny4313__)
			int intref)
#else
                        __attribute__ ((unused)) int intref)
#endif
{
  if (osccal >= 0 && osccal <= 0xFF) // only legal OSCCAL values!
    OSCCAL = osccal;
#if !defined(__AVR_ATtiny2313__) && !defined(__AVR_ATtiny2313A__) && !defined(__AVR_ATtiny4313__)
  if (intref >= 0) // only legal values
    Vcc::setIntref(intref);
#endif
}

void avrCalibrate::init(void)
{
  init(eeprom_read_byte((byte *)(E2END-1)) == 0 ? eeprom_read_byte((byte *)E2END) : NOOSCCAL,
#if !defined(__AVR_ATtiny2313__) && !defined(__AVR_ATtiny2313A__) && !defined(__AVR_ATtiny4313__)
       (int)eeprom_read_word((uint16_t *)EE_INTREF));
#else
       NOVOLT);
#endif
}
