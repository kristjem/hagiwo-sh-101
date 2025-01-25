#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include <Encoder.h>

#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define ENCODER_OPTIMIZE_INTERRUPTS
Encoder myEnc(6, 3);
float oldPosition  = -999;
float newPosition = -999;
bool buttonState = HIGH;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const int debounceDelay = 100;
bool buttonPressed = false;
bool disp_reflesh = 1; // Declare the disp_reflesh variable

const byte div_options[7] = {1, 2, 4, 8, 16, 32, 64};
byte current_div = 0;
byte menu = 0;
byte root_note = 0;
byte sequence_length = 4;
byte pattern[16] = {1, 1, 4, 3}; // Default pattern
int step = 0; // Declare step as a global variable
int clock_counter = 0; // Counter for clock division

const int circle_of_fifths_major[12] = {0, 7, 2, 9, 4, 11, 6, 1, 8, 3, 10, 5};
const int circle_of_fifths_minor[12] = {0, 5, 10, 3, 8, 1, 6, 11, 4, 9, 2, 7};
const char* note_names[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

void setup() {
  analogWriteResolution(12);
  pinMode(7, INPUT_PULLDOWN); // CLK in
  pinMode(8, OUTPUT); // CV out
  pinMode(9, OUTPUT); // Trigger out
  pinMode(10, INPUT_PULLUP); // Push button
  pinMode(1, OUTPUT); // CH1 gate out
  pinMode(2, OUTPUT); // CH2 gate out

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  display.clearDisplay();

  Wire.begin();

  // Initialize Serial for debugging
  Serial.begin(9600);
}

void loop() {
  handleEncoder();
  handleButtonPress();

  // Menu Navigation and Settings
  if (buttonPressed) {
    buttonPressed = false;
    updateMenu();
  }

  // Handle clock input and generate sequence
  static bool old_CLK_in = 0;
  bool CLK_in = digitalRead(7);
  if (old_CLK_in == 0 && CLK_in == 1) {
    clock_counter++;
    if (clock_counter >= div_options[current_div]) {
      clock_counter = 0;
      generateSequence();
    }
  }
  old_CLK_in = CLK_in;

  // Refresh Display
  refreshDisplay();
}

void handleEncoder() {
  newPosition = myEnc.read();
  if ((newPosition - 3) / 4 > oldPosition / 4) { // Adjust the resolution to avoid multiple shifts
    oldPosition = newPosition;
    menu = (menu + 1) % 5; // Cycle through 5 menu options
    Serial.print("Encoder Position: ");
    Serial.println(newPosition);
    Serial.print("Menu: ");
    Serial.println(menu);
    disp_reflesh = 1; // Mark display for refresh
  } else if ((newPosition + 3) / 4 < oldPosition / 4) { // Adjust the resolution to avoid multiple shifts
    oldPosition = newPosition;
    menu = (menu - 1 + 5) % 5; // Ensure menu wraps around correctly
    Serial.print("Encoder Position: ");
    Serial.println(newPosition);
    Serial.print("Menu: ");
    Serial.println(menu);
    disp_reflesh = 1; // Mark display for refresh
  }
}

void handleButtonPress() {
  bool reading = digitalRead(10);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        buttonPressed = true;
        Serial.println("Button Pressed");
        disp_reflesh = 1; // Mark display for refresh
      }
    }
  }
  lastButtonState = reading;
}

void updateMenu() {
  switch (menu) {
    case 0:
      root_note = (root_note + 1) % 24; // Cycle through 24 notes (12 major + 12 minor)
      break;
    case 1:
      sequence_length = (sequence_length == 4 ? 8 : (sequence_length == 8 ? 16 : 4));
      // Initialize new pattern elements to a default value
      for (int i = 4; i < sequence_length; i++) {
        pattern[i] = 1; // Default to 1 (C note in C major scale)
      }
      break;
    case 2:
      current_div = (current_div + 1) % 7;
      break;
    case 3:
      // Pattern update (example: toggle between two patterns)
      if (pattern[0] == 1 && pattern[1] == 1) {
        byte new_pattern[16] = {1, 1, 3, 4, 1, 1, 4, 3};
        memcpy(pattern, new_pattern, sizeof(pattern));
      } else {
        byte new_pattern[16] = {1, 1, 4, 3};
        memcpy(pattern, new_pattern, sizeof(pattern));
      }
      break;
    case 4: // Reset sequence
      resetSequence();
      break;
  }
}

int getNoteFromPattern(int root, int step) {
  int scale_index = root / 12;
  int note_index = root % 12;
  if (scale_index == 0) { // Major scale
    return (note_index + circle_of_fifths_major[pattern[step % sequence_length] - 1]) % 12;
  } else { // Minor scale
    return (note_index + circle_of_fifths_minor[pattern[step % sequence_length] - 1]) % 12;
  }
}

void generateSequence() {
  int note = getNoteFromPattern(root_note, step);
  int cv_value = map(note, 0, 12, 0, 4095); // Map note to CV range

  // Output CV using onboard DAC and external DAC
  intDAC(cv_value); // Onboard DAC
  MCP(cv_value); // External DAC

  // Gate out LOW (active)
  digitalWrite(1, LOW); // CH1 gate out
  digitalWrite(2, LOW); // CH2 gate out
  delay(5);
  // Gate out HIGH (inactive)
  digitalWrite(1, HIGH); // CH1 gate out
  digitalWrite(2, HIGH); // CH2 gate out

  step++;
  if (step >= sequence_length) {
    step = 0;
  }
  disp_reflesh = 1; // Mark display for refresh
}

void resetSequence() {
  step = 0;
}

void refreshDisplay() {
  if (disp_reflesh == 1) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    // Draw menu items with a dot indicator for the current menu position
    display.setCursor(0, 0);
    if (menu == 0) display.print("> ");
    display.print("Root Note: ");
    display.print(root_note);

    display.setCursor(0, 10);
    if (menu == 1) display.print("> ");
    display.print("Seq Length: ");
    display.print(sequence_length);

    display.setCursor(0, 20);
    if (menu == 2) display.print("> ");
    display.print("Div: ");
    display.print(div_options[current_div]);

    display.setCursor(0, 30);
    if (menu == 3) display.print("> ");
    display.print("Pattern: ");
    for (int i = 0; i < sequence_length; i++) {
      if (i == step % sequence_length) {
        display.setTextColor(BLACK, WHITE); // Highlight the current step
      } else {
        display.setTextColor(WHITE);
      }
      int note = getNoteFromPattern(root_note, i);
      display.print(note_names[note]);
      display.print(" ");
    }

    display.setCursor(0, 40 + (sequence_length > 8 ? 10 : 0)); // Adjust position based on sequence length
    if (menu == 4) display.print("> ");
    display.print("Reset Sequence");

    display.display();
    disp_reflesh = 0; // Reset display refresh flag
  }
}

// Onboard DAC output function
void intDAC(int intDAC_OUT) {
  analogWrite(A0, intDAC_OUT / 4); // "/4" -> 12bit to 10bit
}

// External DAC output function
void MCP(int MCP_OUT) {
  Wire.beginTransmission(0x60);
  Wire.write((MCP_OUT >> 8) & 0x0F);
  Wire.write(MCP_OUT);
  Wire.endTransmission();
}