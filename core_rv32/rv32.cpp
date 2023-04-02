#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "macro.h"
#include "rv32.h"

#define INC_PC()			(mnPC += 4)
#define UN_SUPPORTED()		assert(false)
#define UNDEF_INST(nAddr)	Exception(TRAP_DECODE, nAddr)
#define XLEN				(32)
#define PRINT_INST(...)		// printf(__VA_ARGS__)
//#define PRINT_INST(...)
#define REGNAME(idx)		(gaRegName[idx])
const char* gaRegName[]
{
	"Z0",
	"RA",		///< Return Addr.
	"SP",		///< Stack pointer.
	"GP",		///< Global pointer.
	"TP",		///< Thread pointer.
	"T0",		///< Temporary pointer.
	"T1",
	"T2",
	"S0",
	"S1",
	"A0",		///< Func Arg0
	"A1",
	"A2",
	"A3",
	"A4",
	"A5",
	"A6",
	"A7",		///< func Arg7
	"S2",
	"S3",
	"S4",
	"S5",
	"S6",
	"S7",
	"S8",
	"S9",
	"S10",
	"S11",
	"T3",
	"T4",
	"T5",
	"T6",
};

class RV32Core: public CpuCore
{
private:
	uint32 mnPC;
	uint32 maRegs[NUM_GP];
	uint32 maCSR[NUM_CSR];

	uint8 mnCntMem;
	Memory* maChunk[MAX_MEM_CHUNK];

	uint32 mbmTrap;	///< Trap information. @see TrapId
	uint32 mTrapParam;

	bool mbWFI;		///< true if WFI.

	template <typename T>
	MemRet load(uint32 nAddr, T* pnVal)
	{
		MemRet eRet;
		for (uint32 nChunk = 0; nChunk < mnCntMem; nChunk++)
		{
			eRet = maChunk[nChunk]->Load(nAddr, pnVal);
			if (MR_OUT_RANGE != eRet)
			{
				return eRet;
			}
		}
		return MR_OUT_RANGE;
	}

	template <typename T>
	MemRet store(uint32 nAddr, T nVal)
	{
		MemRet eRet;
		for (uint32 nChunk = 0; nChunk < mnCntMem; nChunk++)
		{
			eRet = maChunk[nChunk]->Store(nAddr, nVal);
			if (MR_OUT_RANGE != eRet)
			{
				return eRet;
			}
		}
		return MR_OUT_RANGE;
	}

	void doArith(CmdFormat stInst, uint32 nPC)
	{
		switch (stInst.R.nFun)
		{
			case 0b000: // ADD, SUB, MUL
			{
				if (0 == stInst.R.nFunExt)  // ADD
				{
					PRINT_INST("%08X: ADD %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					maRegs[stInst.R.nRd] = (int32)maRegs[stInst.R.nRs1] + (int32)maRegs[stInst.R.nRs2];
				}
				else if(0b100000 == stInst.R.nFunExt) // SUB
				{
					PRINT_INST("%08X: SUB %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					maRegs[stInst.R.nRd] = (int32)maRegs[stInst.R.nRs1] - (int32)maRegs[stInst.R.nRs2];
				}
				else if (0b01 == stInst.R.nFunExt)	// MUL, M-ext
				{
					PRINT_INST("%08X: MUL %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs1] * maRegs[stInst.R.nRs2];
				}
				break;
			}
			case 0b001:	// SLL, MULH
			{
				if (0 == stInst.R.nFunExt) // SLL : Shift Left Logical.
				{
					PRINT_INST("%08X: SLL %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs1] << maRegs[stInst.R.nRs2];
				}
				else if (0b01 == stInst.R.nFunExt) // MULH, M-ext
				{
					PRINT_INST("%08X: MULH %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					maRegs[stInst.R.nRd] = ((int64)(int32)maRegs[stInst.R.nRs1] * (int64)(int32)maRegs[stInst.R.nRs2]) >> XLEN;
				}
				break;
			}
			case 0b010:	// SLT, MULHSU.
			{
				if (0b00 == stInst.R.nFunExt) // SLT : Set Less Than.
				{
					PRINT_INST("%08X: SLT %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					maRegs[stInst.R.nRd] = ((int)maRegs[stInst.R.nRs1] < (int)maRegs[stInst.R.nRs2]) ? 1 : 0;
				}
				else if (0b01 == stInst.R.nFunExt) // MULHSU, M-ext
				{
					PRINT_INST("%08X: MULHSU %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					maRegs[stInst.R.nRd] = ((int64)(int32)maRegs[stInst.R.nRs1] * (uint64)maRegs[stInst.R.nRs2]) >> XLEN;
				}
				break;
			}
			case 0b011:	// SLTU : Set Less Than Unsigned.
			{
				if (0b00 == stInst.R.nFunExt) // SLTU
				{
					PRINT_INST("%08X: SLTU %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					maRegs[stInst.R.nRd] = (maRegs[stInst.R.nRs1] < maRegs[stInst.R.nRs2]) ? 1 : 0;
				}
				else if (0b01 == stInst.R.nFunExt) // MULHU, M-ext
				{
					PRINT_INST("%08X: MULHU %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					maRegs[stInst.R.nRd] = ((uint64)maRegs[stInst.R.nRs1] * (uint64)maRegs[stInst.R.nRs2]) >> XLEN;
				}
				break;
			}
			case 0b100:	// XOR
			{
				if (0b00 == stInst.R.nFunExt) // XOR
				{
					PRINT_INST("%08X: XOR %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs1] ^ maRegs[stInst.R.nRs2];
				}
				else if (0b01 == stInst.R.nFunExt) // DIV, M-ext
				{
					PRINT_INST("%08X: DIV %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					if (0 != maRegs[stInst.R.nRs2])
					{
						maRegs[stInst.R.nRd] = (int32)maRegs[stInst.R.nRs1] / (int32)maRegs[stInst.R.nRs2];
					}
					else
					{
						maRegs[stInst.R.nRd] = FF32;
					}
				}
				break;
			}
			case 0b101:	// SRL, SRA
			{
				if (0 == stInst.R.nFunExt)  // SRL : Shift Right Logical.
				{
					PRINT_INST("%08X: SRL %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs1] >> maRegs[stInst.R.nRs2];
				}
				else if(0b0100000 == stInst.R.nFunExt)// SRA : Shift Right Arithmatic(부호 비트를 살리면서)
				{
					PRINT_INST("%08X: SRA %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					maRegs[stInst.R.nRd] = (int32)maRegs[stInst.R.nRs1] >> (int32)maRegs[stInst.R.nRs2];
				}
				else if (0b01 == stInst.R.nFunExt) // DIVU, M-ext
				{
					PRINT_INST("%08X: DIVU %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					if (0 != maRegs[stInst.R.nRs2])
					{
						maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs1] / maRegs[stInst.R.nRs2];
					}
					else
					{
						maRegs[stInst.R.nRd] = FF32;
					}
				}

				break;
			}
			case 0b110:	// OR
			{
				if (0 == stInst.R.nFunExt)  // OR
				{
					PRINT_INST("%08X: OR %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs1] | maRegs[stInst.R.nRs2];
				}
				else if (0b01 == stInst.R.nFunExt) // REM, M-ext
				{
					PRINT_INST("%08X: REM %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					if (0 != maRegs[stInst.R.nRs2])
					{
						maRegs[stInst.R.nRd] = (int32)maRegs[stInst.R.nRs1] % (int32)maRegs[stInst.R.nRs2];
					}
					else
					{
						maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs1];
					}
				}
				break;
			}
			case 0b111:	// AND
			{
				if (0 == stInst.R.nFunExt)  // AND
				{
					PRINT_INST("%08X: AND %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs1] & maRegs[stInst.R.nRs2];
				}
				else if (0b01 == stInst.R.nFunExt) // REMU, M-ext
				{
					PRINT_INST("%08X: REMU %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
					if (0 != maRegs[stInst.R.nRs2])
					{
						maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs1] % maRegs[stInst.R.nRs2];
					}
					else
					{
						maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs1];
					}
				}
				break;
			}
			
		}
	}

	void doArithImm(CmdFormat stInst, uint32 nPC)
	{
		uint32 nImm = stInst.I.nImm;
		if (nImm & 0x800) nImm |= 0xFFFFF000;
		switch (stInst.I.nFun)
		{
			case 0b000: // ADDI.
			{
				PRINT_INST("%08X: ADDI %s %s %d\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), (int32)nImm);
				maRegs[stInst.I.nRd] = (int32)maRegs[stInst.I.nRs1] + (int32)nImm;
				break;
			}
			case 0b001:	// SLLI
			{
				uint32 nShamt = stInst.R.nRs2;
				PRINT_INST("%08X: SLLI %s %s 0x%X\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), nShamt);
				maRegs[stInst.I.nRd] = maRegs[stInst.I.nRs1] << nShamt;
				break;
			}
			case 0b101: // SRLI, SRAI
			{
				uint32 nShamt = stInst.R.nRs2;
				if (0 == stInst.R.nFunExt) // SRLI.
				{
					PRINT_INST("%08X: SRLI %s %s 0x%X\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), nShamt);
					maRegs[stInst.I.nRd] = maRegs[stInst.I.nRs1] >> nShamt;
				}
				else // SRAI.
				{
					PRINT_INST("%08X: SRAI %s %s 0x%X\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), nShamt);
					maRegs[stInst.I.nRd] = (int32)maRegs[stInst.I.nRs1] >> nShamt;
				}
				break;
			}
			case 0b010: // SLTI : Set Less Than Imm.
			{
				PRINT_INST("%08X: SLTI %s %s %d\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), (int32)nImm);
				maRegs[stInst.I.nRd] = ((int32)maRegs[stInst.I.nRs1] < (int32)nImm) ? 1 : 0;
				break;
			}
			case 0b011: // SLTIU
			{
				PRINT_INST("%08X: SLTU %s %s 0x%X\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), nImm);
				maRegs[stInst.I.nRd] = (maRegs[stInst.I.nRs1] < nImm) ? 1 : 0;
				break;
			}
			case 0b100:	// XORI
			{
				PRINT_INST("%08X: XORI %s %s 0x%X\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), nImm);
				maRegs[stInst.I.nRd] = maRegs[stInst.I.nRs1] ^ nImm;
				break;
			}
			case 0b110:	// ORI
			{
				PRINT_INST("%08X: ORI %s %s 0x%X\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), nImm);
				maRegs[stInst.I.nRd] = maRegs[stInst.I.nRs1] | nImm;
				break;
			}
			case 0b111: // ANDI.
			{
				PRINT_INST("%08X: ANDI %s %s 0x%X\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), nImm);
				maRegs[stInst.I.nRd] = maRegs[stInst.I.nRs1] & nImm;
				break;
			}
			UNDEF_INST(nPC);
		}
	}

	void doBranch(CmdFormat stInst, uint32 nPC)
	{
		uint32 nImm = stInst.S.nImm0 & 0x1E; // 4:1
		nImm |= (stInst.S.nImm0 & 0x1) << 11; // 11
		nImm |= (stInst.S.nImm1 & 0x3F) << 5; // 10:5
		nImm |= (stInst.S.nImm1 & 0x40) << 12; // 12

		if (nImm & 0x800)
		{
			nImm = (uint32)nImm | 0xFFFFF000;
		}
		switch (stInst.S.nFun)
		{
			case 0b000:	// BEQ
			{
				PRINT_INST("%08X: BEQ %s %s 0x%X\n", nPC, REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2), (int32)nImm);
				if (maRegs[stInst.S.nRs1] == maRegs[stInst.S.nRs2]) mnPC = nPC + (int32)nImm;
				break;
			}
			case 0b001:	// BNE
			{
				PRINT_INST("%08X: BNE %s %s 0x%X\n", nPC, REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2), (int32)nImm);
				if (maRegs[stInst.S.nRs1] != maRegs[stInst.S.nRs2]) mnPC = nPC + (int32)nImm;
				break;
			}
			case 0b100:	// BLT
			{
				PRINT_INST("%08X: BLT %s %s 0x%X\n", nPC, REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2), (int32)nImm);
				if ((int32)maRegs[stInst.S.nRs1] < (int32)maRegs[stInst.S.nRs2]) mnPC = nPC + (int32)nImm;
				break;
			}
			case 0b101:	// BGE
			{
				PRINT_INST("%08X: BGE %s %s 0x%X\n", nPC, REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2), (int32)nImm);
				if ((int32)maRegs[stInst.S.nRs1] >= (int32)maRegs[stInst.S.nRs2]) mnPC = nPC + (int32)nImm;
				break;
			}
			case 0b110:	// BLTU : Less than.
			{
				PRINT_INST("%08X: BLTU %s %s 0x%X\n", nPC, REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2), (int32)nImm);
				if (maRegs[stInst.S.nRs1] < maRegs[stInst.S.nRs2]) mnPC = nPC + (int32)nImm;
				break;
			}
			case 0b111:	// BGEU : Bigger or Equal Unsigned.
			{
				PRINT_INST("%08X: BGEU %s %s 0x%X\n", nPC, REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2), (int32)nImm);
				if (maRegs[stInst.S.nRs1] >= maRegs[stInst.S.nRs2]) mnPC = nPC + (int32)nImm;
				break;
			}
			UNDEF_INST(nPC);
		}
	}

	/**
	* A extention.
	* 부가 설명.
	* Load Reserved는 load와 동일한데, reservation을 등록한다는 점이 있고, 
	* Store Conditional은 reserved된 memory가 다른 명령에 의해서 update되었다면, updata가 실패한다.
	* 실제 구현은 tag등을 걸어놓아서 update되지 않았음을 보증하게 될텐데, 
	* resource제약 등으로, memory전체에 대해서 1개만 tagging을 하거나, 
	* memory 영역을 나누어서 영역별 tagging을 하는 방식을 쓰기도 한다. 
	* 
	* 이 개념은 multi core환경에서 의미있는 것으로, single core에서는 무시 가능한듯..
	*/
	void doAtomic(CmdFormat stInst, uint32 nPC)
	{
		uint32 nVal;
		uint32 nAddrS1 = maRegs[stInst.R.nRs1];

		static uint32 nResevedAddr;

		switch (stInst.R.nFunExt)
		{
			case 0b00010:	// LR.W : Load Reserved.
			{
				PRINT_INST("%08X: LR.W %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1));
				nResevedAddr = nAddrS1;
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = nVal;
				break;
			}
			case 0b00011:	// SC.W : Store Conditional.
			{
				PRINT_INST("%08X: SC.W %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
				if (nResevedAddr == nAddrS1) // success.
				{
					store(nAddrS1, maRegs[stInst.R.nRs2]);
					maRegs[stInst.R.nRd] = 0;
				}
				else
				{
					maRegs[stInst.R.nRd] = 1;
				}
				nResevedAddr = FF32;
				break;
			}
			case 0b00001:	// SWAP.W
			{
				PRINT_INST("%08X: SWAP.W %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs2];
				maRegs[stInst.R.nRs2] = nVal;
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			case 0b00000:	// ADD.W
			{
				PRINT_INST("%08X: ADD.W %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = nVal + maRegs[stInst.R.nRs2];
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			case 0b00100:	// XOR.W
			{
				PRINT_INST("%08X: XOR.W %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = nVal ^ maRegs[stInst.R.nRs2];
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			case 0b01100:	// AND.W
			{
				PRINT_INST("%08X: AND.W %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = nVal & maRegs[stInst.R.nRs2];
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			case 0b01000:	// OR.W
			{
				PRINT_INST("%08X: OR.W %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = nVal | maRegs[stInst.R.nRs2];
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			case 0b10000:	// MIN.W
			{
				PRINT_INST("%08X: MIN.W %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = MIN((int32)nVal, (int32)maRegs[stInst.R.nRs2]);
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			case 0b10100:	// MAX.W
			{
				PRINT_INST("%08X: MAX.W %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = MAX((int32)nVal, (int32)maRegs[stInst.R.nRs2]);
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			case 0b11000:	// MINU.W
			{
				PRINT_INST("%08X: MINU.W %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = MIN(nVal, maRegs[stInst.R.nRs2]);
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			case 0b11100:	// MAXU.W
			{
				PRINT_INST("%08X: MAXU.W %s %s %s\n", nPC, REGNAME(stInst.R.nRd), REGNAME(stInst.R.nRs1), REGNAME(stInst.R.nRs2));
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = MAX(nVal, maRegs[stInst.R.nRs2]);
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			UNDEF_INST(nPC);
		}
		maRegs[stInst.R.nRd] = 0;
	}

	void doSystem(CmdFormat stInst, uint32 nPC) // 0b1110011
	{
		// 0x30200073
		// 0b0011_0000_0010_.0000_0.000_.0000_0.111_0011
		//   Imm             Rs1    nFun nRd    nOp.
		switch (stInst.I.nFun)
		{
			case 0b000: // ECALL, EBREAK,
			{
				if (0x000 == stInst.I.nImm) // ECALL
				{
					PRINT_INST("%08X: ECALL A0: 0x%X\n", nPC, maRegs[GP_A0]);
					if (0 == maRegs[GP_A0]) // Test Failure.
					{
						printf("\n\tFaile\n\n");
						ASSERT(false);
					}
					else if (0x2A == maRegs[GP_A0]) // Test success.
					{
						printf("\n\tSuccess\n\n");
						exit(0);
					}
					else if (1 == maRegs[GP_A0]) // Print line.
					{
						uint8 nVal;
						uint32 nDataAddr = maRegs[GP_A1];
						while (true)
						{
							load(nDataAddr, &nVal);
							if (0 == nVal)
							{
								break;
							}
							putchar(nVal);
							nDataAddr++;
						}
					}
					else
					{
						ASSERT(false);
					}
				}	
				else if (0x001 == stInst.I.nImm)// EBREAK
				{
					PRINT_INST("%08X: EBREAK\n", nPC);
				} 
				else if (0x002 == stInst.I.nImm) // URET.
				{
					PRINT_INST("%08X: URET\n", nPC);
				}
				else if (0x102 == stInst.I.nImm) // SRET.
				{
					PRINT_INST("%08X: SRET\n", nPC);
				}
				else if (0x202 == stInst.I.nImm) // HRET.
				{
					PRINT_INST("%08X: HRET\n", nPC);
				}
				else if (0x302 == stInst.I.nImm) // MRET.
				{
					PRINT_INST("%08X: MRET\n", nPC);
					if (maCSR[mip]) // Pending interrupt exist.
					{
						int32 nIsrCode = BIT_SCAN_LSB(maCSR[mip]);
						BIT_CLR(maCSR[mip], BIT(nIsrCode));
						maCSR[mcause] = nIsrCode;
						uint32 nVAddr = maCSR[mtvec] + (4 * nIsrCode);
						load(nVAddr, &mnPC);
					}
					else
					{
						mnPC = maCSR[mepc];
						maCSR[mstatus] |= BIT(MSTATUS_MIE);
						maCSR[mcause] = FF32;
					}
				}
				else if (0x105 == stInst.I.nImm) // WFI
				{
					PRINT_INST("%08X: WFI\n", nPC);
					maCSR[mstatus] |= BIT(MSTATUS_MIE);
					mbWFI = true;
				}
				break;
			}
			case 0b001:	// CSRRW: SWAP CSR value.
			{
				PRINT_INST("%08X: CSRRW %s %s 0x%X\n", nPC, REGNAME(stInst.I.nRd), REGNAME(stInst.I.nRs1), stInst.I.nImm);
				if (0 != stInst.I.nRd)
				{
					maRegs[stInst.I.nRd] = maCSR[stInst.I.nImm];
				}
				maCSR[stInst.I.nImm] = maRegs[stInst.I.nRs1];
				break;
			}
			case 0b010:	// CSRRS: Read and Set bit.
			{
				PRINT_INST("%08X: CSRRS %s %s 0x%X\n", nPC, REGNAME(stInst.I.nRd), REGNAME(stInst.I.nRs1), stInst.I.nImm);
				if (0 != maRegs[stInst.I.nRs1])
				{
					maRegs[stInst.I.nRd] = maCSR[stInst.I.nImm];
				}
				if (0 != maRegs[stInst.I.nRs1])
				{
					maCSR[stInst.I.nImm] |= maRegs[stInst.I.nRs1];
				}
				break;
			}
			case 0b011: // CSRRC: Read and Clear bit.
			{
				PRINT_INST("%08X: CSRRC %s %s 0x%X\n", nPC, REGNAME(stInst.I.nRd), REGNAME(stInst.I.nRs1), stInst.I.nImm);
				if (0 != maRegs[stInst.I.nRd])
				{
					maRegs[stInst.I.nRd] = maCSR[stInst.I.nImm];
				}
				if (0 != maRegs[stInst.I.nRs1])
				{
					maCSR[stInst.I.nImm] &= ~maRegs[stInst.I.nRs1];
				}
				break;
			}
			case 0b101: // CSRRWI: Imm
			{
				PRINT_INST("%08X: CSRRWI %s %s 0x%X\n", nPC, REGNAME(stInst.I.nRd), REGNAME(stInst.I.nRs1), stInst.I.nImm);
				if (0 != maRegs[stInst.I.nRd])
				{
					maRegs[stInst.I.nRd] = maCSR[stInst.I.nImm];
				}
				maCSR[stInst.I.nImm] = stInst.I.nRs1;
				break;
			}
			case 0b110: // CSRRSI: 
			{
				PRINT_INST("%08X: CSRRSI %s %s 0x%X\n", nPC, REGNAME(stInst.I.nRd), REGNAME(stInst.I.nRs1), stInst.I.nImm);
				if (0 != maRegs[stInst.I.nRd])
				{
					maRegs[stInst.I.nRd] = maCSR[stInst.I.nImm];
				}
				maCSR[stInst.I.nImm] |= stInst.I.nRs1;
				break;
			}
			case 0b111: // CSRRCI.
			{
				PRINT_INST("%08X: CSRRCI %s %s 0x%X\n", nPC, REGNAME(stInst.I.nRd), REGNAME(stInst.I.nRs1), stInst.I.nImm);
				if (0 != maRegs[stInst.I.nRd])
				{
					maRegs[stInst.I.nRd] = maCSR[stInst.I.nImm];
				}
				maCSR[stInst.I.nImm] &= ~stInst.I.nRs1;
				break;
			}
			UNDEF_INST(nPC);
		}
	}

	/**
	* @return true if continue, false on WFI.
	*/
	inline bool handleException()
	{
		if (mbmTrap != 0)
		{
			if (mbmTrap & BIT(TRAP_BUS))
			{

			}
			else if (mbmTrap & BIT(TRAP_IRQ))
			{
				ASSERT(maCSR[mstatus] & BIT(MSTATUS_MIE));
				maCSR[mepc] = mnPC;
				maCSR[mstatus] &= ~BIT(MSTATUS_MIE);

				int32 nIsrCode = BIT_SCAN_LSB(maCSR[mip]);
				BIT_CLR(maCSR[mip], BIT(nIsrCode));
				maCSR[mcause] = nIsrCode;
				uint32 nVAddr = maCSR[mtvec] + (4 * nIsrCode);
				load(nVAddr, &mnPC);
				mbmTrap &= ~BIT(TRAP_IRQ);
				mbWFI = false;
			}
		}
		return mbWFI;
	}

public:
	void Init(uint32 nStart)
	{
		mnCntMem = 0;
		memset(maRegs, 0, sizeof(maRegs));
		mbmTrap = 0;
		mbWFI = false;
		mnPC = nStart;
		maCSR[mcycle] = 0;
		maCSR[mcause] = FF32;
	}

	void AddMemory(Memory* pMem)
	{
		maChunk[mnCntMem] = pMem;
		mnCntMem++;
	}

	void Interrupt(uint32 nCode)
	{
		if (maCSR[mstatus] & BIT(MSTATUS_MIE))
		{
			mbmTrap |= BIT(TRAP_IRQ);
		}
		BIT_SET(maCSR[mip], BIT(nCode));
	}

	void Exception(TrapType nType, uint32 nParam)
	{
		mbmTrap |= BIT(nType);
		mTrapParam = nParam;
	}

	bool Step()
	{
		if (handleException())
		{
			return true;
		}

		CmdFormat stInst;

		MemRet eMR = load(mnPC, &stInst.nRaw);
		maCSR[mcycle]++;
		if (MR_OK == eMR)
		{
			uint32 nPC = mnPC;
			INC_PC();
//			printf("0x%08X: %8X \n", nPC, stInst.nRaw);

			switch (stInst.B.nOpc)
			{
				case 0b0010111: // AUIPC
				{
					PRINT_INST("%08X: AUIPC %s 0x%X\n", nPC, REGNAME(stInst.I.nRd), stInst.I.nImm);
					maRegs[stInst.U.nRd] = nPC + (int32)(stInst.U.nImm << 12);
					break;
				}
				case 0b0110011:	// ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
				{
					doArith(stInst, nPC);
					break;
				}
				case 0b0010011:	// ADDI, SLLI, SRLI, SRAI, SLTI, SLTIU, XORI, ORI, ANDI.
				{
					doArithImm(stInst, nPC);
					break;
				}
				case 0b0100011: // Store.
				{
					uint32 nAddr = maRegs[stInst.S.nRs1]; // Base.
					uint32 nImm = stInst.S.nImm0 + (stInst.S.nImm1 << 5);
					nAddr += (nImm & 0x800) ? (nImm | 0xFFFFF000) : nImm;
					uint32 nVal = maRegs[stInst.S.nRs2];
					switch (stInst.S.nFun)
					{
						case 0b000:  // SB
						{
							PRINT_INST("%08X: SB %s 0x%X (%s)\n", nPC, REGNAME(stInst.S.nRs2), (int32)nImm, REGNAME(stInst.S.nRs1));
							eMR = store(nAddr, (uint8)nVal);
							break;
						}
						case 0b001:  // SH
						{
							PRINT_INST("%08X: SH %s 0x%X (%s)\n", nPC, REGNAME(stInst.S.nRs2), (int32)nImm, REGNAME(stInst.S.nRs1));
							eMR = store(nAddr, (uint16)nVal);
							break;
						}
						case 0b010:  // SW
						{
							PRINT_INST("%08X: SW %s 0x%X (%s)\n", nPC, REGNAME(stInst.S.nRs2), (int32)nImm, REGNAME(stInst.S.nRs1));
							eMR = store(nAddr, (uint32)nVal);
							break;
						}
						UNDEF_INST(nPC);
					}
					break;
				}
				case 0b0000011:	// LB, LH, LW, LBU, LHU
				{
					uint32 nImm = stInst.I.nImm;
					if (0x800 & nImm)
					{
						nImm |= 0xFFFFF000;
					}
					uint32 nSrcAddr = maRegs[stInst.I.nRs1] + (int32)nImm;
					
					switch (stInst.I.nFun)
					{
						case 0b000:	// LB
						{
							PRINT_INST("%08X: LB %s 0x%X (%s)\n", nPC, REGNAME(stInst.I.nRd), (int32)nImm, REGNAME(stInst.I.nRs1));
							uint8 nRead;
							eMR = load(nSrcAddr, &nRead);
							maRegs[stInst.I.nRd] = (int8)nRead;
							break;
						}
						case 0b001:	// LH
						{
							PRINT_INST("%08X: LH %s 0x%X (%s)\n", nPC, REGNAME(stInst.I.nRd), (int32)nImm, REGNAME(stInst.I.nRs1));
							uint16 nRead;
							eMR = load(nSrcAddr, &nRead);
							maRegs[stInst.I.nRd] = (int16)nRead;
							break;
						}
						case 0b010:	// LW
						{
							PRINT_INST("%08X: LW %s 0x%X (%s)\n", nPC, REGNAME(stInst.I.nRd), (int32)nImm, REGNAME(stInst.I.nRs1));
							uint32 nRead;
							eMR = load(nSrcAddr, &nRead);
							maRegs[stInst.I.nRd] = nRead;
							break;
						}
						case 0b100:	// LBU
						{
							PRINT_INST("%08X: LBU %s 0x%X (%s)\n", nPC, REGNAME(stInst.I.nRd), (int32)nImm, REGNAME(stInst.I.nRs1));
							uint8 nRead;
							eMR = load(nSrcAddr, &nRead);
							maRegs[stInst.I.nRd] = nRead;
							break;
						}
						case 0b101:	// LHU
						{
							PRINT_INST("%08X: LHU %s 0x%X (%s)\n", nPC, REGNAME(stInst.I.nRd), (int32)nImm, REGNAME(stInst.I.nRs1));
							uint16 nRead;
							eMR = load(nSrcAddr, &nRead);
							maRegs[stInst.I.nRd] = nRead;
							break;
						}
						UNDEF_INST(nPC);
					}
					break;
				}
				case 0b0110111:	// LUI. Load Upper Imm.
				{
					maRegs[stInst.U.nRd] = stInst.U.nImm << 12;
					PRINT_INST("%08X: LUI %s 0x%X\n", nPC, REGNAME(stInst.U.nRd), stInst.U.nImm << 12);
					break;
				}
				case 0b1101111:	// JAL.
				{
					uint32 nImm20 = stInst.J.nImm0 | (stInst.J.nImm1 << 10)
						| (stInst.J.nImm2 << 11) | (stInst.J.nImm3 << 19);
					maRegs[stInst.J.nRd] = nPC + 4;
					if (nImm20 & 0x80000)
					{
						nImm20 |= 0xFFF00000;
					}
					mnPC = nPC + (int32)(nImm20 * 2);	// 주소는 2B aligned.
					PRINT_INST("%08X: JAL %s 0x%X\n", nPC, REGNAME(stInst.J.nRd), (int32)(nImm20 * 2));
					break;
				}
				case 0b1100111:	// JALR.
				{
					uint32 nImm = stInst.I.nImm;
					if (nImm & 0x800)
					{
						nImm |= 0xFFFFF000;
					}
					if (0 != stInst.I.nRd)
					{
						maRegs[stInst.I.nRd] = nPC + 4;
					}
					mnPC = maRegs[stInst.I.nRs1] + (int32)nImm;
					PRINT_INST("%08X: JAL %s %s 0x%X\n", nPC, REGNAME(stInst.I.nRd), REGNAME(stInst.I.nRs1), stInst.I.nImm);
					break;
				}
				case 0b1100011: // BEQ, BNE, BLT, BLTU, BGEU
				{
					doBranch(stInst, nPC);
					break;
				}
				case 0b0101111: // LR.W, SC.W, AMO{SWAP, ADD, XOR, AND, OR, MIN, MAX, MINU, MAXU}.W
				{
					ASSERT(0b010 == stInst.R.nFun);
					doAtomic(stInst, nPC);
					break;
				}
				case 0b0001111:	// FENCE, FENCE.I
				{
					switch (stInst.I.nFun)
					{
						case 0b000:	// FENCE.
						case 0b001:	// FENCE.I
						{
							// NOTHING TODO.
							break;
						}
						UNDEF_INST(nPC);
					}
					break;
				}
				case 0b1110011: // ECALL, EBREAK, WFI, MRET, CSRRW, CSRRS, CSRRC, CSRRWI, CSRRSI, CSRRCI
				{
					doSystem(stInst, nPC);
					break;
				}

				UNDEF_INST(nPC);
			}
		}
		maRegs[0] = 0;

		if (MR_OK != eMR) // Memory fault.
		{
			// Memory failure.
		}
		return false;
	}
};


CpuCore* CreateCore()
{
	return new RV32Core();
}