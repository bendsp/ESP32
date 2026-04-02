# Matrix Resources

This folder holds vendor resources and notes for the HUB75 RGB LED matrix setup.

## Hardware In Use

- ESP32-S3-DevKitC-1
- Seengreat RGB Matrix Adapter Board (E)
- HUB75 64x64 RGB panel

## Vendor Resources

- Seengreat adapter wiki: https://seengreat.com/wiki/186/rgb-matrix-adapter-board-e
- Vendor demo sketch archive: `matrix/vendor/ESP32S3/PatternPlasma/PatternPlasma.ino`
- HUB75 DMA library archive: `libraries/ESP32-HUB75-MatrixPanel-DMA-master/`

## Known ESP32-S3 Pin Mapping For This Adapter

- `R1 -> IO37`
- `G1 -> IO6`
- `B1 -> IO36`
- `R2 -> IO35`
- `G2 -> IO5`
- `B2 -> IO0`
- `A -> IO45`
- `B -> IO1`
- `C -> IO48`
- `D -> IO2`
- `E -> IO4`
- `CLK -> IO47`
- `LAT -> IO38`
- `OE -> IO21`

## First Test Strategy

- use external `5V` power on the adapter board
- keep ESP32 connected to the Mac for upload and serial logs
- do not unplug or replug matrix wiring while powered
- start with low brightness before trying the stock demo at full intensity
