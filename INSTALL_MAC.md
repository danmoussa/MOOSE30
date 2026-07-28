# macOS build, upload and monitor

From the extracted `MOOSE30_Bluetooth_Radio_v5` folder:

```bash
./build_all.sh
```

The first C6 build downloads the PIOArduino ESP32 platform and may take longer than later builds.

## Classic ESP32

Connect only the classic ESP32:

```bash
./upload_classic.sh
./monitor_classic.sh
```

Expected final startup line:

```text
[READY] MOOSE30 audio controller ready
```

Exit the monitor with `Ctrl+C`.

## Waveshare ESP32-C6

Disconnect the classic board, connect only the Waveshare board:

```bash
./upload_display.sh
./monitor_display.sh
```

Expected final startup line:

```text
[READY] MOOSE30 display controller ready
```

Exit the monitor with `Ctrl+C`.
