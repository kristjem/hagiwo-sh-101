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

const byte div_options[7] = {1, 2, 4, 8, 16, 32, 64};
byte current_div = 0;
byte menu = 0;
byte root_note = 0;
byte sequence_length = 4;
byte pattern[16] = {1, 1, 4, 3}; // Default pattern

const int circle_of_fifths_major[12] = {0, 7, 2, 9, 4, 11, 6, 1, 8, 3, 10, 5};
const int circle_of_fifths_minor[12] = {0, 5, 10, 3, 8, 1, 6, 11, 4, 9, 2, 7};

// Define menu items
enum MenuItem {
    ROOT_NOTE,
    SEQ_LENGTH,
    DIV,
    PATTERN,
    MENU_SIZE
};

MenuItem currentMenuItem = ROOT_NOTE;
int rootNote = 0;
int seqLength = 4;
int div = 1;

void setup() {
  analogWriteResolution(12);
  pinMode(7, INPUT_PULLDOWN); // CLK in
  pinMode(8, OUTPUT); // CV out
  pinMode(9, OUTPUT); // Trigger out
  pinMode(10, INPUT_PULLUP); // Push button

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  display.clearDisplay();

  Wire.begin();
}

void loop() {
  handleEncoderInput();
  handleButtonPress();
  updateOLED();

  // Menu Navigation and Settings
  if (buttonPressed) {
    buttonPressed = false;
    updateMenu();
  }

  // Handle clock input and generate sequence
  static bool old_CLK_in = 0;
  bool CLK_in = digitalRead(7);
  if (old_CLK_in == 0 && CLK_in == 1) {
    generateSequence();
  }
  old_CLK_in = CLK_in;

  // Refresh Display
  refreshDisplay();
}

void handleEncoderInput() {
  newPosition = myEnc.read();
  if ((newPosition / 4) != (oldPosition / 4)) {
    oldPosition = newPosition;
    currentMenuItem = (MenuItem)((currentMenuItem + (newPosition > oldPosition ? 1 : -1) + MENU_SIZE) % MENU_SIZE);
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
      }
    }
  }
  lastButtonState = reading;
}

void updateMenu() {
  switch (currentMenuItem) {
    case ROOT_NOTE:
      root_note = (root_note + 1) % 24; // Cycle through 24 notes (12 major + 12 minor)
      break;
    case SEQ_LENGTH:
      sequence_length = (sequence_length == 4 ? 8 : (sequence_length == 8 ? 16 : 4));
      break;
    case DIV:
      current_div = (current_div + 1) % 7;
      break;
    case PATTERN:
      // Pattern update (example: toggle between two patterns)
      if (pattern[0] == 1 && pattern[1] == 1) {
        byte new_pattern[16] = {1, 1, 3, 4, 1, 1, 4, 3};
        memcpy(pattern, new_pattern, sizeof(pattern));
      } else {
        byte new_pattern[16] = {1, 1, 4, 3};
        memcpy(pattern, new_pattern, sizeof(pattern));
      }
      break;
    default:
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
  static int step = 0;
  int note = getNoteFromPattern(root_note, step);
  int cv_value = map(note, 0, 12, 0, 4095); // Map note to CV range
  analogWrite(8, cv_value / 4); // Output CV (12-bit to 10-bit conversion)
  digitalWrite(9, HIGH); // Trigger pulse
  delay(5);
  digitalWrite(9, LOW);
  step++;
  if (step >= sequence_length * div_options[current_div]) {
    step = 0;
  }
}

void resetSequence() {
  static int step = 0;
  step = 0;
}

void refreshDisplay() {
  updateOLED();
}

void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.print("Root Note: ");
  display.print(root_note);

  display.setCursor(0, 10);
  display.print("Seq Length: ");
  display.print(sequence_length);

  display.setCursor(0, 20);
  display.print("Div: ");
  display.print(div_options[current_div]);

  display.setCursor(0, 30);
  display.print("Pattern: ");
  for (int i = 0; i < sequence_length; i++) {
    display.print(pattern[i]);
    display.print(" ");
  }

  display.display();
}