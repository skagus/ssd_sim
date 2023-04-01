
#include "sim_hw.h"
#include "timer.h"

void handleTimer(void* pInEvt);

class Timer;

struct TimerEvt
{
	Timer* pThis;
	bool bValid;
};

/**
* 이 타이머는, periodic down count mode만 지원한다. 
* ISR 지원가능.
*/
class Timer : public Memory
{
	uint32 mnPeriod;
	uint32 mnRestTick;
	uint32 mnTickPeriod;
	uint32 mnCfg;
	TimerEvt* mpPrvEvt;

	TimerEvt* newEvt(uint32 nTimeout)
	{
		TimerEvt* pNew = (TimerEvt*)SIM_NewEvt(HW_TIMER, nTimeout);
		pNew->pThis = this;
		pNew->bValid = true;
		return pNew;
	}

public:
	Timer(uint32 nBaseAddr) : Memory()
	{
		mnBase = nBaseAddr;
		mnSize = 0x10;
		SIM_AddHW(HwID::HW_TIMER, handleTimer);
		mnCfg = 0;
		mpPrvEvt = 0;
	}

	void Handle(TimerEvt* pEvt)
	{
		mpPrvEvt = nullptr;
		if (pEvt->bValid)
		{
			if (mnCfg & TIMER_ENABLE)
			{
				mnRestTick--;
				if (0 == mnRestTick)
				{
					mnRestTick = mnPeriod;
					if ((mnCfg & TIMER_INT_EN) && (nullptr != mpIrqCore))
					{
						mpIrqCore->Interrupt(mnIrqId);
					}
				}
				mpPrvEvt = newEvt(mnTickPeriod);
			}
		}
	}

	MemRet Load(uint32 nAddr, uint32* pnVal)
	{
		if (NOT(isInRange(nAddr)))
		{
			return MR_OUT_RANGE;
		}
		uint32 nOff = nAddr - mnBase;
		if (TIMER_CFG_OFS == nOff)
		{
			*pnVal = mnCfg;
		}
		else if (TIMER_PERIOD_OFS == nOff)
		{
			*pnVal = mnPeriod;
		}
		else if (TIMER_CUR_VAL_OFS == nOff)
		{
			*pnVal = mnRestTick;
		}
		else if (TIMER_TICK_CLK_OFS == nOff)
		{
			*pnVal = mnTickPeriod;
		}
		else
		{
			return MR_ERROR;
		}
		return MR_OK;
	}

	MemRet Store(uint32 nAddr, uint32 nVal)
	{
		if (NOT(isInRange(nAddr)))
		{
			return MR_OUT_RANGE;
		}
		MemRet eRet = MR_ERROR;
		uint32 nOff = nAddr - mnBase;
		if (TIMER_CFG_OFS == nOff)
		{
			// Timer Enable을 적을 때마다 timer를 새로 시작한다.(이전 값은 중요하지 않음)
			if (TIMER_ENABLE & nVal)
			{
				if (nullptr != mpPrvEvt)
				{
					mpPrvEvt->bValid = false;
				}
				mpPrvEvt = newEvt(mnTickPeriod);
			}
			mnCfg = nVal;
		}
		else if (TIMER_PERIOD_OFS == nOff)
		{
			mnPeriod = nVal;
		}
		else if (TIMER_CUR_VAL_OFS == nOff)
		{
			mnRestTick = nVal;
		}
		else if (TIMER_TICK_CLK_OFS == nOff)
		{
			mnTickPeriod = nVal;
		}
		else
		{
			return MR_ERROR;
		}
		return MR_OK;
	}
};

void handleTimer(void* pInEvt)
{
	TimerEvt* pEvt = (TimerEvt*)pInEvt;
	pEvt->pThis->Handle(pEvt); // add handleException.
}


Memory* TIMER_CreateHW(uint32 nBaseAddr)
{
	return new Timer(nBaseAddr);
}
