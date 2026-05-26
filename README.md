# PICO-Diginterface

An RP2040-based digital interface for amateur radio. Enumerates as a UAC2 sound card and CDC virtual COM port, providing microphone input, PDM speaker output, dual PTT channels, and CAT control passthrough over a single USB connection.

Inspired by [AIOC](https://github.com/skuep/AIOC).

---

## Hardware

The PCB implements full galvanic isolation between the host interface and the radio to eliminate ground loops and RF interference, using PC817 optocouplers and 1:1 600 ohms isolation transformers.

### Pinout

| GPIO | Function       | Direction    | Notes                                                |
|------|----------------|--------------|------------------------------------------------------|
| 4    | UART1 TX       | Output       | Inverted, 4800 baud                                  |
| 5    | UART1 RX       | Input        | Inverted, pull-up enabled                            |
| 12   | PTT Status LED | Output       | Active high; asserted when any PTT channel is active |
| 18   | PDM Data Out   | Output (PIO) | PDM bitstream at 1.536 MHz                           |
| 22   | PTT Channel 1  | Output       | Active high                                          |
| 24   | PTT Channel 2  | Output       | Active high                                          |
| 25   | Board LED      | Output       | Blinks USB status                                    |
| 26   | Mic Input      | Analog input | ADC channel 0, 12-bit, 48 kHz                        |

---

## Firmware

### Overview

When connected to a host PC, the firmware presents three USB interfaces:

- **UAC2 Microphone** — 48 kHz, 16-bit mono audio captured via the on-board 12-bit ADC
- **UAC2 Speaker** — 48 kHz, 16-bit mono audio from the host, output as a PDM bitstream via PIO
- **CDC Serial** — Virtual COM port bridged to UART1 for CAT rig control and radio programming

### Features

- **USB Audio (UAC2)** — Full-duplex USB Audio Class 2 interface. Microphone path: 48 kHz, 12-bit ADC, transmitted as 16-bit PCM. Speaker path: 48 kHz / 16-bit PCM from host, output as a 1.536 MHz PDM bitstream via PIO.
- **Push-to-Talk** — Two independent PTT output channels, keyed automatically when the received audio level exceeds a configurable threshold. A hold-off timer keeps the channel open for a defined period after the signal drops.
- **CAT Control Bridge** — Bidirectional CDC serial passthrough to UART1 (inverted, 4800 baud) for rig control and radio programming.

### LED Status

| Blink Interval | State                   |
|----------------|-------------------------|
| Solid on       | Audio streaming         |
| 250 ms         | Unmounted               |
| 1000 ms        | Mounted, idle           |
| 2500 ms        | Not mounted / suspended |

### Building

#### Prerequisites

- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) v1.5.1 or later
- CMake 3.19+
- ARM GCC toolchain

#### Compiling

```bash
mkdir build && cd build
cmake .. -DPICO_SDK_PATH=/path/to/pico-sdk
make -j$(nproc)
```

Flash the resulting `.uf2` file by holding BOOTSEL while connecting the board, then copying the file to the mass storage device that appears.



## Enjoy the project and 73 de PP2GS!