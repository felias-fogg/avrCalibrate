// This sketch can be used to calibrate the OSCCAL value (for getting
// a more accurate system clock when using the RC oscillator) and the
// bandgap reference voltage (for measuring, e.g., supply voltage). For the
// frequency calibration, you need an UNO, Leonardo, Mega or similar
// board, which generates a (reasonably) accurate 100 Hz signal.

// The result of the calibration is stored in EEPROM. By default, the
// the bandgap reference voltage is stored as a two byte value in
// the last two bytes of EEPROM, the OSCCAL value is stored just
// before that. A validness flag for the OSCCAL value is stored before it. 
// In addition, the values are also communicated over the
// SCK line using 1200 baud.

// The MOSI line is used as the input for the 100 Hz signal
// In case of F_CPU==8000000, we use a prescaler of 8, otherwise 1. Since
// the input frequency is 100 Hz and we stop counting after 10 periods,
// a fully calibrated clock will give us 100000 counts.


#define VERSION "0.4.0" 

#define TRUEMILLIVOLT 5007 // the true voltage measured in mV
#define TRUEMILLIHZ 100000 // the true frequency in milli HZ

#include <EEPROM.h>
#include <avr/pgmspace.h>
#include <util/delay_basic.h>
#include <util/delay.h>

#define FRQPIN SCK
#ifdef SPIE // if chip has an SPI module
#define TXPIN MOSI
#else
#define TXPIN MISO
#endif
#define TXDELAY ((F_CPU / 1200)-15)/4 // means 1200 baud
#define TXBITMSK digitalPinToBitMask(TXPIN)
#define TXPORTREG portOutputRegister(digitalPinToPort(TXPIN))

// The ATmega8 has a different nominal bandgap reference voltages
#ifdef __AVR_ATmega8__ 
#define DEFINTREF 1300
#else
#define DEFINTREF 1100
#endif

#define TIMEOUT (10000UL*(F_CPU/1000000UL))
#define SUCCTHRES 3 // number of consecutive measurements to accept a level change
#define MIN_CHANGE 200


#if !defined(__AVR_ATtiny2313__) && !defined(__AVR_ATtiny2313A__) && !defined(__AVR_ATtiny4313__) \
  && !defined(__AVR_ATtiny13__) // these do not support Vcc measuring
#include <Vcc.h>
#endif


#if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny24A__) || defined(__AVR_ATtiny44__) \
  || defined(__AVR_ATtiny44A__) || defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny84A__)  \
  || defined(__AVR_ATtiny441__) || defined(__AVR_ATtiny841__)
#define TIMSK TIMSK0
#define TCNT TCNT0
#define TIFR TIFR0
#define TOV TOV0
#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#define TCNT TCNT0
#define TIFR TIFR0
#define TOV TOV0
#elif defined(__AVR_ATtiny261__) || defined(__AVR_ATtiny261A__) || defined(__AVR_ATtiny461__) \
  || defined(__AVR_ATtiny461A__) || defined(__AVR_ATtiny861__) || defined(__AVR_ATtiny861A__)
#define TCNT TCNT0L
#define TIFR TIFR0
#define TOV TOV0
#elif defined(__AVR_ATtiny1634__) 
#define TCNT TCNT0
#define TIFR TIFR0
#define TOV TOV0
#define OSCCAL OSCCAL0 // the chip has two OSCCAL regs, 0 is for ordinary clock 
#else
#error "Unsupported MCU"
#endif

#if (F_CPU != 8000000) && (F_CPU != 1000000) 
  #error "Unsupported clock frequency. Only 1 MHz and 8 MHz are possible."
#endif

unsigned long wait;
long count;
int dir = 0;
char valstr[16];
long ticks[3] = {0, 0, 0};



void setup(void)
{
  // startup and greeting
  _delay_ms(2000); // wait for serv er to start up
  digitalWrite(TXPIN, HIGH);
  pinMode(TXPIN, OUTPUT);
  txstr(F("\n\rcalibTarget V" VERSION "\n\r"));
  // setup regs (and stop millis interrupt)
  TIMSK = 0; // disable millis interrupt
  TCCR0A = 0; // normal operation

  // calibrate OSCCAL
  calOSCCAL();

  // calibrate VCC
  calVcc();

  while (1);
}

void loop(void) { }

void calOSCCAL(void)
{
  int dir = 1;
  byte osccal;

  ticks[0] = measure();
  if (!ticksOK(ticks)) return;
  if (ticks[0] > TRUEMILLIHZ) dir = -1;
  do {
    reportMeasurement();
    for (byte i=2; i>0; i--) ticks[i] = ticks[i-1];
    OSCCAL = OSCCAL + dir;
    _delay_ms(50); // let frequency change settle and wait for I/O being finished
    ticks[0] = measure();
    if (!ticksOK(ticks)) return;
  } while ((ticks[0] < TRUEMILLIHZ && dir == 1 && OSCCAL != 0xFF) || (ticks[0] >= TRUEMILLIHZ && dir == -1 && OSCCAL != 0));
  reportMeasurement();
  if (abs(ticks[0] - TRUEMILLIHZ) > abs(ticks[1] - TRUEMILLIHZ)) 
    OSCCAL = OSCCAL + -1*dir; // use previous OSCCAL value
  txstr(F("\n\rFinal OSCCAL: 0x"));
  txstr(itoa(OSCCAL,valstr,16));
  txnl();
  osccal = OSCCAL;
  EEPROM.put(E2END-2, osccal);
  EEPROM.put(E2END-3, (byte)0);
}


// perform measurement, i.e. count ten periods
long measure(void)
{
  // wait for first falling edge
  TCCR0B = (F_CPU == 8000000) ? 0b010 : 0b001; // WGM02=0, prescaler == 8 if 8 MHz, otherwise =1
  if (!fallingEdge()) return -1; 
  count = 0;
  TCNT = 0;
  TIFR |= (1<<TOV);
  for (byte i=0; i<10; i++) {
    if (!fallingEdge()) return -1; 
  }
  TCCR0B = 0; // stop counting
  incIfTOV();
  count = count + TCNT;
  return count;
}

boolean fallingEdge(void)
{
  if (!waitTransTo(true)) return false; 
  return waitTransTo(false);
}

void incIfTOV(void)
{
  if (TIFR&(1<<TOV)) {
    count = count+256;
    TIFR |= (1<<TOV);
  }
}


// wait for level change (and only accept if two consecuitive measurements)
bool waitTransTo(boolean level)
{
  unsigned long wait=0;
  byte succ=0;
  while (wait < TIMEOUT) {
    if (digitalRead(FRQPIN) == level) succ++;
    else succ = 0;
    if (succ >= SUCCTHRES) return true;
    incIfTOV();
    wait++;
  }
  return false;
}


boolean ticksOK(long t[3])
{
  if (t[0] < 0) {
    txstr(F("\n\rCalib. timeout"));
    return false;
  }
  if ((abs(t[0]-t[1]) < MIN_CHANGE) && (abs(t[1]-t[2]) < MIN_CHANGE)) {
    txstr(F("\n\rCalib. impossible"));
    return false;
  }
  return true;
}

void reportMeasurement(void)
{
  txstr(F("\n\rOSCCAL: 0x"));
  txstr(itoa(OSCCAL,valstr,16));
  txstr(F(",  ticks: "));
  txstr(ltoa(ticks[0],valstr,10));
}  

void calVcc(void)
{
#if defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny2313A__) || defined(__AVR_ATtiny4313__) \
  || defined(__AVR_ATtiny13__) // these do not support Vcc measuring
  txstr(F("MCU does not support measuring Vcc\n\r"));
#else
  long intref, volt, controlvolt;
  volt = Vcc::measure(1000,DEFINTREF);
#if FLASHEND >= 0x800
  txstr(F("\n\rTrue voltage (mV): "));
  txstr(itoa(TRUEMILLIVOLT,valstr,10));
  txstr(F("\n\rMeasured with default intref (mV): "));
  txstr(itoa(volt,valstr,10));
#endif
  intref = ((long)(DEFINTREF) * (long)(TRUEMILLIVOLT)) / volt;
  txstr(F("\n\rIntref: "));
  txstr(itoa(intref,valstr,10));
  controlvolt = Vcc::measure(1000,intref);
#if FLASHEND >= 0x800
  txstr(F("\n\rMeasured (mV):     "));
  txstr(itoa(controlvolt,valstr,10));  
#endif
  txnl();
  EEPROM.put(E2END-1,intref);
#endif
}

// I/O stuff


void txchar(byte b) {
  // By declaring these as local variables, the compiler will put them
  // in registers _before_ disabling interrupts and entering the
  // critical timing sections below, which makes it a lot easier to
  // verify the cycle timings
  volatile byte *reg = TXPORTREG;
  unsigned int delay = TXDELAY;
  byte reg_mask = TXBITMSK;
  byte inv_mask = ~TXBITMSK;
  byte oldSREG = SREG;
  cli();  // turn off interrupts for a clean txmit

  // Write the start bit
  *reg &= inv_mask;

  _delay_loop_2(delay);

  // Write each of the 8 bits
  for (uint8_t i = 8; i > 0; --i)
  {
    if (b & 1) // choose bit
      *reg |= reg_mask; // send 1
    else
      *reg &= inv_mask; // send 0

    _delay_loop_2(delay);
    b >>= 1;
  }

  // restore pin to natural state
  *reg |= reg_mask;

  SREG = oldSREG; // turn interrupts back on
  _delay_loop_2(delay);
}

  
void txstr(const char* s)
{
  while (*s) txchar(*s++);
}

void txstr(const __FlashStringHelper* f)
{
  const char* s;
  s=reinterpret_cast<PGM_P>(f);
  while (pgm_read_byte(s)) txchar(pgm_read_byte(s++));
}

void txnl(void)
{
  txstr(F("\n\r"));
}
