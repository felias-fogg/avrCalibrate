#include <avrCalibrate.h>

#define OSCCAL_CALIB 0x76
#define INTREF_CALIB 3399

void setup(void)
{
  avrCalibrate::init(OSCCAL_CALIB, INTREF_CALIB);
}

void loop(void) { }
