// This sketch can be used to calibrate the OSCCAL value (for getting
// a more accurate system clock when using the RC oscillator) and the
// bandgap reference voltage (for measuring, e.g., supply voltage). For the
// frequency calibration, you need an UNO, Leonardo, Mega or similar
// board, which generates a (reasonably) accurate 100 Hz signal.

// The result of the calibration is stored in EEPROM. By default, the
// the bandgap reference voltage is stored as two byte value in
// the last two bytes of EEPROM, the OSCCAL value is stored just
// before that. In addition, the values are also communicated over the
// SCK line using 1200 baud.

// The MOSI line is used as the input for the 100 Hz signal
// We use it as an interrupt input line
// In addition, we use TIMER0 as the calibration timer, counting the overflow separately.
// In case of F_CPU==8000000, we use a prescaler of 8, otherwise 1. Since
// the input frequency is 100 Hz and we stop counting only on falling edges,
// a fully calibrated clock will give us 10000 counts.


#define VERSION "0.3.0" 

#define TRUEMILLIVOLT 5021 // the true voltage measured in mV

#include <EEPROM.h>
#include <avr/pgmspace.h>
#include <util/delay_basic.h>
#include <util/delay.h>

#define txpin SCK
#define txdelay ((F_CPU / 1200)-15)/4 // means 1200 baud
#define txbitmsk digitalPinToBitMask(txpin)
#define txportreg portOutputRegister(digitalPinToPort(txpin))




#if !defined(__AVR_ATtiny2313__) && !defined(__AVR_ATtiny2313A__) && !defined(__AVR_ATtiny4313__) \
  && !defined(__AVR_ATtiny13__) // these do not support Vcc measuring
#include <Vcc.h>
#endif

// The ATmega8 has a different nominal bandgap reference voltages
#ifdef __AVR_ATmega8__ 
#define DEFINTREF 1300
#else
#define DEFINTREF 1100
#endif

#define FREQ_TIMEOUT_SEC 30 // timeout for frequency measurement (<640)
#define MIN_MEASURE_CNT 200 // minimal number of frequency measurement before accept it as valid (<256)
#define EXPECTED_TICKS 10000 // number of expected ticks for 100 Hz signal
#define MAX_DEVIATION 5000  // maximal deviation (i.e., roughly 50%)
#define MIN_CHANGE_PER_STEP 5U // is usually 50
#define CHECK_AFTER_STEPS 5 // after that many check check whether something has changed

#if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny24A__) || defined(__AVR_ATtiny44__) \
  || defined(__AVR_ATtiny44A__) || defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny84A__)  \
  || defined(__AVR_ATtiny441__) || defined(__AVR_ATtiny841__)
#define PINREG PINA
#define PINNUM 6
#define TCNT TCNT0
#define PCINT_vect PCINT0_vect
#define TIMER_COMPA_vect TIM0_COMPA_vect
#define PCIE PCIE0
#define PCINT PCINT6
#define PCMSK PCMSK0
//#define GIMSK GIMSK
#define TIMSK TIMSK0
//#define OSCCAL OSCCAL
#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#define PINREG PINB
#define PINNUM 0
#define TCNT TCNT0
#define PCINT_vect PCINT0_vect
#define TIMER_COMPA_vect TIM0_COMPA_vect
//#define PCIE PCIE
#define PCINT PCINT0
//#define PCMSK PCMSK
//#define GIMSK GIMSK
//#define TIMSK TIMSK
//#define OSCCAL OSCCAL
#elif defined(__AVR_ATtiny261__) || defined(__AVR_ATtiny261A__) || defined(__AVR_ATtiny461__) \
  || defined(__AVR_ATtiny461A__) || defined(__AVR_ATtiny861__) || defined(__AVR_ATtiny861A__)
#define PINREG PINB
#define PINNUM 0
#define TCNT TCNT0L
//#define PCINT_vect PCINT_vect
#define TIMER_COMPA_vect TIMER0_COMPA_vect
#define PCIE PCIE0
#define PCINT PCINT8
#define PCMSK PCMSK1
//#define GIMSK GIMSK
//#define TIMSK TIMSK 
//#define OSCCAL OSCCAL
#elif defined(__AVR_ATtiny1634__) 
#define PINREG PINB
#define PINNUM 1
#define TCNT TCNT0
#define PCINT_vect PCINT1_vect
#define TIMER_COMPA_vect TIM0_COMPA_vect
#define PCIE PCIE1
#define PCINT PCINT9
#define PCMSK PCMSK0
//#define GIMSK GIMSK
//#define TIMSK TIMSK 
#define OSCCAL OSCCAL0 // the chip has two OSCCAL regs, 0 is for ordinary clock 
#else
#error "Unsupported MCU"
#endif

#define FREQ_ERROR 1
#define VOLTREF_ERROR 2

volatile byte valid=0; // measurement only valid when >2
volatile byte ovfcount=0;
volatile unsigned int count=0;
unsigned int currcount=0;
enum {INIT, FREQ, VOLTREF, FINISH} stage = INIT; 
enum {START, UP, DOWN, STOP} dir = START;
int posdiff = EXPECTED_TICKS, minusdiff = EXPECTED_TICKS;
unsigned int freqloop = 0;
byte error = 0;
unsigned int step = 0;
unsigned startcount = 0;
long intref, volt;
byte osccal;
char valstr[16];


ISR(PCINT_vect)
{
  if ((PINREG & (1<<PINNUM)) == 0) { // falling edge
    count = ((unsigned int)ovfcount << 8) | TCNT;
    ovfcount = 0;
    TCNT = 0;
    if (valid <= MIN_MEASURE_CNT) valid++;
  }
}

ISR(TIMER_COMPA_vect)
{
  ovfcount++;
}

void setup(void)
{
  digitalWrite(txpin, HIGH);
  pinMode(txpin, OUTPUT);

  TIMSK = 0; // disable millis interrupt
  TCCR0A = 0; // normal operation
  TCCR0B = (F_CPU == 8000000) ? 0b010 : 0b001; // WGM02=0, prescaler == 8 if 8 MHz, otherwise =1
  OCR0A = 0xFF; // interrupt whenever we reach 0xFF
  TIMSK = (1 << OCIE0A); // enable interrupt for reaching OCRA value
  GIMSK = (1 << PCIE); // enable only PCIs 
  PCMSK = (1 << PCINT); // enable only PCI on frequency input pin
  _delay_ms(2000);
  txstr(F("\n\rcalibTarget Version " VERSION "\n\r"));
  stage = FREQ;
}

void loop(void)
{
  if (stage == FREQ) {
    if (F_CPU != 8000000 && F_CPU != 1000000) { // unsupported frequency
      txstr(F("Unsupported MCU clock frequency\n\r"));
      error = FREQ_ERROR;
      stage = VOLTREF;
      return;
    }      
    if (++freqloop > FREQ_TIMEOUT_SEC * 100) { // more than 30 seconds
      txstr(F("Timeout: could not calibrate MCU clock\n\r"));
      error = FREQ_ERROR;
      stage = VOLTREF;
      return;
    }
    if (valid > MIN_MEASURE_CNT) {
      if (dir == STOP) {
	txstr(F("Final OSCCAL: 0x"));
	txstr(itoa(OSCCAL,valstr,16));
	txstr(F("\n\r"));
	osccal = OSCCAL;
	EEPROM.put(E2END-2, osccal);
	stage = VOLTREF;
	return;
      }
      _delay_ms(500); // allow the freq to settle to new value 
      currcount = getAvgCount();
      //mySerial.println(currcount);
      if (currcount < EXPECTED_TICKS - MAX_DEVIATION || currcount > EXPECTED_TICKS + MAX_DEVIATION) {
	_delay_ms(10);
	return; // unreasonable values
      }
      if (startcount == 0) startcount = currcount;
      txstr(F("OSCCAL: 0x"));
      txstr(itoa(OSCCAL,valstr,16));
      txstr(F(",  ticks: "));
      txstr(itoa(currcount,valstr,10));
      txstr(F("\n\r"));
      if (step >= CHECK_AFTER_STEPS) {
	if ((dir == UP && currcount - startcount < MIN_CHANGE_PER_STEP * step) ||
	    (dir == DOWN && startcount - currcount < MIN_CHANGE_PER_STEP * step)) {
	  error = FREQ_ERROR;
	  stage = VOLTREF;
	  txstr(F("Not enough change detected. We stop changing OSCCAL."));
	  txstr(F("\n\r"));
	  return;
	}
      }
      if (currcount > EXPECTED_TICKS && dir == UP) { // higher freq and moving upwards
	posdiff = currcount - EXPECTED_TICKS;
	if (posdiff > minusdiff) {
	  OSCCAL-- ; // use last value
	}
	dir = STOP;
      } else if (currcount > EXPECTED_TICKS) {
	posdiff = currcount - EXPECTED_TICKS;
	if (OSCCAL == 0) {
	  limitReached();
	  return;
	}
	OSCCAL--;
	step++;
	dir = DOWN;
      } else if (currcount <= EXPECTED_TICKS && dir == DOWN) {
	minusdiff = EXPECTED_TICKS - currcount;
	if (posdiff < minusdiff) {
	  OSCCAL++ ; // use last value
	}
	dir = STOP;
      } else if (currcount <= EXPECTED_TICKS) {
	minusdiff = EXPECTED_TICKS - currcount;
	if (OSCCAL == 0xFF) {
	  txstr(F("OSCCAL limit reached"));
	  txstr(F("\n\r"));
	  error = FREQ_ERROR;
	  stage = VOLTREF;
	  return;
	}
	OSCCAL++;
	step++;
	dir = UP;
      }
      _delay_ms(100); // allow some time for settling
    } else _delay_ms(10);
  } else if (stage == VOLTREF) {
#if defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny2313A__) || defined(__AVR_ATtiny4313__) \
    || defined(__AVR_ATtiny13__) // these do not support Vcc measuring
    txstr(F("MCU does not support measuring Vcc"));
    txstr(F("\n\r"));
    error |= VOLTREF_ERROR;
    stage = FINISH;
#else
    volt = Vcc::measure(1000,DEFINTREF);
    txstr(F("True voltage (mV): "));
    txstr(itoa(TRUEMILLIVOLT,valstr,10));
    txstr(F("\n\r"));
    txstr(F("Measured (mV):     "));
    txstr(itoa(volt,valstr,10));
    intref = (((DEFINTREF * 1023L) / volt) * TRUEMILLIVOLT) / 1023L;
    txstr(F("Correct intref value: "));
    txstr(itoa(intref,valstr,10));
    EEPROM.put(E2END-1,intref);
    stage = FINISH;
#endif
    return;
  } else if (stage == FINISH) {
    if (error == 0) {
      txstr(F("...done!\n\r"));
      while (1);
    } else {
      txstr(F("...not completely successful:\n\r"));
      if (error & FREQ_ERROR)
	txstr(F(" - OSCCAL calibration failed\n\r"));
      if (error & VOLTREF_ERROR)
	txstr(F(" - internal reference voltage calibration failed\n\r"));
      while (1);
    }
  }
}

unsigned long getCount(void)
{
  unsigned int temp;
  cli();
  temp = count;
  count = 0;
  sei();
  return temp;
}

unsigned long getAvgCount(void)
{
  unsigned int vals[3];
  unsigned int temp;
  byte i = 0;
  while (i < 3) {
    _delay_ms(50);
    temp = getCount();
    if  (temp > EXPECTED_TICKS - MAX_DEVIATION && temp < EXPECTED_TICKS + MAX_DEVIATION)  {
      vals[i++] = temp;
      //txstr(F("val: ")), mySerial.println(temp);
    }
  }
  if ((vals[0] <= vals[1] && vals[1] <= vals[2]) || (vals[0] >= vals[1] && vals[1] >= vals[2])) return vals[1];
  if ((vals[1] <= vals[0] && vals[0] <= vals[2]) || (vals[1] >= vals[0] && vals[0] >= vals[2])) return vals[0];
  return vals[2];
}

void limitReached(void)
{
  txstr(F("OSCCAL limit reached\n\r"));
  error = FREQ_ERROR;
  stage = VOLTREF;
}


// I/O stuff


void txchar(byte b) {
  // By declaring these as local variables, the compiler will put them
  // in registers _before_ disabling interrupts and entering the
  // critical timing sections below, which makes it a lot easier to
  // verify the cycle timings
  volatile byte *reg = txportreg;
  unsigned int delay = txdelay;
  byte reg_mask = txbitmsk;
  byte inv_mask = ~txbitmsk;
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
  char* s;
  s=reinterpret_cast<PGM_P>(f);
  while (pgm_read_byte(s)) txchar(pgm_read_byte(s++));
}
