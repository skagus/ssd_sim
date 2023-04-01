#pragma once

#include "core.h"

#define UART_CFG_OFF		(0x00)
#define UART_STATE_OFF		(0x04)
#define UART_RX_BYTE		(0x08)
#define UART_TX_BYTE		(0x0C)

#define UART_RX_RDY			BIT(0)
#define UART_TX_BUSY		BIT(1)

Memory* UART_CreateHW(uint32 nBaseAddr);

void UART_PollRx();
