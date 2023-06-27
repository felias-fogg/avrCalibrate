// This sketch uses the method of using the values stored in EEPROM

// You can use the calibServer sketch and the same ICSP connections as the ones used for
// the calibration process.

#include <avrCalibrate.h>
#include <SoftwareSerial.h>

#ifdef SPIE // if chip has an SPI module
#define TXPIN SCK
#define RXPIN MISO
#define SIGPIN MOSI
#else
#define TXPIN SCK
#define RXPIN MOSI
#define SIGPIN MISO
#endif

SoftwareSerial ser(RXPIN, TXPIN);

void setup(void)
{
  byte wait=0;
  pinMode(SIGPIN, INPUT_PULLUP);
  while (wait < 100)
    if (digitalRead(SIGPIN) == LOW) wait++;
    else wait = 0;
  ser.begin(1200);
  ser.print(F("Original OSCCAL: 0x"));
  ser.println(OSCCAL,HEX);
  ser.print(F("Original Vcc measurement (mV): "));
  ser.println(Vcc::measure(100,DEFAULT_INTREF));
  avrCalibrate::init();
  ser.print(F("New OSCCAL: 0x"));
  ser.println(OSCCAL,HEX);
  ser.print(F("New Vcc measurement (mV): "));
  ser.println(Vcc::measure(100));
}

void loop(void) { }

