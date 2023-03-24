// Copyright 2022 Charles Lohr, you may use this file or any portions herein under any of the BSD, MIT, or CC0 licenses.
#pragma once
#include <stdint.h>
#include "peri.h"
/**
	Notes:
		* There is a dedicated CLNT at 0x10000000.
		* There is free MMIO from there to 0x12000000.
		* You can put things like a UART, or whatever there.
		* Feel free to override any of the functionality with macros.
*/


// This is the functionality we want to override in the emulator.
//  think of this as the way the emulator's processor is connected to the outside world.
#define MINIRV32_RAM_IMAGE_OFFSET  0x80000000


#define MINIRV32WARN(...) printf(__VA_ARGS__);
#define MINIRV32_POSTEXEC( pc, ir, retval ) { if( retval > 0 ) { if( fail_on_all_faults ) { printf( "FAULT\n" ); return 3; } else retval = HandleException( ir, retval ); } }
#define MINIRV32_HANDLE_MEM_STORE_CONTROL( addy, val ) if( HandleControlStore( addy, val ) ) return val;
#define MINIRV32_HANDLE_MEM_LOAD_CONTROL( addy, rval ) rval = HandleControlLoad( addy );
#define MINIRV32_OTHERCSR_WRITE( csrno, value ) HandleOtherCSRWrite( image, csrno, value );

#define MINIRV32_OTHERCSR_READ(...);

#define MINIRV32_STORE4(nAddr, val) *(uint32_t*)(image + nAddr) = val
#define MINIRV32_STORE2(nAddr, val) *(uint16_t*)(image + nAddr) = val
#define MINIRV32_STORE1(nAddr, val) *(uint8_t*)(image + nAddr) = val
#define MINIRV32_LOAD4(nAddr) *(uint32_t*)(image + nAddr)
#define MINIRV32_LOAD2(nAddr) *(uint16_t*)(image + nAddr)
#define MINIRV32_LOAD1(nAddr) *(uint8_t*)(image + nAddr)

// As a note: We quouple-ify these, because in HLSL, we will be operating with
// uint4's.  We are going to uint4 data to/from system RAM.
//
// We're going to try to keep the full processor state to 12 x uint4.
struct MiniRV32IMAState
{
	uint32_t aReg[32];

	uint32_t pc;
	uint32_t mstatus;
	uint32_t cyclel;
	uint32_t cycleh;

	uint32_t timerl;
	uint32_t timerh;
	uint32_t timermatchl;
	uint32_t timermatchh;

	uint32_t mscratch;
	uint32_t mtvec;
	uint32_t mie;
	uint32_t mip;

	uint32_t mepc;
	uint32_t mtval;
	uint32_t mcause;

	// Note: only a few bits are used.  (Machine = 3, User = 0)
	// Bits 0..1 = privilege.
	// Bit 2 = WFI (Wait for interrupt)
	// Bit 3 = Load/Store has a reservation.
	uint32_t extraflags;
};

int32_t MiniRV32IMAStep( struct MiniRV32IMAState* pCpu, uint8_t * image, uint32_t vProcAddress, uint32_t elapsedUs, int count );

extern uint32_t gnSizeRAM;
extern int fail_on_all_faults;

#define CSR( x ) pCpu->x
#define SETCSR( x, val ) { pCpu->x = val; }
#define REG( x ) pCpu->aReg[x]
#define REGSET( x, val ) { pCpu->aReg[x] = val; }


