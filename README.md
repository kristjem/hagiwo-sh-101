# Strip board layout and firmware of HAGIWOs SH-101 eurorack sequencer

Original documentation by HAGIWO can be found [here (using Google Translate).](https://note-com.translate.goog/solder_state/n/n80f4baf81226?_x_tr_sl=en&_x_tr_tl=no&_x_tr_hl=no&_x_tr_pto=wapp&_x_tr_hist=true)

## Installation of firmware
1. Download firmware file and open it in Arduino IDE
2. See [the resources here](https://wiki.seeedstudio.com/Seeeduino-XIAO/) to make the Seeeduino XIAO available in Arduino IDE
3. Make sure to install dependencies. These are:
* Encoder (by Paul Stoffregen)
* Adafruit SSD1306
4. Compile and Upload the firmware

## Calibrating the module
First make sure the software is calibrated. The _calb value should be somewhere between 0.8 and 1.2. For my particular build, 1.15 seems to work and gives a somewhat stable tracking over some octaves. The software calibration affects the "spread" between octaves or notes:
>`float AD_CH1_calb = 1.15;//reduce resistance error // CV in`<br> 
>`//float AD_CH2_calb = 1.094;//reduce resistance error // Trig in` 

Next, fine tune the trim pots to pitch it all up or down.
