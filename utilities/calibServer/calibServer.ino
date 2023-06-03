// This sketch generates a 100 Hz frequency on an Arduino Uno/Nano/ProMini/Leornado/Mega (or similar)
// open drain pin or pull-push pin (your choice!) using Timer1.

#define VERSION "0.1.0"

#define PUSHPULL true

#define FREQPIN MOSI
#define TTYPIN SCK


#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__) || \
    defined(__AVR_ATmega168P__) || defined(__AVR_ATmega168__) || \
    defined (__AVR_ATmega32U4__) || \
    defined(__AVR_ATmega1284P__) ||  defined(__AVR_ATmega1284__) || \
    defined(__AVR_ATmega644P__) ||  defined(__AVR_ATmega644__) || \
    defined(__AVR_ATmega324P__) ||  defined(__AVR_ATmega324__) || \
    defined(__AVR_ATmega164P__) ||  defined(__AVR_ATmega164__) || \
    defined(__AVR_ATmega2560__)
#else
#error "Unsupported MCU type"
#endif

#include <SoftwareSerial.h>

SoftwareSerial ser(TTYPIN,2);

volatile boolean level = false; // level of waveform 

ISR(TIMER1_COMPA_vect)
{
  level = !level;
#if PUSHPULL
  if (!level) digitalWrite(FREQPIN, LOW);
  else digitalWrite(FREQPIN, HIGH);
#else
  if (!level) pinMode(FREQPIN, OUTPUT);
  else pinMode(FREQPIN, INPUT);
#endif
}

void setup(void)
{
  Serial.begin(115200);
  ser.begin(1200);
  Serial.println(F("\n\rcalibServer Version " VERSION));
  Serial.println(F("Feedback from target board:"));
  TIMSK0 = 0; // stop the millis interrupt in order to avoid glitches
  // setup Timer1 in mode 11 
  TCCR1A = 0b11; // WGM11 = WGM10 = 1, no output
  TCCR1B = 0b10010; // WGM13=1, WGM12=0, prescaler = 8
  OCR1A = F_CPU/3200-1;
  TIMSK1 = (1 << OCIE1A);  // enable OCR1A interrupt
#if PUSHPULL
  pinMode(FREQPIN, OUTPUT);
#endif
}

void loop(void)
{
  if (ser.available()) Serial.print((char)ser.read());
}
