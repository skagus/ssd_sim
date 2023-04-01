#pragma once

#include <stdlib.h>
#include "types.h"

class CpuCore;

enum MemRet
{
	MR_OK,
	MR_OUT_RANGE,
	MR_ERROR,
	NUM_MR,
};

enum TrapType
{
	TRAP_IRQ,
	TRAP_BUS,
	TRAP_DBG,
	TRAP_DECODE,
	NUM_TRAP,
};


class Memory
{
protected:
	uint32 mnBase;
	uint32 mnSize;
	CpuCore* mpIrqCore;
	uint32 mnIrqId;

	inline bool isInRange(uint32 nAddr)
	{
		return ((mnBase <= nAddr) && (nAddr < mnBase + mnSize));
	}

public:
	Memory()
	{
		mpIrqCore = nullptr;
	}
	virtual MemRet Load(uint32 nAddr, uint32* pnVal) { return MR_ERROR; };
	virtual MemRet Load(uint32 nAddr, uint16* pnVal) { return MR_ERROR; };
	virtual MemRet Load(uint32 nAddr, uint8* pnVal) { return MR_ERROR; };
	virtual MemRet Store(uint32 nAddr, uint32 nVal) { return MR_ERROR; };
	virtual MemRet Store(uint32 nAddr, uint16 nVal) { return MR_ERROR; };
	virtual MemRet Store(uint32 nAddr, uint8 nVal) { return MR_ERROR; };

	void AttatchIRQ(CpuCore* pCore, uint32 nIdx)
	{
		mpIrqCore = pCore;
		mnIrqId = nIdx;
	}
};

class SRAM : public Memory
{
private:
	uint8* mpMem;

public:
	SRAM(uint32 nBase, uint32 nSize, uint8* pSpace) : Memory()
	{
		mnBase = nBase;
		mnSize = nSize;
		mpMem = (nullptr == pSpace) ? (uint8*)malloc(nSize) : pSpace;
	}
	MemRet Load(uint32 nAddr, uint32* pnVal)
	{
		if (isInRange(nAddr))
		{
			*pnVal = *(uint32*)(mpMem + (nAddr - mnBase));
			return MR_OK;
		}
		return MR_OUT_RANGE;
	}
	MemRet Load(uint32 nAddr, uint16* pnVal)
	{
		if (isInRange(nAddr))
		{
			*pnVal = *(uint16*)(mpMem + (nAddr - mnBase));
			return MR_OK;
		}
		return MR_OUT_RANGE;
	}
	MemRet Load(uint32 nAddr, uint8* pnVal)
	{
		if (isInRange(nAddr))
		{
			*pnVal = *(uint8*)(mpMem + (nAddr - mnBase));
			return MR_OK;
		}
		return MR_OUT_RANGE;
	}

	MemRet Store(uint32 nAddr, uint32 nVal)
	{
		if (isInRange(nAddr))
		{
			*(uint32*)(mpMem + (nAddr - mnBase)) = nVal;
			return MR_OK;
		}
		return MR_OUT_RANGE;
	}
	MemRet Store(uint32 nAddr, uint16 nVal)
	{
		if (isInRange(nAddr))
		{
			*(uint16*)(mpMem + (nAddr - mnBase)) = nVal;
			return MR_OK;
		}
		return MR_OUT_RANGE;
	}
	MemRet Store(uint32 nAddr, uint8 nVal)
	{
		if (isInRange(nAddr))
		{
			*(uint8*)(mpMem + (nAddr - mnBase)) = nVal;
			return MR_OK;
		}
		return MR_OUT_RANGE;
	}
};

class CpuCore
{
public:
	virtual void Init(uint32 nStart) = 0;
	virtual void AddMemory(Memory* pMem) = 0;
	virtual bool Step() = 0;
	virtual void Interrupt(uint32 nCode) = 0;
};

CpuCore* CreateCore();
