#pragma once
#include <Arduino.h>

namespace Moose30Config {
constexpr uint32_t kSerialBaud = 115200;
constexpr uint32_t kLinkBaud = 115200;
constexpr uint32_t kHeartbeatIntervalMs = 5000;

constexpr int kI2sBclkPin = 26;
constexpr int kI2sLrckPin = 25;
constexpr int kI2sDataPin = 22;

constexpr int kDisplayRxPin = 16;  // RX <- C6 TX GPIO19
constexpr int kDisplayTxPin = 17;  // TX -> C6 RX GPIO18

constexpr int kPreviousButtonPin = 32;
constexpr int kNextButtonPin = 33;
constexpr uint32_t kButtonDebounceMs = 180;

constexpr size_t kMetadataLength = 128;
constexpr size_t kCommandLength = 24;
}
