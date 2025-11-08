# Bassline Key Sequencer (Dual-Channel) for Seeeduino XIAO

A musical, pattern-based **bassline sequencer** for Eurorack that runs on the same hardware as the Hagiwo SH-101-style sequencer (Seeeduino XIAO + SSD1306 OLED + encoder + button + clock in).  
It generates diatonic bass motifs that **stay in key**, and it can **auto-change patterns every N bars**. Two independent CV/Gate outputs are available; **CH2 mirrors CH1** with a selectable transpose (e.g., +12 semitones for octave lines).

> Built to be drop-in compatible with the original Hagiwo wiring and pinout.

---

## Features

- **Key & Scale aware**: choose any root (C..B) and Major/Minor.
- **Musical patterns**: predefined 4- and 8-step templates designed for bass.
- **Flexible length**: use each pattern’s native length or force to 8 steps.
- **Clock division per channel**: groove against your master clock.
- **Auto pattern change**: optionally cycle pattern every 1/2/4/8/16/32 bars.
- **Dual output**:
  - **CH1** = main bassline.
  - **CH2** = same pattern, user-selectable transpose (default +12).
- **Compact OLED UI**: one encoder + push button for everything.
- **Tight gates**: short 10 ms gate pulses on each step by default.

---

## Hardware

This firmware assumes the exact Hagiwo SH-101-style module connections:

- **MCU**: Seeeduino XIAO
- **OLED**: SSD1306 128x64 (I2C at `0x3C`)
- **Encoder**: A (D6), B (D3), Push Switch (D10)
- **Clock In**: D7 (rising edge advances)
- **CH1 Gate**: D1 (active LOW)
- **CH2 Gate**: D2 (active LOW)
- **CH1 CV**: Internal DAC (`A0`) via `intDAC()`
- **CH2 CV**: MCP4725 DAC (I2C `0x60`) via `MCP()`

> Pins 8 and 9 are left defined as inputs for compatibility but are not required.

---

## Installation

1. **Arduino IDE & Libraries**
   - Install **Adafruit GFX** and **Adafruit SSD1306**.
   - Install **Encoder** library by Paul Stoffregen.
2. **Board Support**
   - Add the Seeeduino/SAMD board package appropriate for the XIAO.
3. **Clone / Copy**
   - Drop `bassline_key_sequencer.ino` into your sketch folder.
4. **Compile & Upload**
   - Select the **Seeeduino XIAO** board and correct port.
   - Upload.

---

## Controls

The UI is a single-line menu with **navigate** vs **edit** modes.

- **Rotate encoder**:
  - **Navigate mode**: move between parameters.
  - **Edit mode**: change the selected parameter’s value.
- **Press encoder**: toggle between **navigate** and **edit**.

### Parameters

- **Key**: C..B (transposes the entire pattern)
- **Scale**: Major / Minor (maps scale degrees to semitones)
- **Pattern**: 1..12 (predefined bass motifs)
- **Len**:
  - **Native**: use each pattern’s original 4 or 8 steps
  - **Force8**: force 8-step looping
- **Div1 / Div2**: clock divide per channel (1,2,4,8,16,32,64)
- **Auto**: enable/disable auto pattern cycling
- **Bars**: number of bars before changing pattern (1/2/4/8/16/32)
- **Tr2**: CH2 transpose in semitones (-24..+24), default **+12**
- **M1 / M2**: mute CH1 / CH2

> **Bars & bar counting**: 4 steps = 1 bar. When using 8-step patterns, a full pattern loop equals 2 bars.

---

## Musical Approach

Patterns are stored as **scale degrees** (1..7 with octave wrapping), so they automatically fit your selected **Key** and **Scale**. The result is a simple but effective generator of classic bass shapes (root-5th movement, scalar approaches, octave jumps, etc.).

- You can extend patterns by adding your own degree arrays.
- Want chromatic approaches? add ±1 semitone adjustments (see code comments for degree handling).

---

## Tips

- Use **Div2** plus **Tr2=+12** to create octave-up counterlines that don’t step every tick, adding motion without crowding the bass.
- Enable **Auto** with **Bars=4 or 8** for song-like phrasing where the bassline freshens every few bars.
- If your oscillator expects 1V/oct and the pitch seems off a little, adjust your analog chain or modify the lookup table.

---

## License

This project builds on open hardware ideas from Hagiwo’s SH-101-style sequencer and is provided under an MIT-style permissive license. Check original licenses for included libraries.

---

## Troubleshooting

- **No OLED**: verify I2C wiring, SSD1306 address `0x3C`, and power.
- **No CV on CH2**: check MCP4725 wiring and that it’s on `0x60`.
- **Step doesn’t advance**: ensure a clean 0→1 rising clock on D7.
- **Pitch range**: the internal table covers 61 semitones (0..60). Transpose plus key must stay within range.

---

## Roadmap Ideas

- Independent pattern for CH2.
- Accent/slide outputs.
- More scales/modes (Dorian, Mixolydian).
- CV inputs for Key/Pattern modulation.
