// This sketch uses the method of using the values passed explicitly to the init method

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

#if defined(__AVR_ATtiny441__) || defined(__AVR_ATtiny841__) || defined(__AVR_ATtiny828__) || defined(__AVR_ATtiny1634__) 
#define OSCCAL OSCCAL0
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
#if !defined(__AVR_ATtiny2313__) && !defined(__AVR_ATtiny2313A__) && !defined(__AVR_ATtiny4313__) \
  && !defined(__AVR_ATtiny13__) // these do not support Vcc measuring
  ser.print(F("Original Vcc measurement (mV): "));
  ser.println(Vcc::measure(100,DEFAULT_INTREF));
#endif
  avrCalibrate::init(OSCCAL+2, DEFAULT_INTREF-100);
  ser.print(F("New OSCCAL: 0x"));
  ser.println(OSCCAL,HEX);
#if !defined(__AVR_ATtiny2313__) && !defined(__AVR_ATtiny2313A__) && !defined(__AVR_ATtiny4313__) \
  && !defined(__AVR_ATtiny13__) // these do not support Vcc measuring
  ser.print(F("New Vcc measurement (mV) with DEFAULT_INTREF-100: "));
  ser.println(Vcc::measure(100));
#endif
}

void loop(void) { }
