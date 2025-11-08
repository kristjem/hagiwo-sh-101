/*  Bassline Key Sequencer (dual-channel) for Seeeduino XIAO
    Hardware/pinout follows Hagiwo SH-101 sequencer exactly.

    - CH1 CV:  internal DAC (A0) via intDAC()
    - CH2 CV:  MCP4725 (I2C 0x60) via MCP()
    - CH1 Gate: D1  (LOW = gate ON)
    - CH2 Gate: D2  (LOW = gate ON)
    - Clock In: D7  (rising edge advances step)
    - Push SW : D10 (active HIGH)
    - Encoder : D6 (A), D3 (B)

    What this firmware does:
    - Generates musical bassline patterns from predefined templates.
    - Select Key (C..B) and Scale (Major/Minor).
    - Choose Pattern (4 or 8 steps) and Clock Divide per channel.
    - Optional: change pattern automatically every N bars.
    - CH2 mirrors CH1 but transposed (default +12 semitones; adjustable).

    Requires:
      - Adafruit_GFX, Adafruit_SSD1306
      - Encoder (Paul Stoffregen)
*/

#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>

/* ---------- OLED ---------- */
#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/* ---------- Encoder & UI ---------- */
Encoder myEnc(6, 3);
long oldPosition = -999;
long newPosition = -999;
bool SW = 0, old_SW = 0;

/* ---------- IO & runtime ---------- */
bool CLK_in = 0, old_CLK_in = 0;
int gate_timer1 = 0, gate_timer2 = 0;

/* ---------- Legacy CV tables (keep pinout and scaling) ---------- */
// Output pre-quantize table: 61 semitones (0..60) -> 12-bit DAC value (0..4095)
const int cv_qnt_out[61] = {
  0,  68, 137, 205, 273, 341, 410, 478, 546, 614, 683, 751,
  819,  887, 956, 1024, 1092, 1161, 1229, 1297, 1365, 1434, 1502, 1570,
  1638, 1707, 1775, 1843, 1911, 1980, 2048, 2116, 2185, 2253, 2321, 2389,
  2458, 2526, 2594, 2662, 2731, 2799, 2867, 2935, 3004, 3072, 3140, 3209,
  3277, 3345, 3413, 3482, 3550, 3618, 3686, 3755, 3823, 3891, 3959, 4028, 4095
};

/* ---------- Musical data ---------- */
const char* KEY_NAMES[12] = {
  "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
};
const byte MAJOR_SCALE[7] = {0,2,4,5,7,9,11};
const byte MINOR_SCALE[7] = {0,2,3,5,7,8,10};

/* Patterns are expressed as scale degrees (diatonic), where:
   0 = scale degree 1 (tonic), 1 = degree 2, ... 6 = degree 7,
   7..13 are the next octave degrees, etc.
*/
const int NUM_PATTERNS = 12;
const int PATTERN_LEN[NUM_PATTERNS] = {4,4,4,4,8,8,8,8,8,8,4,8};
const int patternSet[NUM_PATTERNS][8] = {
  {0,4,5,7, 0,0,0,0},         // 1: R–3–4–5
  {0,7,5,4, 0,0,0,0},         // 2: R–5–4–3
  {0,2,4,5, 0,0,0,0},         // 3: R–2–3–4
  {0,3,4,7, 0,0,0,0},         // 4: R–♭3/3–4–5
  {0,2,4,5, 7,5,4,2},         // 5: up & down
  {0,7,5,4, 3,4,5,7},         // 6: fifth-centric
  {0,4,7,12, 7,4,2,0},        // 7: triad + octave
  {0,2,0,4, 5,4,2,0},         // 8: approach/return
  {0,4,5,4, 7,5,4,2},         // 9: classic groove
  {0,2,3,5, 7,5,3,2},         // 10: minorish
  {0,5,7,12, 0,0,0,0},        // 11: octave jump
  {0,2,4,5, 7,9,7,5}          // 12: extend to 6th then back
};

/* ---------- User parameters (editable via menu) ---------- */
byte keyIndex = 0;         // 0..11 (C..B)
bool isMinor = false;      // false=Major, true=Minor
byte patternIndex = 0;     // 0..NUM_PATTERNS-1
bool forceLen8 = false;    // false = use pattern length, true = force 8
const byte div_ch1[7] = {1,2,4,8,16,32,64};
const byte div_ch2[7] = {1,2,4,8,16,32,64};
byte select_div_ch1 = 0, select_div_ch2 = 0;

const byte barChoices[6] = {1,2,4,8,16,32};
byte barChangeIndex = 0;   // change pattern every N bars (barChoices)
bool autoChangePattern = false;

int ch2Transpose = 12;     // semitone transpose for CH2 (default +12)
bool mute_ch1 = false;
bool mute_ch2 = false;

/* ---------- Transport / playback state ---------- */
unsigned long step_ch1_div = 0;
unsigned long step_ch2_div = 0;
byte stepPlay = 0;         // current position within pattern
unsigned long barCounter = 0; // counts completed bars
bool disp_reflesh = true;

/* ---------- Menu system ---------- */
enum MenuItem : byte {
  M_KEY = 1,
  M_SCALE,
  M_PATTERN,
  M_LEN_MODE,
  M_DIV_CH1,
  M_DIV_CH2,
  M_AUTOCHG_ONOFF,
  M_AUTOCHG_BARS,
  M_CH2_TRANSPOSE,
  M_MUTE_CH1,
  M_MUTE_CH2,
  M_MAX
};
byte menu = M_KEY;
bool editMode = false; // false = navigate, true = edit current item

/* ---------- Helpers ---------- */
int degreeToSemitone(int degree, bool minorScale) {
  const byte* scale = minorScale ? MINOR_SCALE : MAJOR_SCALE;
  int oct = floor((float)degree / 7.0f);
  int deg = degree % 7;
  if (deg < 0) { deg += 7; oct -= 1; }
  return scale[deg] + 12 * oct;
}

int applyKeyAndClamp(int semiFromRoot, int addKey) {
  int s = semiFromRoot + addKey;
  if (s < 0) s = 0;
  if (s > 60) s = 60; // match cv_qnt_out range
  return s;
}

/* ---------- Arduino setup ---------- */
void setup() {
  analogWriteResolution(10);
  analogReadResolution(12);

  pinMode(7, INPUT_PULLDOWN); // CLK in
  pinMode(8, INPUT);          // compatibility
  pinMode(9, INPUT);          // compatibility
  pinMode(10, INPUT_PULLUP);  // push SW
  pinMode(1, OUTPUT);         // CH1 gate out (LOW active)
  pinMode(2, OUTPUT);         // CH2 gate out (LOW active)

  Wire.begin();

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  display.clearDisplay();
  display.display();
}

/* ---------- Gate helpers ---------- */
void gateOn(uint8_t pin, int& timerVar) {
  digitalWrite(pin, LOW);
  timerVar = millis();
}
void gateOff(uint8_t pin) {
  digitalWrite(pin, HIGH);
}

/* ---------- Advance one step if clock divide satisfied ---------- */
void maybeAdvanceStep() {
  // CH1 divide counter
  step_ch1_div++;
  bool advance1 = (step_ch1_div >= div_ch1[select_div_ch1]);
  if (advance1) step_ch1_div = 0;

  // CH2 divide counter
  step_ch2_div++;
  bool advance2 = (step_ch2_div >= div_ch2[select_div_ch2]);
  if (advance2) step_ch2_div = 0;

  // Determine current effective pattern length
  int nativeLen = PATTERN_LEN[patternIndex];
  int effectiveLen = forceLen8 ? 8 : nativeLen;

  // On each channel that advances this clock, output note & gate
  if (advance1 && !mute_ch1) {
    int degree = patternSet[patternIndex][stepPlay % nativeLen];
    int semi = degreeToSemitone(degree, isMinor);
    semi = applyKeyAndClamp(semi, keyIndex);
    intDAC(cv_qnt_out[semi]);
    gateOn(1, gate_timer1);
  }
  if (advance2 && !mute_ch2) {
    int degree = patternSet[patternIndex][stepPlay % nativeLen];
    int semi = degreeToSemitone(degree, isMinor) + ch2Transpose;
    semi = applyKeyAndClamp(semi, keyIndex);
    MCP(cv_qnt_out[semi]);
    gateOn(2, gate_timer2);
  }

  // Only increment shared pattern step when at least one channel advanced
  if (advance1 || advance2) {
    stepPlay++;
    if (stepPlay >= effectiveLen) {
      stepPlay = 0;
      // count bars (define 4 steps = 1 bar)
      barCounter++;
      // auto change pattern on bar boundary
      if (autoChangePattern) {
        byte barsN = barChoices[barChangeIndex];
        if (barCounter % barsN == 0) {
          patternIndex = (patternIndex + 1) % NUM_PATTERNS;
        }
      }
    }
  }
}

/* ---------- UI handling ---------- */
void handleEncoderAndButton() {
  old_SW = SW;
  SW = (digitalRead(10) == HIGH);

  // Encoder delta
  newPosition = myEnc.read();
  bool turnedLeft  = ((newPosition - 3) / 4  > (oldPosition) / 4);
  bool turnedRight = ((newPosition + 3) / 4  < (oldPosition) / 4);

  if (turnedLeft)  { oldPosition = newPosition; onTurn(-1); }
  if (turnedRight) { oldPosition = newPosition; onTurn(+1); }

  // Button press toggles editMode or applies action
  if (SW && !old_SW) {
    editMode = !editMode;
    disp_reflesh = true;
  }
}

void onTurn(int dir) {
  disp_reflesh = true;
  if (!editMode) {
    // Navigate menu
    int m = menu + dir;
    if (m < M_KEY) m = M_MUTE_CH2;
    if (m > M_MUTE_CH2) m = M_KEY;
    menu = (byte)m;
    return;
  }

  // Edit the selected parameter
  switch (menu) {
    case M_KEY:
      keyIndex = (keyIndex + 12 + dir) % 12;
      break;
    case M_SCALE:
      if (dir != 0) isMinor = !isMinor;
      break;
    case M_PATTERN:
      patternIndex = (patternIndex + NUM_PATTERNS + dir) % NUM_PATTERNS;
      stepPlay = 0;
      break;
    case M_LEN_MODE:
      if (dir != 0) forceLen8 = !forceLen8;
      stepPlay = 0;
      break;
    case M_DIV_CH1:
      {
        int v = (int)select_div_ch1 + dir;
        if (v < 0) v = 6; if (v > 6) v = 0;
        select_div_ch1 = (byte)v;
        step_ch1_div = 0;
      }
      break;
    case M_DIV_CH2:
      {
        int v = (int)select_div_ch2 + dir;
        if (v < 0) v = 6; if (v > 6) v = 0;
        select_div_ch2 = (byte)v;
        step_ch2_div = 0;
      }
      break;
    case M_AUTOCHG_ONOFF:
      if (dir != 0) autoChangePattern = !autoChangePattern;
      break;
    case M_AUTOCHG_BARS:
      {
        int v = (int)barChangeIndex + dir;
        if (v < 0) v = 5; if (v > 5) v = 0;
        barChangeIndex = (byte)v;
        barCounter = 0;
      }
      break;
    case M_CH2_TRANSPOSE:
      {
        int t = ch2Transpose + dir; // allow range -24..+24
        if (t < -24) t = 24;
        if (t >  24) t = -24;
        ch2Transpose = t;
      }
      break;
    case M_MUTE_CH1:
      if (dir != 0) mute_ch1 = !mute_ch1;
      break;
    case M_MUTE_CH2:
      if (dir != 0) mute_ch2 = !mute_ch2;
      break;
  }
}

/* ---------- OLED drawing ---------- */
void drawLabelVal(int x, int y, const char* label, const char* val, bool selected) {
  if (selected && editMode) display.fillRect(x-2, y-1, 60, 9, WHITE);
  display.setCursor(x, y);
  display.setTextColor((selected && editMode) ? BLACK : WHITE);
  display.print(label);
  display.print(val);
  display.setTextColor(WHITE);
}

void OLED_display() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Title
  display.setCursor(0,0);
  display.print("Bassline Key Sequencer");

  // Left column (core musical params)
  drawLabelVal(0,12, "Key:", KEY_NAMES[keyIndex], menu==M_KEY);
  drawLabelVal(0,22, "Sca:", isMinor?"Minor":"Major", menu==M_SCALE);

  char buf[10];
  snprintf(buf, sizeof(buf), "%d/%d", (int)(patternIndex+1), (int)NUM_PATTERNS);
  drawLabelVal(0,32, "Pat:", buf, menu==M_PATTERN);
  drawLabelVal(0,42, "Len:", forceLen8?"Force8":"Native", menu==M_LEN_MODE);

  // Right column (timing & channel)
  char dbuf[6];
  snprintf(dbuf, sizeof(dbuf), "%u", div_ch1[select_div_ch1]);
  drawLabelVal(66,12, "Div1:", dbuf, menu==M_DIV_CH1);

  snprintf(dbuf, sizeof(dbuf), "%u", div_ch2[select_div_ch2]);
  drawLabelVal(66,22, "Div2:", dbuf, menu==M_DIV_CH2);

  drawLabelVal(66,32, "Auto:", autoChangePattern?"On":"Off", menu==M_AUTOCHG_ONOFF);

  char bbuf[6];
  snprintf(bbuf, sizeof(bbuf), "%u", barChoices[barChangeIndex]);
  drawLabelVal(66,42, "Bars:", bbuf, menu==M_AUTOCHG_BARS);

  // Bottom line: CH2 transpose & mutes
  char tbuf[8];
  snprintf(tbuf, sizeof(tbuf), "%d", ch2Transpose);
  drawLabelVal(0,54, "Tr2:", tbuf, menu==M_CH2_TRANSPOSE);
  drawLabelVal(40,54, "M1:", mute_ch1?"On":"Off", menu==M_MUTE_CH1);
  drawLabelVal(80,54, "M2:", mute_ch2?"On":"Off", menu==M_MUTE_CH2);

  display.display();
}

/* ---------- Main loop ---------- */
void loop() {
  // UI
  handleEncoderAndButton();

  // Clock edge detect
  old_CLK_in = CLK_in;
  CLK_in = digitalRead(7);
  if (!old_CLK_in && CLK_in) {
    maybeAdvanceStep();
    disp_reflesh = true;
  }

  // Gate lengths (10 ms)
  if (gate_timer1 + 10 >= (int)millis()) digitalWrite(1, LOW); else gateOff(1);
  if (gate_timer2 + 10 >= (int)millis()) digitalWrite(2, LOW); else gateOff(2);

  // Draw
  if (disp_reflesh) {
    OLED_display();
    disp_reflesh = false;
  }
}

/* ---------- CV output (unchanged from base) ---------- */
void intDAC(int intDAC_OUT) {
  analogWrite(A0, intDAC_OUT / 4); // 12bit -> 10bit for PWM DAC write
}

void MCP(int MCP_OUT) {
  Wire.beginTransmission(0x60);
  Wire.write((MCP_OUT >> 8) & 0x0F);
  Wire.write(MCP_OUT & 0xFF);
  Wire.endTransmission();
}
