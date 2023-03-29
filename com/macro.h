
#pragma once

#include <Windows.h>
#include <stdio.h>
#include <assert.h>
#include <intrin.h>
#include <stdio.h>
#include "types.h"

//// 기본 macro 정의 ////

#ifdef FALSE
	#undef FALSE
	#undef TRUE
#endif

//extern void SIM_Print(const char* szFormat, ...);

#define NOT(x)						(!(x))
#define BRK_IF(bCond, bPrint)		do{ if (bCond){								\
									if(bPrint){printf("BRK: %s(%d) %s\n", __FILE__, __LINE__, #bCond);} \
									__debugbreak(); }}while(0)
#define ASSERT(bCond)				BRK_IF(NOT(bCond), true)
#define IF_THEN(bCond, bAssert)		ASSERT(NOT(bCond) || (bAssert))

#define STATIC_ASSERT(exp, str)		static_assert(exp, str);


#define DIV_CEIL(val, mod)			(((val) + (mod) - 1) / (mod))
#define ALIGN_UP(size, unit)		(DIV_CEIL(size, unit) * (unit))

#define BIT(shift)					(1 <<(shift))
#define BIT_SET(dst, mask)			((dst) |= (mask))
#define BIT_CLR(dst, mask)			((dst) &= ~(mask))
#define BIT_TST(dst, mask)			((dst) & (mask))
#define BIT_COUNT(val32)			__popcnt(val32)
#define BIT_ALL(count)				(BIT(count) - 1)

#define MEMSET_OBJ(obj, val)		memset((void*)&(obj), val, sizeof(obj))
#define MEMSET_ARRAY(arr, val)		memset((void*)(arr), val, sizeof(arr))
#define MEMSET_PTR(ptr, val)		memset((void*)(ptr), val, sizeof(*ptr))

#define PRINT_DEF(x)				SIM_Print("DEF: %s == %d\n", #x, x)

#define KIBI(x)						((x) * 1024)
#define MIBI(x)						KIBI((x)*1024)
#define GIBI(x)						MIBI((x)*1024)
#define MIN(x,y)					((x) > (y) ? (y) : (x))
#define MAX(x,y)					((x) > (y) ? (x) : (y))

extern const int test_array[4];

__inline uint8 BIT_CLR_1ST_LSB(uint32& bitmap)
{
	uint32 offset;	// offset is from LSB
	if (_BitScanForward((unsigned long*)(&offset), bitmap))
	{
		BIT_CLR(bitmap, BIT(offset));
	}
	else
	{
		offset = 32;
	}
	return (uint8)offset;
}

inline uint8 BIT_SCAN_LSB(uint32 bitmap)
{
	uint32 pos;
	ASSERT(bitmap != 0);
	_BitScanForward((DWORD*)&pos, bitmap);

	return (uint8)pos;
};

