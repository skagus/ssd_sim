#pragma once

#include "core.h"

#define TIMER_CFG_OFS			(0x00)
#define TIMER_PERIOD_OFS		(0x04)
#define TIMER_CUR_VAL_OFS		(0x08)
#define TIMER_TICK_CLK_OFS		(0x0C)

#define TIMER_ENABLE			BIT(0)
#define TIMER_INT_EN			BIT(1)

Memory* TIMER_CreateHW(uint32 nBaseAddr);

