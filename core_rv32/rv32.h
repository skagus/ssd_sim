#pragma once
#include "types.h"
#include "core.h"

#define RET_SLEEP		FF32
#define MAX_MEM_CHUNK	(3)	// 

#define MSTATUS_MIE		(0)

enum TrapType
{
	TRAP_IRQ,
	TRAP_BUS,
	TRAP_DBG,
	TRAP_DECODE,
	NUM_TRAP,
};

enum GpId
{
	GP_ZERO,
	GP_RA,		///< Return Addr.
	GP_SP,		///< Stack pointer.
	GP_GP,		///< Global pointer.
	GP_TP,		///< Thread pointer.
	GP_T0,		///< Temporary pointer.
	GP_T1,
	GP_T2,
	GP_S0, GP_FP = GP_S0,	///< Saved Reg, Frame pointer.
	GP_S1,
	GP_A0, GP_A1, GP_A2, GP_A3, GP_A4, GP_A5, GP_A6, GP_A7,		///< func Argument.
	GP_S2, GP_S3, GP_S4, GP_S5, GP_S6, GP_S7, GP_S8, GP_S9, GP_S10, GP_S11,
	GP_T3, GP_T4, GP_T5, GP_T6,
	NUM_GP,
};

enum CsrId
{
	ssatus = 0x100,
	sedeleg = 0x102,
	sideleg,
	sie,
	stvec,
	scounteren,

	sscratch = 0x140,
	sepc,
	scause,
	stval,
	sip,

	satp = 0x180,

	mstatus = 0x300,
	misa,
	medeleg,
	mideleg,
	mie,
	mtvec,
	mcounteren,
	mstatush = 0x310,		///< status 2.

	mcountinhibit = 0x320,
	mhpmevent3 = 0x323,	// 연속.
	mhpmevent31 = 0x33F,

	mscratch = 0x340,
	mepc,
	mcause,
	mtval,
	mip,
	mtinst = 0x34A,
	mtval2,

	pmpcfg = 0x3A0,		// 연속.
	pmpcfg_15 = 0x3AF,
	pmpaddr = 0x3B0,	// 연속.
	pmpaddr_63 = 0x3EF,

	tselect = 0x7A0,
	tdata1,
	tdata2,
	tdata3,

	dcsr = 0x7B0,
	dpc,
	dscratch0,
	dscranch1,

	mcycle = 0xB00,
	minstret = 0xB02,
	mhpmcount3 = 0xB03, // 연속.
	mhpmcount31 = 0xB1F,

	NUM_CSR,
};


union CmdFormat
{
	struct
	{
		uint32 nOpc : 7;
		uint32 nRd : 5;
		uint32 nFun : 3;
		uint32 nRs1 : 5;
		uint32 nRs2 : 5;
		uint32 nFunExt : 7;
	} R;	///< Regiser Register type.
	struct
	{
		uint32 nOpc : 7;
		uint32 nRd : 5;
		uint32 nFun : 3;
		uint32 nRs1 : 5;
		uint32 nImm : 12;
	} I;	///< Short Imm and Loads.
	struct
	{
		uint32 nOpc : 7;
		uint32 nImm0 : 5;
		uint32 nFun : 3;
		uint32 nRs1 : 5;
		uint32 nRs2 : 5;
		uint32 nImm1 : 7;
	} S;	///> Store
	struct
	{ // B instruction의 Imm이 복잡한 이유는, sign을 MSB에 두고, 사용하지 않는 bit0 자리를 활용하기 위함.
		uint32 nOpc : 7;
		uint32 nImm2 : 1;
		uint32 nImm0 : 4;
		uint32 nFun : 3;
		uint32 nRs1 : 5;
		uint32 nRs2 : 5;
		uint32 nImm1 : 6;
		uint32 nImm3 : 1;
	} B;	///< Branch and variation of S-type.
	struct
	{
		uint32 nOpc : 7;
		uint32 nRd : 5;
		uint32 nImm : 20;
	} U;	///< Long Immediage.
	struct
	{
		uint32 nOpc : 7;
		uint32 nRd : 5;
		uint32 nImm2 : 8;
		uint32 nImm1 : 1;
		uint32 nImm0 : 10;
		uint32 nImm3 : 1;
	} J;	///< Jump and variation of U type.

	uint32 nRaw;
};
