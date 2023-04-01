#pragma once
/**
* HW 전용 include file.
* 
*/

#include "types.h"
#include "macro.h"

#define BYTE_PER_EVT	(20)	///< Max sizeof event.

/**
HW ID: 같은 handler를 사용하게 된다.
*/
enum HwID
{
	HW_UART,
	HW_TIMER,
	NUM_HW,
};

typedef void (*EvtHdr)(void* pEvt);

//////////////////////////////////////

void SIM_AddHW(HwID id, EvtHdr pfEvtHandler);
void* SIM_NewEvt(HwID eOwn, uint32 time);

bool SIM_PeekTick(uint32 nTick);
void SIM_SwitchToSim();		///< used by CPU.

