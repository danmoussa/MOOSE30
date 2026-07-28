#pragma once
#include <Arduino.h>

namespace Moose30Protocol {
inline void sanitizeCopy(char* destination, size_t destinationSize, const uint8_t* source) {
  if (destination == nullptr || destinationSize == 0) return;
  destination[0] = '\0';
  if (source == nullptr) return;

  size_t writeIndex = 0;
  for (size_t readIndex = 0;
       source[readIndex] != '\0' && writeIndex < destinationSize - 1;
       ++readIndex) {
    char c = static_cast<char>(source[readIndex]);
    if (c == '\r' || c == '\n') c = ' ';
    if (c == '|') c = '/';
    destination[writeIndex++] = c;
  }
  destination[writeIndex] = '\0';
}
}
