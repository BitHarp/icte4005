#pragma once
#include <stdint.h>

void wireLinkBegin(uint32_t timeoutMicroseconds);
bool wireSend(uint8_t address, char command);
uint8_t wireRead(uint8_t address, uint8_t* bytes, uint8_t length);
