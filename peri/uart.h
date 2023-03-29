#pragma once

#include "core.h"

Memory* UART_CreateHW(uint32 nBaseAddr);

void UART_PollRx();
