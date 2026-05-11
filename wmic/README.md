# Wireless Mic Firmware

This repository contains the firmware for an nRF5340-based wireless microphone running Zephyr RTOS.

## Project structure

- `wmic/` — Zephyr application for the wireless microphone.
- `modules/CMSIS_DSP/` — CMSIS-DSP library imported as a git submodule.
- `res/` — hardware reference and peripheral documentation.
- `scripts/` — supporting DSP and I2S helper scripts.

## Prerequisites

This project is built with Zephyr RTOS. You must install a compatible Zephyr development environment and an ARM toolchain.

Recommended prerequisites:

- Zephyr SDK or nRF Connect SDK with Zephyr support
- `arm-none-eabi-gcc` (GNU Arm Embedded Toolchain)
- `cmake` 3.20 or newer
- `ninja`
- Python 3.8+ with Zephyr Python requirements
- `git`

Optional for flashing/debugging:

- `nrfjprog` / `nrfutil`
- SEGGER J-Link tools

## Required environment variables

Set `ZEPHYR_BASE` to the Zephyr source tree location. Example on Windows PowerShell:

```powershell
$env:ZEPHYR_BASE = 'C:\path\to\zephyr'
```

Or on Linux/macOS:

```bash
export ZEPHYR_BASE=/path/to/zephyr
```

## Clone the repository

Clone the repo with submodules to ensure CMSIS-DSP is available:

```bash
git clone --recurse-submodules <repo-url>
```

If you already cloned without submodules, initialize them:

```bash
git submodule update --init --recursive
```

## Build the project

From the repository root, create a build directory and configure the Zephyr application:

```bash
cmake -B wmic/build -S wmic -DBOARD=nrf5340dk_nrf5340_cpuapp
cmake --build wmic/build
```

If you use Ninja directly:

```bash
cmake -B wmic/build -S wmic -DBOARD=nrf5340dk_nrf5340_cpuapp
ninja -C wmic/build
```

## Flashing

After a successful build, flash the firmware to the board using the generated `.hex` or `.bin` file.

Example with `nrfjprog`:

```bash
nrfjprog --eraseall
nrfjprog --program wmic/build/zephyr/zephyr.hex --verify --reset
```

> Note: `nrfjprog` must be installed and available on `PATH` (Nordic Command Line Tools).

Or use the Zephyr `west` wrapper if available and force the board-appropriate runner:

```bash
west flash --build-dir wmic/build --runner nrfjprog
```

## Troubleshooting

- If `arm_math.h` is missing, the `modules/CMSIS_DSP` submodule is not initialized.
  Run `git submodule update --init --recursive`.
- If CMake cannot find Zephyr, verify `ZEPHYR_BASE` is set and points to a valid Zephyr repository.
- For board selection issues, confirm the build board matches your hardware.

## Notes

- `wmic/CMakeLists.txt` uses `find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})`.
- `wmic/prj.conf` contains Zephyr configuration for the wireless microphone application.
- `modules/CMSIS_DSP` is required for DSP functions such as `arm_fir_q15` and `arm_math.h`.
