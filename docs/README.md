# ESP32 Docs

This folder holds the vendor PDFs and working notes for the board in this repo.

## Board We Appear To Have

Amazon listing text and the board silkscreen suggest this is a clone of the `ESP32-S3-DevKitC-1` built around an `ESP32-S3-WROOM-1-N16R8` module.

- `N16` means 16 MB flash
- `R8` means 8 MB PSRAM

That matters because the official Espressif documentation is split across:

- the `ESP32-S3` chip itself
- the `ESP32-S3-WROOM-1` module
- the `ESP32-S3-DevKitC-1` development board

## Saved PDFs

- `esp32-s3_datasheet_en.pdf`
- `esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf`
- `SCH_ESP32-S3-DevKitC-1_V1.1_20221130.pdf`
- `esp-dev-kits-en-master-esp32s3.pdf`

## What Each One Is For

- `esp32-s3_datasheet_en.pdf`: chip capabilities, pins, power, peripherals
- `esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf`: the exact module family on the board
- `SCH_ESP32-S3-DevKitC-1_V1.1_20221130.pdf`: schematic for the dev board wiring
- `esp-dev-kits-en-master-esp32s3.pdf`: board user guide collection for the S3 dev kits

## First Connection Notes

Before plugging anything into headers:

- power it only over USB
- do not connect `5V` or `3V3` pins to anything yet
- do not jumper pins together yet
- prefer a known good data-capable USB-C cable

On this Mac, `pyserial` and `esptool` were not installed when this repo was initialized, so first-talk setup will include installing them.
