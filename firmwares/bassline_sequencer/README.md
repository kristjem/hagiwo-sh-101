# Bassline Key Sequencer (Dual-Channel) for Seeeduino XIAO

A **pattern-based bassline sequencer** for Eurorack, designed for the Hagiwo SH-101-style hardware (Seeeduino XIAO + SSD1306 OLED + encoder + button + clock in).  
It generates **diatonic bass motifs that stay in key**, with optional **auto-pattern changes** every N bars. Two independent CV/Gate outputs are available; **CH2 mirrors CH1** with selectable transpose (e.g., +12 semitones for octave lines).

> Fully compatible with Hagiwo SH-101-style wiring and pinout.

---

## Features

- **Key & Scale aware** – choose any root (C..B) and Major/Minor. Patterns automatically conform to your key.
- **Musical patterns** – predefined 4- and 8-step bass motifs: roots, fifths, octaves, scalar runs.
- **Flexible pattern length** – use native pattern length or force 8-step loops.
- **Clock division per channel** – groove against a master clock.
- **Auto pattern change** – cycle pattern every 1/2/4/8/16/32 bars for song-like phrasing.
- **Dual outputs**:
  - **CH1** = main bassline.
  - **CH2** = mirrored pattern, user-selectable transpose (default +12 semitones).
- **Compact OLED UI** – one encoder + push button for navigation and editing.
- **Tight gates** – 10 ms pulses per step.
- **Smooth encoder handling** – rotations and clicks are separated for intuitive navigation and editing, even at fast turns.

---

## Hardware

- **MCU**: Seeeduino XIAO  
- **OLED**: SSD1306 128x64 (I2C `0x3C`)  
- **Encoder**: A (D6), B (D3), Push Switch (D10)  
- **Clock In**: D7 (rising edge advances)  
- **CH1 Gate**: D1 (LOW active)  
- **CH2 Gate**: D2 (LOW active)  
- **CH1 CV**: Internal DAC (`A0`) via `intDAC()`  
- **CH2 CV**: MCP4725 DAC (I2C `0x60`) via `MCP()`  

> Pins 8 and 9 are left defined as inputs for compatibility but are unused.

---

## Installation

1. **Arduino IDE & Libraries**
   - Install **Adafruit GFX** and **Adafruit SSD1306**.
   - Install **Encoder** library by Paul Stoffregen.
2. **Board Support**
   - Add the Seeeduino/SAMD board package appropriate for XIAO.
3. **Clone / Copy**
   - Drop `bassline_key_sequencer.ino` into a sketch folder.
4. **Compile & Upload**
   - Select the **Seeeduino XIAO** board and correct port.
   - Upload.

---

## Controls

UI operates in two modes: **navigate** vs **edit**.

- **Rotate encoder**:
  - **Navigate mode** – move between parameters.
  - **Edit mode** – change the selected parameter’s value.
- **Press encoder** – toggle between navigate and edit.

> Rotation and clicks are handled separately. A click immediately after a fast turn is ignored for 0.5s to prevent accidental toggles.

---

### Menu Parameters & Musical Role

| Menu | Description | Musical Idea / Usability |
|------|-------------|-------------------------|
| **Key** | C..B root note | Transposes the entire bassline; start in C for simple jams or in any key to fit your song. |
| **Scale** | Major / Minor | Maps pattern degrees to semitones; minor gives moody basslines, major is bright. |
| **Pattern** | 1..12 | Select a bass motif: root-fifth jumps, scalar movement, octaves. Great for instant groove variation. |
| **Len** | Native / Force8 | Native respects pattern’s 4/8 steps; Force8 ensures consistent 8-step loops for phrase alignment. |
| **Div1 / Div2** | Clock divide per channel (1,2,4,8,16,32,64) | Control step timing: create syncopated or slower counterlines. Use Div2 with Tr2 for octave movement without clutter. |
| **Auto** | On / Off | Enable automatic pattern cycling every N bars. Good for evolving basslines over time. |
| **Bars** | 1/2/4/8/16/32 | Number of bars before auto-changing pattern; 4 bars = 1 musical phrase. |
| **Tr2** | CH2 transpose (-24..+24) | Offset CH2 pattern by semitones; default +12 = octave counterline. Adds harmonic depth. |
| **M1 / M2** | Mute CH1 / CH2 | Silence channels individually to create breaks or reduce clutter. |

> **Bars & bar counting:** 4 steps = 1 bar. 8-step patterns count as 2 bars.

---

## Musical Approach

- Patterns are stored as **scale degrees**, automatically fitting your selected **Key** and **Scale**.  
- Designed for classic bass shapes: roots, fifths, octaves, scalar runs.  
- Extendable: add custom degree arrays for personal grooves.  
- For chromatic accents, modify ±1 semitone adjustments in code.

---

## Tips

- Use **Div2** + **Tr2 = +12** for octave counterlines that step less frequently than CH1.  
- Enable **Auto** + **Bars = 4 or 8** for song-like phrasing where basslines refresh periodically.  
- Adjust analog output scaling if your oscillator doesn’t track 1V/oct perfectly.

---

## License

MIT-style permissive license. Builds on open hardware concepts from Hagiwo SH-101 sequencer. Check included library licenses.

---

## Troubleshooting

- **No OLED**: verify I2C wiring, address `0x3C`, and power.  
- **No CV on CH2**: check MCP4725 wiring and I2C address `0x60`.  
- **Steps don’t advance**: ensure clean 0→1 rising edge on D7.  
- **Pitch range**: internal table covers 0..60 semitones. Key + Tr2 must stay in range.

---

## Roadmap Ideas

- Independent pattern for CH2.  
- Accent/slide outputs.  
- More scales/modes (Dorian, Mixolydian).  
- CV inputs for Key/Pattern modulation.
