# Testing and expected logs

## 1. Classic ESP32 standalone test

Upload and monitor:

```bash
./upload_classic.sh
./monitor_classic.sh
```

Expected boot logs include:

```text
[BOOT] MOOSE30 classic ESP32 starting
[I2S] ready
[BT] starting A2DP sink name=MOOSE30
[READY] MOOSE30 audio controller ready
```

Pair the iPhone with `MOOSE30`. Expected connection logs:

```text
[BT][A2DP] connection=CONNECTING
[BT][A2DP] connection=CONNECTED
[BT][AVRCP] connection=CONNECTED
```

Play music. Expected logs:

```text
[BT][AUDIO] state=STARTED
[BT][META] artist="..."
[BT][META] title="..."
[UART][TX] connected=1 playing=1 ...
```

You can type these commands into the classic monitor and press Enter:

- `STATE`: print/send current state
- `NEXT`: request next track
- `PREV`: request previous track

## 2. Display standalone test

Upload and monitor the C6, then type `TEST` and press Enter. The LCD should show:

```text
<       MOOSE30 TEST ARTIST       >
          Display Test Song
```

The monitor should print:

```text
[DISPLAY] rendered connected=1 playing=1 ...
```

Type `CLEAR` to return to the waiting screen.

## 3. Board-to-board UART test

Disconnect USB power before wiring. Wire GPIO17 TX to GPIO18 RX, GPIO16 RX to GPIO19 TX, and GND to GND. Power both boards.

The C6 should periodically print:

```text
[UART][TX] STATE reason=periodic
```

The classic should print:

```text
[UART][RX] command=STATE
[UART][TX] connected=... playing=... artist="..." title="..."
```

The C6 should then print:

```text
[UART][RX] packet=... connected=... playing=... artist="..." title="..."
[DISPLAY] rendered ...
```

## 4. Audio test

Connect the PCM5102A-style DAC to the classic ESP32 according to `docs/WIRING.md`. Connect DAC L/R/GND to a line-level amplifier or radio AUX input. Do not drive passive speakers directly from the DAC.
