#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include "macro.h"
#include "core.h"

/**
* UART의 memory 구조는.. 
* +0 (4B) : Setup control.
* +4 (2B) : 
*/

#define UART_CFG_OFF		(0x00)
#define UART_STATE_OFF		(0x04)
#define UART_RX_BYTE		(0x08)
#define UART_TX_BYTE		(0x0C)

#define UART_RX_RDY			BIT(0)
#define UART_TX_BUSY		BIT(1)


class UART : public Memory
{
	uint32 mnControl;
	uint32 mnStatus;
	uint32 mnRxBuf;
	uint32 mnTxBuf;

public:
	UART(uint32 nBaseAddr)
	{
		mnBase = nBaseAddr;
		mnSize = 0x20;
		mnRxBuf = FF32;
		mnTxBuf = FF32;
		mnStatus = 0;
	}

	void handle()
	{
		if (_kbhit())
		{
			mnRxBuf = _getch();
			mnStatus |= UART_RX_RDY;
		}
		if (mnStatus & UART_TX_BUSY)
		{
			ASSERT(FF32 != mnTxBuf);
			_putch(mnTxBuf);
			mnStatus &= ~UART_TX_BUSY;
			mnTxBuf = FF32;
		}
	}

	MemRet Load(uint32 nAddr, uint32* pnVal)
	{
		if (NOT(isInRange(nAddr)))
		{
			return MR_OUT_RANGE;
		}

		MemRet eRet = MR_ERROR;
		handle();
		switch (nAddr - mnBase)
		{
			case UART_STATE_OFF:
			{
				*pnVal = mnStatus;
				eRet = MR_OK;
				break;
			}
			default:
			{
				ASSERT(false);
			}
		}
		return eRet;
	}

	MemRet Load(uint32 nAddr, uint8* pnVal)
	{
		if (NOT(isInRange(nAddr)))
		{
			return MR_OUT_RANGE;
		}

		MemRet eRet = MR_ERROR;
		handle();

		switch (nAddr - mnBase)
		{
			case UART_RX_BYTE:
			{
				if (FF32 != mnRxBuf)
				{
					*pnVal = mnRxBuf;
					mnRxBuf = FF32;
					mnStatus &= ~UART_RX_RDY;
				}
				eRet = MR_OK;
				break;
			}
			default:
			{
				ASSERT(false);
			}
		}
		return eRet;
	}

	MemRet Store(uint32 nAddr, uint32 nVal)
	{
		if (NOT(isInRange(nAddr)))
		{
			return MR_OUT_RANGE;
		}

		MemRet eRet = MR_ERROR;

		uint32 nOff = nAddr - mnBase;
		if (UART_CFG_OFF == nOff)
		{
			mnControl = nVal;
			eRet = MR_OK;
		}
		return eRet;
	}

	MemRet Store(uint32 nAddr, uint8 nVal)
	{
		if (NOT(isInRange(nAddr)))
		{
			return MR_OUT_RANGE;
		}

		uint32 nOff = nAddr - mnBase;
		if (UART_TX_BYTE == nOff)
		{
			ASSERT(FF32 == mnTxBuf);
			mnStatus |= UART_TX_BUSY;
			mnTxBuf = nVal;
			return MR_OK;
		}

		return MR_ERROR;
	}
};


Memory* UART_CreateHW(uint32 nBaseAddr)
{
	UART* pUart = new UART(nBaseAddr);
	return pUart;
}

