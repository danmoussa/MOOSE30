# Install summary

1. Extract the v5 ZIP.
2. Open Terminal in the extracted folder.
3. Run `./build_all.sh`.
4. Connect only the classic ESP32 and run `./upload_classic.sh`.
5. Run `./monitor_classic.sh` and test iPhone pairing/audio.
6. Exit with `Ctrl+C`.
7. Connect only the Waveshare ESP32-C6 and run `./upload_display.sh`.
8. Run `./monitor_display.sh`; type `TEST` and press Enter.
9. Exit with `Ctrl+C`, disconnect power, and wire the boards using `docs/WIRING.md`.

Detailed expected logs are in `TESTING.md`.
