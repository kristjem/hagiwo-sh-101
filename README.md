# Strip board layout and firmware of HAGIWOs SH-101 eurorack sequencer

Original documentation by HAGIWO can be found [here (using Google Translate).](https://note-com.translate.goog/solder_state/n/n80f4baf81226?_x_tr_sl=en&_x_tr_tl=no&_x_tr_hl=no&_x_tr_pto=wapp&_x_tr_hist=true)
<br>

### The firmware is sourced from HAGIWOs website, but some changes have been made:
1. #### Averaging ADC readings
    readings from the ADC (Analog-to-Digital Converter) were fluctuating a lot. I think I have a lot of noise in my module. These adjustments help to reduce noise and improve the precision of the measurements. See the function `float readAverageADC(int pin, int numSamples)`. The calibration factor (`AD_CH1_calb`) and averaging are applied during the ADC reading process to ensure accurate and stable measurements:
    ```cpp
    // Analog read and quantize with averaging and calibration
    AD_CH1 = readAverageADC(8, 10) / 4 * AD_CH1_calb; // 12-bit to 10-bit, average 10 samples
    ```
    The note C0 is at 0V, which should in theory output 0V from the SH-101 sequencer. But in practice, noise is introduced and quantized. This produces an off tone pitch for what should be C0 (0V). To fix this, there is an if statement that returns `readAverageADC` = 0, as long as the noise is below a set floor (found by debugging, for me et was about 65):
    ```cpp
    float readAverageADC(int pin, int numSamples) {
    float sum = 0;
    float average = 0;
    for (int i = 0; i < numSamples; i++) {
        sum += analogRead(pin);
    }
    average = sum / numSamples;
    // Serial.println("Average ADC value: " + String(average));
    if (average < 65) { // Adjust to make sure noise at 0V input is eliminated
        return 0;
    } else return average;
    }
    ```

2. #### Debouncing Issue with Rotary Encoder
    In this project, I encountered an issue with the rotary encoder where it would randomly register button presses (clicks) even when the button was not pressed. This was due to noise and spurious signals generated by the rotary encoder.

    ### Solution
    To resolve this issue, I implemented a debounce mechanism in the code. The debounce logic ensures that button presses are only registered when the button state is stable for a certain period of time.

    ### Adjusting the Debounce Delay
    The debounce delay can be adjusted to fine-tune the responsiveness and stability of the button press detection. The debounce delay is set in milliseconds and can be adjusted within the range of 50 to 100 milliseconds.

    ```cpp
    // Debounce settings
    const int debounceDelay = 100; // Adjust this value between 50 and 100 milliseconds
    ```

## Installation of firmware
1. Download firmware file and open it in Arduino IDE
2. See [the resources here](https://wiki.seeedstudio.com/Seeeduino-XIAO/) to make the Seeeduino XIAO available in Arduino IDE
3. Make sure to install dependencies. These are:
* Encoder (by Paul Stoffregen)
* Adafruit SSD1306
4. Compile and Upload the firmware

## Calibrating the module
First make sure the software is calibrated. The _calb value should be somewhere between 0.8 and 1.2. For my particular build, 1.15 seems to work and gives a somewhat stable tracking over some octaves. The software calibration affects the "spread" between octaves or notes:
```cpp
float AD_CH1_calb = 1.15;//reduce resistance error // CV in
//float AD_CH2_calb = 1.094;//reduce resistance error // Trig in
```
Next, fine tune the trim pots to pitch it all up or down.

### Stripboard layout:
![HAGIWO SH-101 strip board layout by Kristoffer Tjemsland](HAGIWO%20SH-101%20strip%20board%20layout%20by%20Kristoffer%20Tjemsland.png)