# ICTE4005 conveyor project

Three independent PlatformIO projects, one per Arduino Uno (classic ATmega328P / R3).

```text
master/      Orchestration, target/current status display, optional buzzer
conveyor/    Five-wire stepper and existing g/s/f/l motor controls
sensing/     Reed switches, magnetic-puck detection and status LEDs
```

Each folder contains its own `platformio.ini` and `src/main.cpp`. Open an individual
folder in VS Code with PlatformIO, or open `icte4005.code-workspace` for all three.

## Current state

These are minimal firmware scaffolds only. They print the board role at 9600 baud and do
not drive the motor, initialise the display, read reeds, or implement I2C commands.
Do not upload them as replacements for working demonstration firmware yet.

The existing working sketches and their libraries will be brought in first;
the small shared command/state interface will then be aligned to that behaviour.
The earlier complex contract from the Barbot planning folder is not implemented here.

## Build and monitor

From this repository root:

```sh
pio run -d master
pio run -d conveyor
pio run -d sensing
```

For a connected board, explicitly select its port (especially with several Unos):

```sh
pio device list
pio run -d conveyor -t upload --upload-port /dev/cu.YOUR_BOARD
pio device monitor -d conveyor --port /dev/cu.YOUR_BOARD --baud 9600
```

Replace `conveyor` with the relevant project and `YOUR_BOARD` with its actual port.
No upload port is hard-coded. PlatformIO's AVR platform is pinned to 5.0.0 in all
three projects; the framework is Arduino and the board ID is `uno`.

### Local validation (11 October 2026)

All three PlatformIO configurations parse successfully. Compilation was attempted
but blocked by the installed x86_64 AVR compiler failing to execute on the arm64
Mac (`Bad CPU type in executable`). A successful firmware build has not yet been
verified. Resolve the local compiler compatibility before uploading; no system
toolchain changes or hardware uploads were performed during scaffolding.

## Importing the working sketches

1. Replace the relevant `src/main.cpp` scaffold with the working sketch, adding
   `#include <Arduino.h>` and forward declarations where needed for C++ compilation.
2. Keep only one `setup()` and one `loop()` per project. Alternatively use a single
   `.ino` file under `src/` and remove that project's scaffold `main.cpp`.
3. Add the sketch's actual external libraries under `lib_deps` in its own
   `platformio.ini`. Built-in Arduino libraries such as Wire need no registry dependency.
4. Preserve proven pins, baud rate and motor behaviour before adding I2C integration.
