# Wiring

## Board-to-board UART

| Classic ESP32 | Waveshare ESP32-C6 |
|---|---|
| GPIO17 TX | GPIO18 RX |
| GPIO16 RX | GPIO19 TX |
| GND | GND |

TX crosses to RX. Both boards use 3.3 V logic.

## Classic ESP32 to PCM5102A-style I2S DAC

| Classic ESP32 | DAC |
|---|---|
| GPIO26 | BCK |
| GPIO25 | LCK / LRCK / WS |
| GPIO22 | DIN |
| GND | GND |

The DAC output is line level. Connect DAC L, R and GND to the radio's line/tape input, not directly to passive speakers.

## Track buttons

| Function | Wiring |
|---|---|
| Previous | GPIO32 -> momentary button -> GND |
| Next | GPIO33 -> momentary button -> GND |

The `<` and `>` shown on the screen are labels. This Waveshare screen is not touch-sensitive.

## Power

For bench testing, power each ESP32 over USB and join all grounds. For vehicle installation, use an automotive-safe regulated supply. Verify the buck-converter output with a multimeter before connecting either board.
