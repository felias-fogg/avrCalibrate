// This sketch can be used to calibrate the OSCCAL value (for getting
// a more accurate system clock when using the RC oscillator) and the
// bandgap reference voltage (for measuring, e.g., supply voltage). For the
// frequency calibration, you need an UNO, Nano, ProMini, Mega or similar
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


#define VERSION "0.6.0" 

//#define TRUEMILLIVOLT 3309 // the true voltage measured in mV
#define TRUEMILLIVOLT 5003 // the true voltage measured in mV
#define TRUETICKS 100000 // the true number of micro secs between two negative edges

#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/delay_basic.h>
#include <util/delay.h>

#ifdef SPIE // if chip has an SPI module
#define TXPIN SCK
#define FRQPIN MISO
#define SIGPIN MOSI
#else
#define TXPIN SCK
#define FRQPIN MOSI
#define SIGPIN MISO
#endif
#define TXDELAY ((F_CPU / 1200)-15)/4 // means 1200 baud
#define TXBITMSK digitalPinToBitMask(TXPIN)
#define TXPORTREG portOutputRegister(digitalPinToPort(TXPIN))

#define TIMEOUT (10000UL*(F_CPU/1000000UL))
#define SUCCTHRES 3 // number of consecutive measurements to accept a level change
#define MIN_CHANGE 100


#if !defined(__AVR_ATtiny2313__) && !defined(__AVR_ATtiny2313A__) && !defined(__AVR_ATtiny4313__) \
  && !defined(__AVR_ATtiny13__) // these do not support Vcc measuring
#include <Vcc.h>
#endif

#if defined(__AVR_ATtiny43U__)
#error "Unsupported MCU"
#elif defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny2313A__) || defined(__AVR_ATtiny4313__)
#define TCNT TCNT0
#define TOV TOV0
#define TCCR0CS TCCR0B
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny24A__) || defined(__AVR_ATtiny44__) \
  || defined(__AVR_ATtiny44A__) || defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny84A__)  
#define TIMSK TIMSK0
#define TCNT TCNT0
#define TIFR TIFR0
#define TOV TOV0
#define TCCR0CS TCCR0B
#elif defined(__AVR_ATtiny441__) || defined(__AVR_ATtiny841__)
#define TIMSK TIMSK0
#define TCNT TCNT0
#define TIFR TIFR0
#define TOV TOV0
#define TCCR0CS TCCR0B
#define OSCCAL OSCCAL0
#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#define TCNT TCNT0
#define TOV TOV0
#define TCCR0CS TCCR0B
#elif defined(__AVR_ATtiny26__)
#define TCNT TCNT0
#define TOV TOV0
#define TCCR0A TCCR0
#define TCCR0CS TCCR0
#elif defined(__AVR_ATtiny261__) || defined(__AVR_ATtiny261A__) || defined(__AVR_ATtiny461__) \
  || defined(__AVR_ATtiny461A__) || defined(__AVR_ATtiny861__) || defined(__AVR_ATtiny861A__) 
#define TCNT TCNT0L
#define TOV TOV0
#define TCCR0CS TCCR0B
#elif defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
#define TIMSK TIMSK0
#define TCNT TCNT0
#define TIFR TIFR0
#define TOV TOV0
#define TCCR0CS TCCR0B
#elif defined(__AVR_ATtiny48__) || defined(__AVR_ATtiny88__)
#define TIMSK TIMSK0
#define TCNT TCNT0
#define TIFR TIFR0
#define TOV TOV0
#define TCCR0CS TCCR0A
#elif defined(__AVR_ATtiny828__)
#define TIMSK TIMSK0
#define TCNT TCNT0
#define TIFR TIFR0
#define TOV TOV0
#define TCCR0CS TCCR0B
#define OSCCAL OSCCAL0
#elif defined(__AVR_ATtiny1634__) 
#define TCNT TCNT0
#define TOV TOV0
#define TCCR0CS TCCR0B
#define OSCCAL OSCCAL0 // the chip has two OSCCAL regs, 0 is for ordinary clock
#elif defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__) ||  defined(__AVR_ATmega328PB__) || \
  defined (__AVR_ATmega168__) || defined (__AVR_ATmega168A__)  || defined (__AVR_ATmega168PA__) || \
  defined (__AVR_ATmega168P__) || defined (__AVR_ATmega168PB__) ||	\
  defined (__AVR_ATmega88__) || defined (__AVR_ATmega88A__) || defined (__AVR_ATmega88P__)  || \
  defined (__AVR_ATmega88PA__) || defined (__AVR_ATmega88PB__) ||	\
  defined (__AVR_ATmega48__) || defined (__AVR_ATmega48A__) || defined (__AVR_ATmega48P__) || \
  defined (__AVR_ATmega48PA__)|| defined (__AVR_ATmega48PB__) || \
  defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega644__) || \
  defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__) || \
  defined(__AVR_ATmega324__) || defined(__AVR_ATmega324A__) || defined(__AVR_ATmega324PA__) || \
  defined(__AVR_ATmega324P__) || defined(__AVR_ATmega164__) || defined(__AVR_ATmega164A__) || \
  defined(__AVR_ATmega164PA__) || defined(__AVR_ATmega164P__) || \
  defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || \
  defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__) 
#define TIMSK TIMSK0
#define TCNT TCNT0
#define TIFR TIFR0
#define TOV TOV0
#define TCCR0CS TCCR0B
#else
#error "Unsupported MCU"
#endif


unsigned long wait;
int dir = 0;
char valstr[16];
long ticks[5];
long count;

void setup(void)
{
  byte wait = 0;

  
  // wait for ready signal from server
  pinMode(SIGPIN, INPUT_PULLUP);
  
  while (wait < 100) {
    if (digitalRead(SIGPIN) == LOW) wait++;
    else wait = 0;
  }
  
  // startup and greeting
  digitalWrite(TXPIN, HIGH);
  pinMode(TXPIN, OUTPUT);
  pinMode(FRQPIN, INPUT_PULLUP);
#if FLASHEND >= 0x0800
  txstr(F("\n\rcalibTarget V" VERSION "\n\r"));
#endif
  // setup regs (and stop millis interrupt)
  TIMSK = 0; // disable millis interrupt
  TCCR0A = 0; // normal operation

  // calibrate OSCCAL
  calOSCCAL();

  // calibrate VCC
  calVcc();
}

void loop(void) { }

void calOSCCAL(void)
{
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)
  txstr(F("OSCCAL calibration not possible.\n\r"));
  eeprom_write_word((unsigned int *)(E2END-1),0xFFFF);
#elif (F_CPU != 8000000) && (F_CPU != 1000000) 
  txstr(F("Unsupported clock frequency. Calibration only possible for 1 MHz and 8 MHz.\n\r"));
  eeprom_write_word((unsigned int *)(E2END-1),0xFFFF);
#else  
  int dir = 1;
  byte osccal;
  long mindiff = TRUETICKS;
  int minix = 0;

  for (byte i=0; i<5; i++) ticks[i] = 0;
  ticks[0] = measure();
  if (!ticksOK(ticks)) return;
  if (ticks[0] > TRUETICKS) dir = -1;
  do {
    reportMeasurement();
    OSCCAL = OSCCAL + dir;
    shiftTicks();
    ticks[0] = measure();
    if (!ticksOK(ticks)) return;
  } while ((ticks[0] < TRUETICKS && dir == 1 && OSCCAL != 0xFF) || (ticks[0] >= TRUETICKS && dir == -1 && OSCCAL != 0));
  reportMeasurement();
  shiftTicks();
  OSCCAL = OSCCAL + dir;
  ticks[0] = measure();
  reportMeasurement();
  for (byte i=0; i < 5; i++) {
    if (mindiff > abs(ticks[i] - TRUETICKS)) {
      minix = i;
      mindiff = abs(ticks[i] - TRUETICKS);
    }
  }
  OSCCAL = OSCCAL + -1*dir*minix;
  txstr(F("\n\rFinal OSCCAL: 0x"));
  txstr(itoa(OSCCAL,valstr,16));
  txnl();
  osccal = OSCCAL;
  eeprom_write_byte((byte *)E2END-1, 0);
  eeprom_write_byte((byte *)E2END, osccal);
#endif
}

void shiftTicks(void)
{
  for (byte i=4; i>0; i--) ticks[i] = ticks[i-1];
}

// perform measurement, i.e. count ten periods
long measure(void)
{
  _delay_ms(100); // let frequency change settle and wait for I/O being finished
  TCCR0CS = (F_CPU == 8000000) ? 0b010 : 0b001; // WGM02=0, prescaler == 8 if 8 MHz, otherwise =1
  // wait for first falling edge
  if (!fallingEdge()) return -1; 
  count = 0;
  TCNT = 0;
  TIFR |= (1<<TOV);
  if (!fallingEdge()) return -1; 
  TCCR0CS = 0; // stop counting
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


boolean ticksOK(long t[5])
{
  if (t[0] < 0) {
#if FLASHEND >= 0x0800
    txstr(F("\n\rOSCCAL calib. timeout\n\r"));
#else
    txstr("\n\rtimeout\n\r");
#endif    
    return false;
  }
  if ((abs(t[0]-t[1]) < MIN_CHANGE) && (abs(t[1]-t[2]) < MIN_CHANGE) && (abs(t[0]-t[2]) < MIN_CHANGE)) {
#if FLASHEND >= 0x0800
    txstr(F("\n\rOSCCAL calib. impossible\n\r"));
#else
    txstr("\n\rNo change!\n\r");
#endif    
    return false;
  }
  return true;
}

void reportMeasurement(void)
{
#if FLASHEND >= 0x0800
  txstr(F("\n\rOSCCAL: 0x"));
#else
  txstr("\n\r0x");
#endif
  txstr(itoa(OSCCAL,valstr,16));
  txstr(F(",  ticks: "));
  txstr(ltoa(ticks[0],valstr,10));
}  

void calVcc(void)
{
#if defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny2313A__) || defined(__AVR_ATtiny4313__) \
  || defined(__AVR_ATtiny13__) // these do not support Vcc measuring
  txstr(F("\n\rMCU does not support measuring Vcc\n\r"));
#else
  long intref, volt;
#if FLASHEND >= 0x800
  long controlvolt;
#endif
  volt = Vcc::measure(2000,DEFAULT_INTREF);
#if FLASHEND >= 0x800
  txstr(F("\n\rTrue voltage (mV): "));
  txstr(itoa(TRUEMILLIVOLT,valstr,10));
  txstr(F("\n\rMeasured with default intref (mV): "));
  txstr(itoa(volt,valstr,10));
#endif
  intref = ((long)(DEFAULT_INTREF) * (long)(TRUEMILLIVOLT)) / volt;
  txstr(F("\n\rIntref: "));
  txstr(itoa(intref,valstr,10));
#if FLASHEND >= 0x800
  controlvolt = Vcc::measure(1000,intref);
  txstr(F("\n\rMeasured (mV):     "));
  txstr(itoa(controlvolt,valstr,10));
  txstr(F("\n\r...done\n\r"));
#endif
  eeprom_write_word((unsigned int*)(E2END-3), intref);
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
