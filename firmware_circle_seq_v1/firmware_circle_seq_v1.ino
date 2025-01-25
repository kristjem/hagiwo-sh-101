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
byte pattern[16] = {0, 5, 7, 0}; // Default pattern: [C, F, G, C]
int step = 0; // Declare step as a global variable
int clock_counter = 0; // Counter for clock division
bool is_major = true; // Track if the scale is major or minor

const char* note_names[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

// Major and minor scales for each root note
const int major_scales[12][7] = {
  {0, 2, 4, 5, 7, 9, 11}, // C major
  {1, 3, 5, 6, 8, 10, 0}, // C# major
  {2, 4, 6, 7, 9, 11, 1}, // D major
  {3, 5, 7, 8, 10, 0, 2}, // D# major
  {4, 6, 8, 9, 11, 1, 3}, // E major
  {5, 7, 9, 10, 0, 2, 4}, // F major
  {6, 8, 10, 11, 1, 3, 5}, // F# major
  {7, 9, 11, 0, 2, 4, 6}, // G major
  {8, 10, 0, 1, 3, 5, 7}, // G# major
  {9, 11, 1, 2, 4, 6, 8}, // A major
  {10, 0, 2, 3, 5, 7, 9}, // A# major
  {11, 1, 3, 4, 6, 8, 10} // B major
};

const int minor_scales[12][7] = {
  {0, 2, 3, 5, 7, 8, 10}, // C minor
  {1, 3, 4, 6, 8, 9, 11}, // C# minor
  {2, 4, 5, 7, 9, 10, 0}, // D minor
  {3, 5, 6, 8, 10, 11, 1}, // D# minor
  {4, 6, 7, 9, 11, 0, 2}, // E minor
  {5, 7, 8, 10, 0, 1, 3}, // F minor
  {6, 8, 9, 11, 1, 2, 4}, // F# minor
  {7, 9, 10, 0, 2, 3, 5}, // G minor
  {8, 10, 11, 1, 3, 4, 6}, // G# minor
  {9, 11, 0, 2, 4, 5, 7}, // A minor
  {10, 0, 1, 3, 5, 6, 8}, // A# minor
  {11, 1, 2, 4, 6, 7, 9} // B minor
};

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
    menu = (menu + 1) % 7; // Cycle through 7 menu options
    Serial.print("Encoder Position: ");
    Serial.println(newPosition);
    Serial.print("Menu: ");
    Serial.println(menu);
    disp_reflesh = 1; // Mark display for refresh
  } else if ((newPosition + 3) / 4 < oldPosition / 4) { // Adjust the resolution to avoid multiple shifts
    oldPosition = newPosition;
    menu = (menu - 1 + 7) % 7; // Ensure menu wraps around correctly
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
      root_note = (root_note + 1) % 12; // Cycle through 12 notes (C to B)
      break;
    case 1:
      is_major = !is_major; // Toggle between major and minor scales
      break;
    case 2:
      if (sequence_length == 4) {
        // Transition from 4 to 8 steps
        for (int i = 0; i < 4; i++) {
          pattern[4 + i] = pattern[i]; // Duplicate the first 4 steps
        }
        // Switch the last two notes
        byte temp = pattern[6];
        pattern[6] = pattern[7];
        pattern[7] = temp;
      } else if (sequence_length == 8) {
        // Transition from 8 to 16 steps
        for (int i = 0; i < 8; i++) {
          pattern[8 + i] = pattern[7 - i]; // Reverse the order of the previous 8 steps
        }
      }
      sequence_length = (sequence_length == 4 ? 8 : (sequence_length == 8 ? 16 : 4));
      break;
    case 3:
      current_div = (current_div + 1) % 7;
      break;
    case 4:
      // Pattern update (example: toggle between two patterns)
      if (pattern[0] == 0 && pattern[1] == 0) {
        byte new_pattern[16] = {0, 0, 7, 5, 0, 0, 7, 5};
        memcpy(pattern, new_pattern, sizeof(pattern));
      } else {
        byte new_pattern[16] = {0, 0, 7, 5};
        memcpy(pattern, new_pattern, sizeof(pattern));
      }
      break;
    case 5:
      randomizePattern();
      break;
    case 6: // Reset sequence
      resetSequence();
      break;
  }
}

void randomizePattern() {
  const int* scale = is_major ? major_scales[root_note] : minor_scales[root_note];
  for (int i = 0; i < sequence_length; i++) {
    if (i % 4 == 0) {
      pattern[i] = 0; // Ensure the first note of each 4-step segment is the Root Note
    } else {
      pattern[i] = scale[random(0, 7)]; // Randomize pattern with valid notes from the selected scale
    }
  }
}

int getNoteFromPattern(int root, int step) {
  int note_index = (root + pattern[step % sequence_length]) % 12;
  return note_index;
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

    // Calculate the starting Y position based on the current menu item
    int startY = 0;
    if (menu > 3) {
      startY = (menu - 3) * 10;
    }

    // Draw menu items with a dot indicator for the current menu position
    display.setCursor(0, 0 - startY);
    if (menu == 0) display.print("> ");
    display.print("Root Note: ");
    display.print(note_names[root_note]);

    display.setCursor(0, 10 - startY);
    if (menu == 1) display.print("> ");
    display.print("Scale: ");
    display.print(is_major ? "Major" : "Minor");

    display.setCursor(0, 20 - startY);
    if (menu == 2) display.print("> ");
    display.print("Seq Length: ");
    display.print(sequence_length);

    display.setCursor(0, 30 - startY);
    if (menu == 3) display.print("> ");
    display.print("Div: ");
    display.print(div_options[current_div]);

    display.setCursor(0, 40 - startY);
    if (menu == 4) display.print("> ");
    display.print("Pattern: ");
    for (int i = 0; i < sequence_length; i++) {
      if (i == (step + sequence_length - 1) % sequence_length) {
        display.setTextColor(BLACK, WHITE); // Highlight the current step
      } else {
        display.setTextColor(WHITE);
      }
      int note = getNoteFromPattern(root_note, i);
      display.print(note_names[note]);
      display.print(" ");
    }

    // Adjust position based on sequence length
    int resetSequenceY = 50 - startY;
    if (sequence_length > 4) {
      resetSequenceY += 10; // Move down for 8 steps
    }
    if (sequence_length > 8) {
      resetSequenceY += 10; // Move further down for 16 steps
    }
    if (sequence_length == 16) {
      resetSequenceY -= 10; // Adjust back up for 16 steps to avoid empty line
    }
    display.setCursor(0, resetSequenceY);
    display.setTextColor(WHITE); // Ensure the "Reset Sequence" text is not highlighted
    if (menu == 5) display.print("> ");
    display.print("Randomize Pattern");

    resetSequenceY += 10;
    display.setCursor(0, resetSequenceY);
    display.setTextColor(WHITE); // Ensure the "Reset Sequence" text is not highlighted
    if (menu == 6) display.print("> ");
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