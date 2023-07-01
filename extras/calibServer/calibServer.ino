// This sketch generates a 10 Hz signal on an Arduino Uno/Nano/ProMini/Mega (or similar)
// open drain pin using Timer1. Additionally, it passes the 1200 baud output of a target board
// to Serial.

#define VERSION "1.1.3"

#define FREQPIN MISO
#define TTYPIN SCK
#define SIGPIN MOSI

#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_DUEMILANOVE) || defined(ARDUINO_AVR_NANO) \
  || defined(ARDUINO_AVR_MEGA) || defined(ARDUINO_AVR_MEGA2560)  \
  || defined(ARDUINO_AVR_PRO)
#else
#error "Board not supported"
#endif


#define PUSHPULL false

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
  pinMode(SCK,INPUT);
  Serial.begin(115200);
  ser.begin(1200);
  Serial.println(F("\n\rcalibServer V " VERSION "\n\r"));
  TIMSK0 = 0; // stop the millis interrupt in order to avoid glitches
  // setup Timer1 in mode 11 
  TCCR1A = 0b11; // WGM11 = WGM10 = 1, no output
  TCCR1B = 0b10010; // WGM13=1, WGM12=0, prescaler = 8
  OCR1A = F_CPU/320;
  TIMSK1 = (1 << OCIE1A);  // enable OCR1A interrupt
#if PUSHPULL
  pinMode(FREQPIN, OUTPUT);
#endif
  Serial.println(F("Press any key to start: "));
  while (!Serial.available());
  Serial.println(F("\n\rFeedback from target board:"));
  pinMode(SIGPIN, OUTPUT);
  digitalWrite(SIGPIN, LOW);
}

void loop(void)
{
  char c;
  if (ser.available()) {
    c = ser.read();
    if (c > 0) Serial.print(c);
  }
}
