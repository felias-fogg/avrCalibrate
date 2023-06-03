# AVR MCU Calibration

### Purpose

If you use a crystal, a ceramic resonator, or an external oscillator, the MCU clock frequency is most probably very accurate, usually better than 0.1%. However, if you are relying on the internal RC oscillator, the specification for accuracy is often much worse. For the ATtiny1634, the guaranteed accuracy of the RC oscillator is actually only ±10%. This can be improved setting the OSCCAL register accordingly. Similarly, the internal reference voltage is nominally 1.1 volt. But it is only guaranteed in the range of ±10%. Again, here one can improve accuracy by determine the exemplar specific value.

The `avrCalibrate` library contains just one function, called `init`, that does nothing more than loading predetermined values to the OSCCAL register (to adjust the MCU clock frequency) and to an internal variable (for remembering the true internal reference voltage) at startup. These predetermined values can either be stored in EEPROM or be provided as constant values. 

### Calibration process

The tricky part is, of course, to determine the calibration values. For that purpose, two Arduino sketches are provided in the `utility` folder of the library folder. `calibServer` is a sketch to be loaded on an ATmega328P or similar board that uses a ceramic resonator or crystal. It generates a reasonably accurate 100 Hz signal (usually with less than 0.1% deviation) that is used to calibrate the OSCCAL value on the target board. The `calibTarget` sketch needs to be loaded to the target board (most probably using a programmer). Before you do that, you need to adjust the compile constant constant `TRUEMILLIVOLT` to the true supply voltage of the target board (which should be measured using an accurate Multimeter). 

The calibration values will be stored in EEPROM in the last 3 bytes. If you want to use these values, you need to set the EESAVE fuse on the target board in order to make sure that the EEPROM is not cleared when you upload the next sketch. This can either be done using a decent fuse setting program such as AVRFuses on the Mac, by using an online tool such as [Engbedded Fuse Calculator](https://www.engbedded.com/fusecalc/) and avrdude, or by selecting this option under the tools menu, provided you use the [ATTinyCore](https://github.com/SpenceKonde/ATTinyCore). 

Instead of using the EEPROM values, you can write down the values that are shown during the calibration process and feed them to the init method (see example sketches). This saves you from messing around with fuses.


### Hardware setup

The simplest way to connect the server board with the target board is to use an ICSP cable and plug the ends into the respective ICSP connectors on both boards. However, this works **only if** both boards can use the same supply voltage. If the server board uses 5V and the target board tolerates only 3.3V, then do not do this! It may lead to the destruction of the electronic parts that are not 5V tolerant. You may want to consider to buy a server board with switchable supply voltage such as [Seeduino 4.3](https://www.seeedstudio.com/Seeeduino-V4-2-p-2517.html) or a [Keyestudio 328 PLUS Board](https://wiki.keyestudio.com/KS0486_Keyestudio_PLUS_Development_Board_(Black_And_Eco-friendly)) in order to overcome this hurdle. 

Alternatively, you can set the compile time constant PUSHPULL in the avrServer sketch to false, provide the target board with its own voltage and install a pull-up resistor of 4.7 kΩ between the target supply voltage and the MOSI pin, which is used to transmit the 100 Hz signal. Well, one might even think about constructing a special ICSP cable with a broken out Vcc pin for the server board that allows choosing 5V or 3.3V and an integrated pull-up resistor.

### Hardware requirements

The following MCUs can be used on the server side (the tested ones are in boldface):

* __ATmega328P__, ATmega328, ATmega168P, ATmega168, ATmega88P, ATmega88 
* ATmega1284P, ATmega1284, ATmega644P, ATmega644, ATmega324P, ATmega324
* ATmega2560
* ATmega32U4

As a target, it is planned to support the following MCUs (boldface ones are actually supported already):

* ATtiny43U
* ATtiny2313(A), ATtiny4313 (only OSCCAL calibtration)
* __ATtiny24(A)__, __ATtiny44(A)__, __ATtiny84(A)__
* ATtiny441, ATtiny841
* ATtiny25, ATtiny45, ATtiny85
* ATtiny261(A), ATtiny461(A), ATtiny861(A)
* ATtiny87, ATtiny167
* ATtiny828
* ATtiny48, ATtiny88
* ATtiny1634
* ATmega 32U4, ATmega16U4
* ATmega48, ATmega48A, ATmega48PA, ATmega48PB, ATmega88, ATmega88A, ATmega88PA, Atmega88PB, ATmega168, ATmega168A, ATmega168PA, ATmega168PB, ATmega328, ATmega328P, ATmega328PB
* ATmega324P, ATmega324, ATmega644P, ATmega644, ATmega1284P, ATmega1284
* ATmega1280, ATmega2560
