#include <Arduino.h>
#include <cctype>
#include <cstring>
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"
#include "esp_avrc_api.h"
#include "config.h"
#include "link_protocol.h"

using namespace Moose30Config;

I2SStream audioOutput;
BluetoothA2DPSink bluetoothSink(audioOutput);
HardwareSerial displayLink(2);

portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;
char artist[kMetadataLength] = "";
char title[kMetadataLength] = "";
volatile bool connected = false;
volatile bool playing = false;
volatile bool stateDirty = true;
uint32_t lastHeartbeatMs = 0;

const char* connectionStateName(esp_a2d_connection_state_t state) {
  switch (state) {
    case ESP_A2D_CONNECTION_STATE_DISCONNECTED: return "DISCONNECTED";
    case ESP_A2D_CONNECTION_STATE_CONNECTING: return "CONNECTING";
    case ESP_A2D_CONNECTION_STATE_CONNECTED: return "CONNECTED";
    case ESP_A2D_CONNECTION_STATE_DISCONNECTING: return "DISCONNECTING";
    default: return "UNKNOWN";
  }
}

const char* audioStateName(esp_a2d_audio_state_t state) {
  switch (state) {
    case ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND: return "REMOTE_SUSPEND";
    case ESP_A2D_AUDIO_STATE_STOPPED: return "STOPPED";
    case ESP_A2D_AUDIO_STATE_STARTED: return "STARTED";
    default: return "UNKNOWN";
  }
}

void markStateDirty() {
  portENTER_CRITICAL(&stateMux);
  stateDirty = true;
  portEXIT_CRITICAL(&stateMux);
}

void metadataCallback(uint8_t attributeId, const uint8_t* value) {
  const char* text = value == nullptr ? "" : reinterpret_cast<const char*>(value);
  if (attributeId == ESP_AVRC_MD_ATTR_ARTIST) {
    portENTER_CRITICAL(&stateMux);
    Moose30Protocol::sanitizeCopy(artist, sizeof(artist), value);
    stateDirty = true;
    portEXIT_CRITICAL(&stateMux);
    Serial.printf("[BT][META] artist=\"%s\"\n", text);
  } else if (attributeId == ESP_AVRC_MD_ATTR_TITLE) {
    portENTER_CRITICAL(&stateMux);
    Moose30Protocol::sanitizeCopy(title, sizeof(title), value);
    stateDirty = true;
    portEXIT_CRITICAL(&stateMux);
    Serial.printf("[BT][META] title=\"%s\"\n", text);
  } else {
    Serial.printf("[BT][META] attribute=%u value=\"%s\"\n", attributeId, text);
  }
}

void connectionCallback(esp_a2d_connection_state_t state, void*) {
  const bool nowConnected = state == ESP_A2D_CONNECTION_STATE_CONNECTED;
  portENTER_CRITICAL(&stateMux);
  connected = nowConnected;
  if (!nowConnected) {
    playing = false;
    artist[0] = '\0';
    title[0] = '\0';
  }
  stateDirty = true;
  portEXIT_CRITICAL(&stateMux);
  Serial.printf("[BT][A2DP] connection=%s\n", connectionStateName(state));
}

void audioStateCallback(esp_a2d_audio_state_t state, void*) {
  portENTER_CRITICAL(&stateMux);
  playing = state == ESP_A2D_AUDIO_STATE_STARTED;
  stateDirty = true;
  portEXIT_CRITICAL(&stateMux);
  Serial.printf("[BT][AUDIO] state=%s\n", audioStateName(state));
}

void avrcConnectionCallback(bool isConnected) {
  Serial.printf("[BT][AVRCP] connection=%s\n", isConnected ? "CONNECTED" : "DISCONNECTED");
}

void volumeCallback(int volume) {
  Serial.printf("[BT][VOLUME] value=%d/127\n", volume);
}

void sendStateIfNeeded() {
  bool shouldSend = false;
  bool connectedCopy = false;
  bool playingCopy = false;
  char artistCopy[kMetadataLength] = {};
  char titleCopy[kMetadataLength] = {};

  portENTER_CRITICAL(&stateMux);
  if (stateDirty) {
    shouldSend = true;
    stateDirty = false;
    connectedCopy = connected;
    playingCopy = playing;
    std::memcpy(artistCopy, artist, sizeof(artistCopy));
    std::memcpy(titleCopy, title, sizeof(titleCopy));
  }
  portEXIT_CRITICAL(&stateMux);

  if (!shouldSend) return;
  artistCopy[sizeof(artistCopy) - 1] = '\0';
  titleCopy[sizeof(titleCopy) - 1] = '\0';
  displayLink.printf("META|%u|%u|%s|%s\n",
                     connectedCopy ? 1U : 0U,
                     playingCopy ? 1U : 0U,
                     artistCopy,
                     titleCopy);
  Serial.printf("[UART][TX] connected=%u playing=%u artist=\"%s\" title=\"%s\"\n",
                connectedCopy ? 1U : 0U,
                playingCopy ? 1U : 0U,
                artistCopy,
                titleCopy);
}

void executeCommand(const char* command, const char* source) {
  Serial.printf("[%s][RX] command=%s\n", source, command);
  if (std::strcmp(command, "PREV") == 0) {
    Serial.println("[CONTROL] previous track");
    bluetoothSink.previous();
  } else if (std::strcmp(command, "NEXT") == 0) {
    Serial.println("[CONTROL] next track");
    bluetoothSink.next();
  } else if (std::strcmp(command, "STATE") == 0) {
    markStateDirty();
  } else if (std::strcmp(command, "STATUS") == 0) {
    markStateDirty();
  } else {
    Serial.printf("[%s][WARN] unknown command=%s\n", source, command);
  }
}

void normalizeCommand(char* command) {
  if (command == nullptr) return;

  size_t start = 0;
  size_t end = std::strlen(command);

  while (command[start] != '\0' && std::isspace(static_cast<unsigned char>(command[start]))) {
    ++start;
  }
  while (end > start && std::isspace(static_cast<unsigned char>(command[end - 1]))) {
    --end;
  }

  size_t writeIndex = 0;
  for (size_t readIndex = start; readIndex < end && writeIndex < kCommandLength - 1; ++readIndex) {
    command[writeIndex++] = static_cast<char>(std::toupper(static_cast<unsigned char>(command[readIndex])));
  }
  command[writeIndex] = '\0';
}

void processDisplayCommands() {
  static char command[kCommandLength];
  static size_t length = 0;

  while (displayLink.available() > 0) {
    const char c = static_cast<char>(displayLink.read());
    if (c == '\n') {
      command[length] = '\0';
      normalizeCommand(command);
      if (command[0] != '\0') executeCommand(command, "UART");
      length = 0;
    } else if (c != '\r') {
      if (length < sizeof(command) - 1) command[length++] = c;
      else {
        length = 0;
        Serial.println("[UART][WARN] incoming command exceeded buffer");
      }
    }
  }
}

void processUsbCommands() {
  static char command[kCommandLength];
  static size_t length = 0;

  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n') {
      command[length] = '\0';
      normalizeCommand(command);
      if (command[0] != '\0') executeCommand(command, "USB");
      length = 0;
    } else if (c != '\r') {
      if (length < sizeof(command) - 1) command[length++] = c;
      else length = 0;
    }
  }
}

void processButtons() {
  static bool previousWasReleased = true;
  static bool nextWasReleased = true;
  static uint32_t lastPressMs = 0;

  const bool previousReleased = digitalRead(kPreviousButtonPin) == HIGH;
  const bool nextReleased = digitalRead(kNextButtonPin) == HIGH;
  const uint32_t now = millis();

  if (now - lastPressMs >= kButtonDebounceMs) {
    if (previousWasReleased && !previousReleased) {
      Serial.println("[BUTTON] PREV pressed");
      bluetoothSink.previous();
      lastPressMs = now;
    } else if (nextWasReleased && !nextReleased) {
      Serial.println("[BUTTON] NEXT pressed");
      bluetoothSink.next();
      lastPressMs = now;
    }
  }

  previousWasReleased = previousReleased;
  nextWasReleased = nextReleased;
}

void printHeartbeat() {
  const uint32_t now = millis();
  if (now - lastHeartbeatMs < kHeartbeatIntervalMs) return;
  lastHeartbeatMs = now;

  bool connectedCopy;
  bool playingCopy;
  portENTER_CRITICAL(&stateMux);
  connectedCopy = connected;
  playingCopy = playing;
  portEXIT_CRITICAL(&stateMux);

  Serial.printf("[STATUS] uptime=%lus connected=%u playing=%u free_heap=%u\n",
                static_cast<unsigned long>(now / 1000),
                connectedCopy ? 1U : 0U,
                playingCopy ? 1U : 0U,
                ESP.getFreeHeap());
}

void setup() {
  Serial.begin(kSerialBaud);
  delay(400);
  Serial.println();
  Serial.println("[BOOT] MOOSE30 classic ESP32 starting");
  Serial.printf("[BOOT] build=%s %s\n", __DATE__, __TIME__);
  Serial.printf("[BOOT] UART2 RX=%d TX=%d baud=%lu\n", kDisplayRxPin, kDisplayTxPin,
                static_cast<unsigned long>(kLinkBaud));
  Serial.printf("[BOOT] I2S BCLK=%d LRCK=%d DATA=%d\n", kI2sBclkPin, kI2sLrckPin, kI2sDataPin);

  displayLink.begin(kLinkBaud, SERIAL_8N1, kDisplayRxPin, kDisplayTxPin);
  pinMode(kPreviousButtonPin, INPUT_PULLUP);
  pinMode(kNextButtonPin, INPUT_PULLUP);

  Serial.println("[I2S] initializing output");
  auto i2sConfig = audioOutput.defaultConfig(TX_MODE);
  i2sConfig.pin_bck = kI2sBclkPin;
  i2sConfig.pin_ws = kI2sLrckPin;
  i2sConfig.pin_data = kI2sDataPin;
  if (!audioOutput.begin(i2sConfig)) {
    Serial.println("[FATAL] I2S initialization failed");
    while (true) delay(1000);
  }
  Serial.println("[I2S] ready");

  bluetoothSink.set_avrc_metadata_attribute_mask(
      ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST);
  bluetoothSink.set_avrc_metadata_callback(metadataCallback);
  bluetoothSink.set_on_connection_state_changed(connectionCallback);
  bluetoothSink.set_on_audio_state_changed(audioStateCallback);
  bluetoothSink.set_avrc_connection_state_callback(avrcConnectionCallback);
  bluetoothSink.set_on_volumechange(volumeCallback);
  bluetoothSink.set_auto_reconnect(true);

  Serial.printf("[BT] starting A2DP sink name=%s\n", MOOSE30_DEVICE_NAME);
  bluetoothSink.start(MOOSE30_DEVICE_NAME, true);
  Serial.println("[READY] MOOSE30 audio controller ready");
  Serial.println("[HELP] Type STATE, NEXT, or PREV followed by Enter");
  markStateDirty();
}

void loop() {
  processDisplayCommands();
  processUsbCommands();
  processButtons();
  sendStateIfNeeded();
  printHeartbeat();
  delay(2);
}
