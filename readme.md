# AVR MCU Calibration

This library contains just one function to set user calibration values and two sketches that can be used to determine the calibration values by connecting the target board with an Arduino UNO (or a similar board) using an ICSP cable.

### Purpose

If you use a crystal, a ceramic resonator, or an external oscillator, the MCU clock frequency is most probably very accurate, usually better than 0.1%. This means it can be used for timing purposes and to support asynchronous communication. However, if you are relying on the internal RC oscillator, the guaranteed accuracy is often much worse. For some MCUs, only ±10% is guaranteed. This is not tolerable for asynchronous serial communication. The accuracy can be improved significantly by setting the OSCCAL register accordingly. Similarly, the internal reference voltage is nominally 1.1 volt. But it is only guaranteed in the range of ±10%. Again, here one can improve accuracy by determining the exemplar specific value. And with that you can very precisely measure the supply voltage, which is very helpful when running on batteries. 

The `avrCalibrate` library contains just one function, called `init`, that does nothing more than loading predetermined values to the `OSCCAL` register (to adjust the MCU clock frequency) and to an internal variable (for remembering the true internal reference voltage) at startup. With that, using the Vcc library later on, you can get precise measurements of the supply voltage.


### Calibration process

The predetermined calibration values can either be stored in EEPROM or can be provided as constant values. The tricky part is, of course, to determine these calibration values. For that purpose, two Arduino sketches are provided in the `utility` folder. `calibServer` is a sketch to be loaded on an ATmega328P or similar board that uses a ceramic resonator or crystal. It generates a reasonably accurate 10 Hz signal that is used to calibrate the `OSCCAL` value on the target board. The `calibTarget` sketch needs to be loaded to the target board using a programmer. Before you do that, you need to adjust the compile-time constant `TRUEMILLIVOLT` to the true supply voltage of the target board (which should be measured using an accurate Multimeter). You then need to connect the two boards using an ICSP cable (see below). After pressing the `RESET` button on the server board, which will also reset the target board, the calibrations process starts.

The calibration values will be stored in EEPROM in the last 4 bytes. The first byte is zero, if the stored OSCCAL value is valid. The second byte is the OSCCAL value to be loaded at startup. The final 2 bytes provide the internal reference voltage value. 

If you want to use these values, you need to set the `EESAVE` fuse on the target board in order to make sure that the EEPROM is not cleared when you upload the next sketch. This can either be done using a decent fuse setting program such as AVRFuses on the Mac, by using an online tool such as [Engbedded Fuse Calculator](https://www.engbedded.com/fusecalc/) and avrdude, or by selecting this option under the tools menu, provided you use the [ATTinyCore](https://github.com/SpenceKonde/ATTinyCore). 

Instead of using the EEPROM values, you can write down the values that are shown during the calibration process and feed them to the init method (see example sketches). This saves you from messing around with fuses, but works only, if you want to deploy just one board.


### Hardware setup

The simplest way to connect the server board to the target board is to use an ICSP cable. However, this works only if you plan to run the target board with the same supply voltage as the one used for the server board because the calibration is supply voltage dependent. Furthermore, if you source a target board with 5V and the board is not 5V tolerant, you may actually destroy it. 

In order to deal with this problem, you may want to consider to buy a server board with switchable supply voltage such as [Seeduino 4.3](https://www.seeedstudio.com/Seeeduino-V4-2-p-2517.html) or [Keyestudio 328 PLUS Board](https://wiki.keyestudio.com/KS0486_Keyestudio_PLUS_Development_Board_(Black_And_Eco-friendly)) in order to overcome this hurdle. 

Alternatively, you can individually connect the MOSI, MISO, RESET, and GND pins on the ICSP connectors, and provide the target board with its own individual supply voltage. As long as it more than 3.3V, the target board will be able to talk to the server board. Further the 10 Hz signal  is generated as an open collector signal, where the pull-up voltage comes from the target board. If you plan to run the board with a lower supply voltage, you will need level shifters.

Instead of connecting the pins individually, you may also build an ICSP cable, where the Vcc line is broken out so that it can be connected to either 5V or 3.3V on the server board. This could look like as in the following picture.

![ICSP cable with Vcc breakout](pics/ICSP.JPG)

### Hardware requirements

The following MCUs can be used on the server side (the tested ones are in boldface):

* __ATmega328P__, ATmega328, ATmega168P(A), ATmega168(A), ATmega88P(A), ATmega88(A) 
* ATmega1284P, ATmega1284, ATmega644P, ATmega644, ATmega324P, ATmega324
* ATmega2560
* ATmega32U4

As mentioned above, their clock frequency should be supplied by a reasonable accurate ceramic resonator or, even better, by a quartz crystal. If using a resonator, you may want to measure the 10 Hz signal at the MISO pin for accuracy. If it is more than 0.5 % off, you can change the compile-time constant `TRUETICKS` in the `calibTarget` sketch.

As targets, it is planned to support the following MCUs (boldface ones are actually supported already). On MCUs with only 2K bytes flash memory, the target sketch is less verbose than on the larger MCUs. With these MCUs, it is also necessary to disable the `millis`/`micros` code in order to save some flash memory. Otherwise the sketch is too large.

* ATtiny43U
* ATtiny2313(A), ATtiny4313 (only `OSCCAL` calibration)
* __ATtiny24(A)__, __ATtiny44(A)__, __ATtiny84(A)__
* ATtiny441, ATtiny841
* __ATtiny25__, __ATtiny45__, __ATtiny85__
* ATtiny26
* __ATtiny261(A)__, __ATtiny461(A)__, __ATtiny861(A)__
* ATtiny87, ATtiny167
* ATtiny828
* ATtiny48, ATtiny88
* __ATtiny1634__
* ATmega48, ATmega48A, ATmega48PA, ATmega48PB, ATmega88, ATmega88A, ATmega88PA, Atmega88PB, ATmega168, ATmega168A, ATmega168PA, ATmega168PB, ATmega328, ATmega328P, ATmega328PB
* ATmega324P, ATmega324, ATmega644P, ATmega644, ATmega1284P, ATmega1284
* ATmega1280, ATmega2560

### Disclaimer
This is still alpha software and some things might not work as advertised. 