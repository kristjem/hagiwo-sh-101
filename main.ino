// ...existing code...

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
int pattern[4] = {1, 1, 4, 3};

// Function to handle encoder input
void handleEncoderInput() {
    // Read encoder values
    int encoderValue = readEncoder(); // Implement this function to read the encoder value

    // Navigate menu
    if (encoderValue > 0) {
        currentMenuItem = (MenuItem)((currentMenuItem + 1) % MENU_SIZE);
    } else if (encoderValue < 0) {
        currentMenuItem = (MenuItem)((currentMenuItem - 1 + MENU_SIZE) % MENU_SIZE);
    }

    // Modify values
    if (isEncoderButtonPressed()) { // Implement this function to check if the encoder button is pressed
        switch (currentMenuItem) {
            case ROOT_NOTE:
                rootNote = (rootNote + 1) % 12; // Example range for root note
                break;
            case SEQ_LENGTH:
                seqLength = (seqLength % 16) + 1; // Example range for sequence length
                break;
            case DIV:
                div = (div % 4) + 1; // Example range for division
                break;
            case PATTERN:
                // Modify pattern values (example logic)
                for (int i = 0; i < 4; i++) {
                    pattern[i] = (pattern[i] % 4) + 1;
                }
                break;
            default:
                break;
        }
    }
}

// Function to update OLED display
void updateOLED() {
    // Clear display
    oled.clear();

    // Display current menu item and value
    switch (currentMenuItem) {
        case ROOT_NOTE:
            oled.print("Root Note: ");
            oled.print(rootNote);
            break;
        case SEQ_LENGTH:
            oled.print("Seq Length: ");
            oled.print(seqLength);
            break;
        case DIV:
            oled.print("Div: ");
            oled.print(div);
            break;
        case PATTERN:
            oled.print("Pattern: ");
            for (int i = 0; i < 4; i++) {
                oled.print(pattern[i]);
                oled.print(" ");
            }
            break;
        default:
            break;
    }

    // Display other information
    // ...existing code...
}

// In the main loop, call the handleEncoderInput and updateOLED functions
void loop() {
    // ...existing code...

    handleEncoderInput();
    updateOLED();

    // ...existing code...
}
