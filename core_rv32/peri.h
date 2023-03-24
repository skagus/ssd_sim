#pragma once
#include <stdint.h>

uint64_t GetTimeMicroseconds();

int IsKBHit();
int ReadKBByte();
void MiniSleep();
void ResetKeyboardInput();
void CaptureKeyboardInput();


uint32_t HandleControlStore(uint32_t addy, uint32_t val);
uint32_t HandleControlLoad(uint32_t addy);
