# Hagiwo SH-101 Alternative Firmware

This is an alternative firmware for the Hagiwo SH-101 synthesizer module, featuring the implementation of the circle of fifths for both major and minor scales. The firmware allows users to select root notes, sequence length, patterns, and divisions, and generate sequences based on the circle of fifths. It retains the handling of noisy encoders to prevent false clicks.

## Features

- **Circle of Fifths**: Supports all root notes in both major and minor scales.
- **Sequence Length**: Options for 4, 8, and 16 bars.
- **Pattern Selection**: Customizable patterns for sequence generation.
- **Division (DIV) Options**: Select from various division rates.
- **Manual Reset**: A 'RESET' menu item to manually reset the sequence playback to the beginning.
- **Noise Handling**: Improved handling of noisy encoders to prevent false clicks.

## Installation

1. Download the firmware_circle_seq.ino file.
2. Open the file in the Arduino IDE.
3. Connect your Hagiwo SH-101 module to your computer.
4. Select the correct board and port in the Arduino IDE.
5. Upload the firmware to the module.

## Usage

### Menu Navigation

- Use the rotary encoder to navigate through the menu options.
- Press the button to select and update settings.

### Menu Options

1. **Root Note**: Select the root note for the sequence (12 major and 12 minor notes).
2. **Sequence Length**: Choose between 4, 8, and 16 bars.
3. **Division (DIV)**: Select the division rate from the available options.
4. **Pattern**: Toggle between different patterns for sequence generation.
5. **RESET**: Manually reset the sequence playback to the beginning.

### Sequence Generation

- The sequence is generated based on the selected root note, pattern, and circle of fifths.
- The CV and trigger outputs are synchronized with the clock input and division settings.
- The sequence loops when it reaches the end.

## Contributing

Feel free to contribute to this project by submitting issues or pull requests. For major changes, please open an issue first to discuss what you would like to change.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.