#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include "macro.h"
#include "core.h"
#include "sim_hw.h"
#include "uart.h"
/**
* UART의 memory 구조는.. 
* +0 (4B) : Setup control.
* +4 (2B) : 
*/


void handleUart(void* pInEvt);

class UART;

struct UartEvt
{
	UART* pThis;
	uint8 nData;
	bool bTx;
};

class UART : public Memory
{
	uint32 mnControl;
	uint32 mnStatus;
	uint32 mnRxBuf;
//	uint32 mnTxBuf;

	UartEvt* newEvt(bool bTx, uint32 nTimeout, uint8 nCh)
	{
		UartEvt* pNew = (UartEvt*)SIM_NewEvt(HW_UART, nTimeout);
		pNew->bTx = bTx;
		pNew->pThis = this;
		pNew->nData = nCh;
		return pNew;
	}

public:
	UART(uint32 nBaseAddr) : Memory()
	{
		mnBase = nBaseAddr;
		mnSize = 0x20;
		mnRxBuf = FF32;
//		mnTxBuf = FF32;
		mnStatus = 0;
		mnControl = 0;
		SIM_AddHW(HwID::HW_UART, handleUart);
	}

	void Handle(UartEvt* pEvt)
	{
		if(pEvt->bTx)
		{
			ASSERT(mnStatus & UART_TX_BUSY);
			mnStatus &= ~UART_TX_BUSY;
			if (nullptr != mpIrqCore)
			{
				mpIrqCore->Interrupt(mnIrqId);
			}
		}
		else
		{
			mnStatus |= UART_RX_RDY;
			mpIrqCore->Interrupt(mnIrqId);
		}
	}

	MemRet Load(uint32 nAddr, uint32* pnVal)
	{
		if (NOT(isInRange(nAddr)))
		{
			return MR_OUT_RANGE;
		}

		MemRet eRet = MR_ERROR;
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
			mnStatus |= UART_TX_BUSY;
			newEvt(true, 10, nVal);

			return MR_OK;
		}

		return MR_ERROR;
	}

	void RxPolling()
	{
//		printf("%x\n", this);
		if (_kbhit())
		{
			mnRxBuf = _getch();
			mnStatus |= UART_RX_RDY;
			if (mpIrqCore)
			{
				mpIrqCore->Interrupt(mnIrqId);
			}
		}
	}
};

UART* gpUART;



void handleUart(void* pInEvt)
{
	UartEvt* pEvt = (UartEvt*)pInEvt;
	_putch(pEvt->nData);
	pEvt->pThis->Handle(pEvt); // add handleException.
}

/**
* RX polling을 위해서..
*/
void UART_PollRx()
{
	gpUART->RxPolling();
}

Memory* UART_CreateHW(uint32 nBaseAddr)
{
	gpUART = new UART(nBaseAddr);
	return gpUART;
}

