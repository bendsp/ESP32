# Arduino Sketches

This folder holds Arduino example sketches for the ESP32-S3 board in this repo.

## How Arduino Sketches Work

Each sketch lives in its own folder, and the main `.ino` file must match the folder name.

Examples:

- `hello_serial/hello_serial.ino`
- `blink_builtin/blink_builtin.ino`

That is why Arduino creates folders like `sketch_apr2a` by default. They are just temporary sketch names.

## Current Sketches

- `hello_serial`: first upload test, prints to the serial monitor once per second
- `blink_builtin`: blinks the board's built-in LED and prints LED state to serial

## Recommended Workflow

- keep one folder per experiment
- use descriptive names instead of date-based names
- when a sketch teaches something new, keep it in this repo rather than overwriting it

## Opening These In Arduino IDE

In Arduino IDE, use `File > Open...` and open the `.ino` file you want.
