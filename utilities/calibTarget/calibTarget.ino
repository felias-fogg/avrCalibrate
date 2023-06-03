// This sketch can be used to calibrate the OSCCAL value (for getting
// a more accurate system clock when using the RC oscillator) and the
// bandgap reference voltage (for measuring, e.g., supply voltage). For the
// frequency calibration, you need an UNO, Leonardo, Mega or similar
// board, which generates a (reasonably) accurate 100 Hz signal.

// The result of the calibration is stored in EEPROM. By default, the
// the bandgap reference voltage is stored in as a two byte value in
// the last two bytes of EEPROM, the OSCCAL value is stored just
// before that. In addition, the values are also communicated over the
// SCK line using 1200 baud.

#define VERSION "0.1.0" 

#define TRUEMILLIVOLT 4989 // the true voltage measured in mV

#include <TXOnlySerial.h>
#include <util/delay.h>
#include <EEPROM.h>
#if !defined(__AVR_ATtiny2313__) && !defined(__AVR_ATtiny2313A__) && !defined(__AVR_ATtiny4313__) \
  && !defined(__AVR_ATtiny13__) // these do not support Vcc measuring
#include <Vcc.h>
#endif

// The ATmega8 has a different nominal bandgap refernce voltage
#ifdef __AVR_ATmega8__ 
#define DEFINTREF 1300
#else
#define DEFINTREF 1100
#endif

#define BAUD 1200

#define FREQ_TIMEOUT_SEC 30 // timeout for frequency measurement (<640)
#define MIN_MEASURE_CNT 100 // minimal number of frequency measurement before accept it as valid (<256)
#define EXPECTED_TICKS 10000 // number of expected ticks for 100 Hz signal
#define MAX_DEVIATION 5000  // maximal deviation (i.e., roughly 50%)

// The MOSI line is used as the input for the 100 Hz signal
// We use it as an interrupt input line
// In addition, we use TIMER0 as the calibration timer, counting the overflow separately.
// In case of F_CPU==8000000, we use a prescaler of 8, otherwiese 1. Since
// the input frequency is 100 Hz and we stop counting only on falling edges,
// a fully calibrated clock will give us 10000 counts.
#if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny24A__) || defined(__AVR_ATtiny44__) \
  || defined(__AVR_ATtiny44A__) || defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny84A__)  \
  || defined(__AVR_ATtiny441__) || defined(__AVR_ATtiny841__)
#define PINREG PINA
#define PINNUM 6
#define PCINT_vect PCINT0_vect
#define PCIE PCIE0
#define PCINT PCINT6
#define PCIMASK PCMSK0
#define GLOBMASK GIMSK
#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#define PCINT_vect PCINT0_vect
#define PCINT PCINT0
#define PCIMASK PCMSK0
#define GLOBMASK GIMSK
#elif defined(__AVR_ATtiny261__) || defined(__AVR_ATtiny261A__) || defined(__AVR_ATtiny461__) \
  || defined(__AVR_ATtiny461A__) || defined(__AVR_ATtiny861__) || defined(__AVR_ATtiny861A__)
#define PCIE PCIE1
#define PCINT PCINT8
#define PCIMASK PCMSK1
#define GLOBMASK GIMSK
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
long intref, volt;
byte osccal;


TXOnlySerial mySerial(SCK); // serial output over the SCK line


ISR(PCINT_vect)
{
  if ((PINREG & (1<<PINNUM)) == 0) { // falling edge
    count = ((unsigned int)ovfcount << 8) | TCNT0;
    ovfcount = 0;
    TCNT0 = 0;
    if (valid <= MIN_MEASURE_CNT) valid++;
  }
}

ISR(TIM0_COMPA_vect)
{
  ovfcount++;
}

void setup(void)
{
  TIMSK0 = 0; // disable millis interrupt
  TCCR0A = 0; // normal operation
  TCCR0B = (F_CPU == 8000000) ? 0b010 : 0b001; // WGM02=0, prescaler == 8 if 8 MHz, otherwise =1
  OCR0A = 0xFF; // interrupt whenever we reach 0xFF
  TIMSK0 = (1 << OCIE0A); // enable interrupt for reaching OCRA value
  GLOBMASK |= (1 << PCIE); // enable PCIs 
  PCIMASK |= (1 << PCINT); // enable PCI on frequency input pin
  mySerial.begin(BAUD);
  _delay_ms(2000);
  mySerial.println(F("\n\rcalibTarget Version " VERSION));
  stage = FREQ;
}

void loop(void)
{
  if (stage == FREQ) {
    if (F_CPU != 8000000 && F_CPU != 1000000) { // unsupported frequency
      mySerial.println(F("Unsupported MCU clock frequency"));
      error = FREQ_ERROR;
      stage = VOLTREF;
      return;
    }      
    if (++freqloop > FREQ_TIMEOUT_SEC * 100) { // more than 30 seconds
      mySerial.println(F("Timeout: could not calibrate MCU clock"));
      error = FREQ_ERROR;
      stage = VOLTREF;
      return;
    }
    if (valid > MIN_MEASURE_CNT) {
      _delay_ms(50); // allow for 4-5 measurements without any possible I/O interrupts
      currcount = getCount();
      if (currcount < EXPECTED_TICKS - MAX_DEVIATION || currcount > EXPECTED_TICKS + MAX_DEVIATION) {
	_delay_ms(10);
	return; // unreasonable values
      }
      if (dir == STOP) {
	mySerial.print(F("Final OSCCAL: 0x"));
	mySerial.println(OSCCAL,HEX);
	osccal = OSCCAL;
	EEPROM.put(E2END-2, osccal);
	mySerial.print(F("OSCCAL value stored in EEPROM at: 0x"));
	mySerial.println(E2END-2,HEX);
	stage = VOLTREF;
	return;
      }
      mySerial.print(F("OSCCAL: 0x"));
      mySerial.print(OSCCAL,HEX);
      mySerial.print(F(",  ticks: "));
      mySerial.println(currcount);
      if (currcount > EXPECTED_TICKS && dir == UP) { // higher freq and moving upwards
	posdiff = currcount - EXPECTED_TICKS;
	if (posdiff > minusdiff) {
	  OSCCAL-- ; // use last value
	}
	dir = STOP;
      } else if (currcount > EXPECTED_TICKS) {
	posdiff = currcount - EXPECTED_TICKS;
	OSCCAL--;
	dir = DOWN;
      } else if (currcount <= EXPECTED_TICKS && dir == DOWN) {
	minusdiff = EXPECTED_TICKS - currcount;
	if (posdiff < minusdiff) {
	  OSCCAL++ ; // use last value
	}
	dir = STOP;
      } else if (currcount <= EXPECTED_TICKS) {
	minusdiff = EXPECTED_TICKS - currcount;
	OSCCAL++;
	dir = UP;
      }
      _delay_ms(100); // allow some time for settling
    } else _delay_ms(10);
  } else if (stage == VOLTREF) {
#if defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny2313A__) || defined(__AVR_ATtiny4313__) \
    || defined(__AVR_ATtiny13__) // these do not support Vcc measuring
    mySerial.println(F("MCU does not support measuring Vcc"));
    error |= VOLTREF_ERROR;
    stage = FINISH;
#else
    mySerial.print(F("True voltage (mV): "));
    mySerial.println(TRUEMILLIVOLT);
    mySerial.print(F("Measured (mV):     "));
    volt = Vcc::measure(1000,DEFINTREF);
    mySerial.println(volt);
    intref = (((DEFINTREF * 1023L) / volt) * TRUEMILLIVOLT) / 1023L;
    mySerial.print(F("Correct intref value: "));
    mySerial.println(intref);
    EEPROM.put(E2END-1,intref);
    mySerial.print(F("INTREF value stored in EEPROM at: 0x"));
    mySerial.println(E2END-1,HEX);
    stage = FINISH;
#endif
    return;
  } else if (stage == FINISH) {
    if (error == 0) {
      mySerial.println(F("...done!"));
      while (1);
    } else {
      mySerial.println(F("...not completely successful"));      
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
