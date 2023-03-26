#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include "core.h"


class UART : public Memory
{
	uint32 mnControl;
	uint32 mnRxBuf;
	uint32 mnTxBuf;
public:
	UART(uint32 nBaseAddr)
	{
		mnBase = nBaseAddr;
		mnSize = 2;
	}

	MemRet Load(uint32 nAddr, uint32* pnVal)
	{
		if (nAddr == mnBase)
		{
			*pnVal = mnControl;
			return MemRet::MR_OK;
		}
		else if (nAddr == (mnBase + 4))
		{
			if (_kbhit())
			{
				*pnVal = _getch();
			}
			else
			{
				*pnVal = FF32;
			}
			return MemRet::MR_OK;
		}
		return MemRet::MR_ERROR;
	}

	MemRet Store(uint32 nAddr, uint32 nVal)
	{
		if (nAddr == mnBase)
		{
			mnControl = nVal;
			return MemRet::MR_OK;
		}
		return MemRet::MR_ERROR;
	}

	MemRet Store(uint32 nAddr, uint8 nVal)
	{
		if (nAddr == (mnBase + 4))
		{
			putchar(nVal);
			return MemRet::MR_OK;
		}
		return MemRet::MR_ERROR;
	}
};


Memory* UART_CreateHW(uint32 nBaseAddr)
{
	UART* pUart = new UART(nBaseAddr);
	return pUart;
}

