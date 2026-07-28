#pragma once
#include <Arduino.h>

namespace DisplayConfig {
constexpr uint32_t kSerialBaud = 115200;
constexpr uint32_t kLinkBaud = 115200;
constexpr uint32_t kHeartbeatIntervalMs = 5000;

constexpr int kLinkRxPin = 18;  // RX <- Classic ESP32 TX GPIO17
constexpr int kLinkTxPin = 19;  // TX -> Classic ESP32 RX GPIO16

constexpr int kLcdMosiPin = 6;
constexpr int kLcdSclkPin = 7;
constexpr int kLcdCsPin = 14;
constexpr int kLcdDcPin = 15;
constexpr int kLcdResetPin = 21;
constexpr int kBacklightPin = 22;
constexpr int kBacklightPercent = 50;

constexpr int kNativeWidth = 172;
constexpr int kNativeHeight = 320;
constexpr int kScreenWidth = 320;
constexpr int kScreenHeight = 172;
constexpr int kEdgeMargin = 5;
constexpr uint32_t kStateRequestIntervalMs = 3000;
constexpr size_t kFieldLength = 128;
constexpr size_t kLineLength = 320;
}
