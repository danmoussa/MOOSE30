#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "config.h"

using namespace DisplayConfig;

HardwareSerial classicLink(1);
SPIClass lcdSpi(FSPI);
Adafruit_ST7789 display(&lcdSpi, kLcdCsPin, kLcdDcPin, kLcdResetPin);

char artist[kFieldLength] = "";
char title[kFieldLength] = "";
bool connected = false;
bool playing = false;
uint32_t lastStateRequestMs = 0;
uint32_t lastHeartbeatMs = 0;
uint32_t receivedPackets = 0;
uint32_t malformedPackets = 0;

void copyField(char* destination, size_t destinationSize, const String& source) {
  if (destination == nullptr || destinationSize == 0) return;
  source.substring(0, destinationSize - 1).toCharArray(destination, destinationSize);
}

String fitText(const char* source, int maxWidth, uint8_t textSize) {
  String value = source == nullptr ? String() : String(source);
  const int charWidth = 6 * textSize;
  const int maxChars = maxWidth / charWidth;
  if (static_cast<int>(value.length()) <= maxChars) return value;
  if (maxChars <= 3) return value.substring(0, maxChars);
  return value.substring(0, maxChars - 3) + "...";
}

void drawCentered(const String& text, int centerX, int baselineY, uint8_t textSize, uint16_t color) {
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextSize(textSize);
  display.setTextColor(color, ST77XX_BLACK);
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(centerX - static_cast<int>(w) / 2, baselineY - static_cast<int>(h) / 2);
  display.print(text);
}

void drawScreen() {
  display.fillScreen(ST77XX_BLACK);
  display.setTextWrap(false);

  if (!connected) {
    drawCentered("MOOSE30", kScreenWidth / 2, 68, 3, ST77XX_WHITE);
    drawCentered("Waiting for iPhone", kScreenWidth / 2, 110, 1, 0x8410);
    Serial.println("[DISPLAY] rendered waiting screen");
    return;
  }

  const String shownArtist = fitText(artist[0] ? artist : "Unknown Artist", 238, 2);
  const String shownTitle = fitText(title[0] ? title : (playing ? "Playing" : "Paused"), 250, 2);

  display.setTextSize(2);
  display.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  display.setCursor(kEdgeMargin, 76);
  display.print("<");

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(">", 0, 0, &x1, &y1, &w, &h);
  display.setCursor(kScreenWidth - kEdgeMargin - w, 76);
  display.print(">");

  drawCentered(shownArtist, kScreenWidth / 2, 59, 2, ST77XX_WHITE);
  drawCentered(shownTitle, kScreenWidth / 2, 108, 2, 0xC618);
  Serial.printf("[DISPLAY] rendered connected=%u playing=%u artist=\"%s\" title=\"%s\"\n",
                connected ? 1U : 0U, playing ? 1U : 0U, artist, title);
}

bool parseMetadata(const String& line) {
  if (!line.startsWith("META|")) return false;

  const int connectedSeparator = line.indexOf('|', 5);
  const int secondSeparator = connectedSeparator < 0 ? -1 : line.indexOf('|', connectedSeparator + 1);
  const int thirdSeparator = secondSeparator < 0 ? -1 : line.indexOf('|', secondSeparator + 1);
  const int fourthSeparator = thirdSeparator < 0 ? -1 : line.indexOf('|', thirdSeparator + 1);
  if (connectedSeparator < 0 || secondSeparator < 0 || thirdSeparator < 0) return false;

  connected = line.substring(5, connectedSeparator).toInt() == 1;

  // Accept both formats:
  // 1) META|connected|playing|artist|title
  // 2) META|connected|artist|title
  if (fourthSeparator >= 0) {
    playing = line.substring(connectedSeparator + 1, secondSeparator).toInt() == 1;
    copyField(artist, sizeof(artist), line.substring(secondSeparator + 1, thirdSeparator));
    copyField(title, sizeof(title), line.substring(thirdSeparator + 1));
  } else {
    copyField(artist, sizeof(artist), line.substring(secondSeparator + 1, thirdSeparator));
    copyField(title, sizeof(title), line.substring(thirdSeparator + 1));
    if (!connected) playing = false;
  }
  ++receivedPackets;

Serial.printf(
    "[UART][RX] packet=%lu connected=%u playing=%u artist=\"%s\" title=\"%s\"\n",
    static_cast<unsigned long>(millis()),
    static_cast<unsigned int>(connected),
    static_cast<unsigned int>(playing),
    artist,
    title
);
  drawScreen();
  return true;
}

void readLink() {
  static String line;
  line.reserve(kLineLength);

  while (classicLink.available() > 0) {
    const char c = static_cast<char>(classicLink.read());
    if (c == '\n') {
      line.trim();
      if (line.length() > 0 && !parseMetadata(line)) {
        ++malformedPackets;
        Serial.printf("[UART][WARN] malformed packet=%lu text=\"%s\"\n",
                      static_cast<unsigned long>(malformedPackets), line.c_str());
      }
      line = "";
    } else if (c != '\r') {
      if (line.length() < kLineLength - 1) line += c;
      else {
        ++malformedPackets;
        line = "";
        Serial.println("[UART][WARN] receive buffer overflow; line discarded");
      }
    }
  }
}

void setBacklightPercent(int percent) {
  percent = constrain(percent, 0, 100);
  const uint8_t duty = static_cast<uint8_t>(map(percent, 0, 100, 0, 255));
  analogWrite(kBacklightPin, duty);
  Serial.printf("[LCD] backlight=%d%% duty=%u\n", percent, duty);
}

void requestState(const char* reason) {
  classicLink.println("STATE");
  Serial.printf("[UART][TX] STATE reason=%s\n", reason);
}

void processUsbCommands() {
  static String command;
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n') {
      command.trim();
      command.toUpperCase();
      if (command == "STATE") requestState("usb");
      else if (command == "NEXT" || command == "PREV") {
        classicLink.println(command);
        Serial.printf("[UART][TX] %s reason=usb\n", command.c_str());
      } else if (command == "TEST") {
        connected = true;
        playing = true;
        copyField(artist, sizeof(artist), "MOOSE30 TEST ARTIST");
        copyField(title, sizeof(title), "Display Test Song");
        drawScreen();
      } else if (command == "CLEAR") {
        connected = false;
        playing = false;
        artist[0] = '\0';
        title[0] = '\0';
        drawScreen();
      } else if (command.length() > 0) {
        Serial.println("[HELP] Commands: STATE, NEXT, PREV, TEST, CLEAR");
      }
      command = "";
    } else if (c != '\r' && command.length() < 32) command += c;
  }
}

void printHeartbeat() {
  const uint32_t now = millis();
  if (now - lastHeartbeatMs < kHeartbeatIntervalMs) return;
  lastHeartbeatMs = now;
  Serial.printf("[STATUS] uptime=%lus connected=%u playing=%u packets=%lu malformed=%lu free_heap=%u\n",
                static_cast<unsigned long>(now / 1000), connected ? 1U : 0U,
                playing ? 1U : 0U, static_cast<unsigned long>(receivedPackets),
                static_cast<unsigned long>(malformedPackets), ESP.getFreeHeap());
}

void setup() {
  Serial.begin(kSerialBaud);
  delay(800);
  Serial.println();
  Serial.println("[BOOT] MOOSE30 Waveshare ESP32-C6 display starting");
  Serial.printf("[BOOT] build=%s %s\n", __DATE__, __TIME__);
  Serial.printf("[BOOT] UART1 RX=%d TX=%d baud=%lu\n", kLinkRxPin, kLinkTxPin,
                static_cast<unsigned long>(kLinkBaud));
  Serial.printf("[BOOT] LCD MOSI=%d SCLK=%d CS=%d DC=%d RST=%d BL=%d\n",
                kLcdMosiPin, kLcdSclkPin, kLcdCsPin, kLcdDcPin, kLcdResetPin, kBacklightPin);

  classicLink.begin(kLinkBaud, SERIAL_8N1, kLinkRxPin, kLinkTxPin);

  pinMode(kBacklightPin, OUTPUT);
  setBacklightPercent(kBacklightPercent);

  Serial.println("[LCD] initializing SPI and ST7789");
  lcdSpi.begin(kLcdSclkPin, -1, kLcdMosiPin, kLcdCsPin);
  display.init(kNativeWidth, kNativeHeight, SPI_MODE0);
  display.setRotation(1);
  display.fillScreen(ST77XX_BLACK);
  Serial.printf("[LCD] ready width=%d height=%d\n", display.width(), display.height());
  if (display.width() != kScreenWidth || display.height() != kScreenHeight) {
    Serial.printf("[LCD][WARN] expected %dx%d after rotation\n", kScreenWidth, kScreenHeight);
  }

  drawScreen();
  requestState("boot");
  lastStateRequestMs = millis();
  Serial.println("[READY] MOOSE30 display controller ready");
  Serial.println("[HELP] Type TEST to draw sample metadata; CLEAR restores waiting screen");
}

void loop() {
  readLink();
  processUsbCommands();

  const uint32_t now = millis();
  if (now - lastStateRequestMs >= kStateRequestIntervalMs) {
    requestState("periodic");
    lastStateRequestMs = now;
  }

  printHeartbeat();
  delay(2);
}
