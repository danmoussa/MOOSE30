# MOOSE30 Bluetooth Radio v5

Two PlatformIO firmware projects:

- `classic_esp32`: Bluetooth Classic A2DP/AVRCP receiver, PCM5102A-style I2S output, physical track controls, UART metadata link, and detailed serial diagnostics.
- `waveshare_c6_display`: Waveshare ESP32-C6-LCD-1.47 display, UART metadata receiver, built-in display test mode, and detailed serial diagnostics.

## Important corrections in v5

- Classic ESP32 uses `huge_app.csv`, so the A2DP firmware fits its 4 MB flash.
- ESP32-C6 uses the PIOArduino Arduino-core-3.x PlatformIO platform. The official `espressif32@6.10.0` package used in v4 could not build Arduino for ESP32-C6.
- The display uses portable Adafruit GFX/ST7789 libraries rather than TFT_eSPI, avoiding known ESP32-C6 compatibility problems.
- Both boards print boot, status, UART, Bluetooth, metadata, playback, control and error logs.

Start with `INSTALL_MAC.md`, then see `TESTING.md` and `docs/WIRING.md`.
