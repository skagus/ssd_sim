#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "rv32.h"

#define ASSERT(x)			assert(x)
#define INC_PC()			(mnPC += 4)
#define UN_SUPPORTED()		assert(false)
#define CASE_UNDEFINED()	default:{UN_SUPPORTED();break;}

class RV32Core
{
private:
	uint32 mnPC;
	uint32 maRegs[NUM_GP];
	uint32 maCSR[NUM_CSR];

	uint8 mnCntMem;
	MemChunk maChunk[MAX_MEM_CHUNK];

	template <typename T>
	MemRet load(uint32 nAddr, T* pnVal)
	{
		MemRet eRet;
		for (uint32 nChunk = 0; nChunk < mnCntMem; nChunk++)
		{
			eRet = maChunk[nChunk].Load(nAddr, pnVal);
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
			eRet = maChunk[nChunk].Store(nAddr, nVal);
			if (MR_OUT_RANGE != eRet)
			{
				return eRet;
			}
		}
		return MR_OUT_RANGE;
	}

	void doArith(CmdFormat stInst)
	{
		switch (stInst.R.nFun)
		{
			case 0b000: // ADD, SUB, MUL
			{
				if (0 == stInst.R.nFunExt)  // ADD
				{
					maRegs[stInst.R.nRd] = (int32)maRegs[stInst.R.nRs1] + (int32)maRegs[stInst.R.nRs2];
				}
				else if(0b100000 == stInst.R.nFunExt) // SUB
				{
					maRegs[stInst.R.nRd] = (int32)maRegs[stInst.R.nRs1] - (int32)maRegs[stInst.R.nRs2];
				}
				else if (0b01 == stInst.R.nFunExt)	// MUL, M-ext
				{
					maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs1] * maRegs[stInst.R.nRs2];
				}
				break;
			}
			case 0b001:	// SLL, MULH
			{
				if (0 == stInst.R.nFunExt) // SLL : Shift Left Logical.
				{
					maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs1] << maRegs[stInst.R.nRs2];
				}
				else if (0b01 == stInst.R.nFunExt) // MULH, M-ext
				{
					ASSERT(false);
					maRegs[stInst.R.nRd] = ((int64)(int32)maRegs[stInst.R.nRs1] * (int64)(int32)maRegs[stInst.R.nRs2]) >> 32;
				}
				break;
			}
			case 0b010:	// SLT, MULHSU.
			{
				if (0b00 == stInst.R.nFunExt) // SLT : Set Less Than.
				{
					maRegs[stInst.R.nRd] = ((int)maRegs[stInst.R.nRs1] < (int)maRegs[stInst.R.nRs2]) ? 1 : 0;
				}
				else if (0b01 == stInst.R.nFunExt) // MULHSU, M-ext
				{
					maRegs[stInst.R.nRd] = ((int64)(int32)maRegs[stInst.R.nRs1] * (uint64)maRegs[stInst.R.nRs2]) >> 32;
				}
				break;
			}
			case 0b011:	// SLTU : Set Less Than Unsigned.
			{
				if (0b00 == stInst.R.nFunExt) // SLTU
				{
					maRegs[stInst.R.nRd] = (maRegs[stInst.R.nRs1] < maRegs[stInst.R.nRs2]) ? 1 : 0;
				}
				else if (0b01 == stInst.R.nFunExt) // MULHU, M-ext
				{
					maRegs[stInst.R.nRd] = ((uint64)maRegs[stInst.R.nRs1] * (uint64)maRegs[stInst.R.nRs2]) >> 32;
				}
				break;
			}
			case 0b100:	// XOR
			{
				if (0b00 == stInst.R.nFunExt) // XOR
				{
					maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs1] ^ maRegs[stInst.R.nRs2];
				}
				else if (0b01 == stInst.R.nFunExt) // DIV, M-ext
				{
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
					maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs1] >> maRegs[stInst.R.nRs2];
				}
				else if(0b0100000 == stInst.R.nFunExt)// SRA : Shift Right Arithmatic(부호 비트를 살리면서)
				{
					maRegs[stInst.R.nRd] = (int32)maRegs[stInst.R.nRs1] >> (int32)maRegs[stInst.R.nRs2];
				}
				else if (0b01 == stInst.R.nFunExt) // DIVU, M-ext
				{
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
					maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs1] | maRegs[stInst.R.nRs2];
				}
				else if (0b01 == stInst.R.nFunExt) // REM, M-ext
				{
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
					maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs1] & maRegs[stInst.R.nRs2];
				}
				else if (0b01 == stInst.R.nFunExt) // REMU, M-ext
				{
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
			CASE_UNDEFINED();
		}
	}

	void doArithImm(CmdFormat stInst)
	{
		switch (stInst.I.nFun)
		{
			case 0b000: // ADDI.
			{
				uint32 nVal = stInst.I.nImm;
				if (nVal & 0x800) nVal |= 0xFFFFF000;
				maRegs[stInst.I.nRd] = (int32)maRegs[stInst.I.nRs1] + (int32)nVal;
				break;
			}
			case 0b001:	// SLLI
			{
				uint32 nShamt = stInst.R.nRs2;
				maRegs[stInst.I.nRd] = maRegs[stInst.I.nRs1] << nShamt;
				break;
			}
			case 0b101: // SRLI, SRAI
			{
				uint32 nShamt = stInst.R.nRs2;
				if (0 == stInst.R.nFunExt) // SRLI.
				{
					maRegs[stInst.I.nRd] = maRegs[stInst.I.nRs1] >> nShamt;
				}
				else // SRAI.
				{
					maRegs[stInst.I.nRd] = (int32)maRegs[stInst.I.nRs1] >> nShamt;
				}
				break;
			}
			case 0b010: // SLTI
			{
				maRegs[stInst.I.nRd] = (int32_t)maRegs[stInst.I.nRs1] < (int32_t)stInst.I.nImm;
				break;
			}
			case 0b011: // SLTIU
			{
				maRegs[stInst.I.nRd] = maRegs[stInst.I.nRs1] < stInst.I.nImm;
				break;
			}
			case 0b100:	// XORI
			{
				maRegs[stInst.I.nRd] = maRegs[stInst.I.nRs1] ^ stInst.I.nImm;
				break;
			}
			case 0b110:	// ORI
			{
				maRegs[stInst.I.nRd] = maRegs[stInst.I.nRs1] | stInst.I.nImm;
				break;
			}
			case 0b111: // ANDI.
			{
				maRegs[stInst.I.nRd] = maRegs[stInst.I.nRs1] & stInst.I.nImm;
				break;
			}
			CASE_UNDEFINED();
		}
	}

	void doBranch(CmdFormat stInst, uint32 nPC)
	{
		int32 nImm = stInst.S.nImm0 + (stInst.S.nImm1 << 5);
		if (nImm & 0x800)
		{
			nImm = (uint32)nImm | 0xFFFFF000;
		}
		switch (stInst.S.nFun)
		{
			case 0b000:	// BEQ
			{
				if (maRegs[stInst.S.nRs1] == maRegs[stInst.S.nRs2]) mnPC = nPC + nImm;
				break;
			}
			case 0b001:	// BNE
			{
				if (maRegs[stInst.S.nRs1] != maRegs[stInst.S.nRs2]) mnPC = nPC + nImm;
				break;
			}
			case 0b100:	// BLT
			{
				if ((int32)maRegs[stInst.S.nRs1] < (int32)maRegs[stInst.S.nRs2]) mnPC = nPC + nImm;
				break;
			}
			case 0b101:	// BGE
			{
				if ((int32)maRegs[stInst.S.nRs1] >= (int32)maRegs[stInst.S.nRs2]) mnPC = nPC + nImm;
				break;
			}
			case 0b110:	// BLTU
			{
				if (maRegs[stInst.S.nRs1] >= maRegs[stInst.S.nRs2]) mnPC = nPC + nImm;
				break;
			}
			case 0b111:	// BGEU
			{
				if (maRegs[stInst.S.nRs1] >= maRegs[stInst.S.nRs2]) mnPC = nPC + nImm;
				break;
			}
			CASE_UNDEFINED();
		}
	}

	void doAtomic(CmdFormat stInst)
	{
		switch (stInst.R.nFunExt)
		{
			case 0b00010:	// LR.W
			{
				break;
			}
			case 0b00011:	// SC.W
			{
				break;
			}
			case 0b00001:	// SWAP.W
			{
				break;
			}
			case 0b00000:	// ADD.W
			{
				break;
			}
			case 0b00100:	// XOR.W
			{
				break;
			}
			case 0b01100:	// AND.W
			{
				break;
			}
			case 0b01000:	// OR.W
			{
				break;
			}
			case 0b10000:	// MIN.W
			{
				break;
			}
			case 0b10100:	// MAX.W
			{
				break;
			}
			case 0b11000:	// MINU.W
			{
				break;
			}
			case 0b11100:	// MAXU.W
			{
				break;
			}
			CASE_UNDEFINED();
		}
	}

	void doSystem(CmdFormat stInst)
	{
		switch (stInst.I.nFun)
		{
			case 0b000: // ECALL
			{
				if (0 == stInst.I.nImm) {}	// ECALL
				else {} // EBREAK
				break;
			}
			case 0b001:	// CSRRW: SWAP CSR value.
			{
				if (0 != stInst.I.nRd)
				{
					maRegs[stInst.I.nRd] = maCSR[stInst.I.nImm];
				}
				maCSR[stInst.I.nImm] = maRegs[stInst.I.nRs1];
				break;
			}
			case 0b010:	// CSRRS: Read and Set bit.
			{
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
				if (0 != maRegs[stInst.I.nRd])
				{
					maRegs[stInst.I.nRd] = maCSR[stInst.I.nImm];
				}
				maCSR[stInst.I.nImm] = stInst.I.nRs1;
				break;
			}
			case 0b110: // CSRRSI: 
			{
				if (0 != maRegs[stInst.I.nRd])
				{
					maRegs[stInst.I.nRd] = maCSR[stInst.I.nImm];
				}
				maCSR[stInst.I.nImm] |= stInst.I.nRs1;
				break;
			}
			case 0b111: // CSRRCI.
			{
				if (0 != maRegs[stInst.I.nRd])
				{
					maRegs[stInst.I.nRd] = maCSR[stInst.I.nImm];
				}
				maCSR[stInst.I.nImm] &= ~stInst.I.nRs1;
				break;
			}
			CASE_UNDEFINED();
		}
	}

public:
	void Init(uint32 nStart)
	{
		mnCntMem = 0;
		maRegs[0] = 0;
		mnPC = nStart;
		maCSR[mcycle] = 0;
	}

	void AddMemChunk(uint32 nBaseAddr, uint32 nSize, uint8* pChunk)
	{
		MemChunk* pMem = maChunk + mnCntMem;
		pMem->Init(nBaseAddr, nSize, pChunk);
		mnCntMem++;
	}

	void Step()
	{
		CmdFormat stInst;
		MemRet eMR = load(mnPC, &stInst.nRaw);
		maCSR[mcycle]++;
		if (MR_OK == eMR)
		{
			uint32 nPC = mnPC;
			INC_PC();
			printf("0x%08X: %8X \n", nPC, stInst.nRaw);

			switch (stInst.B.nOpc)
			{
				case 0b0010111: // AUIPC
				{
					maRegs[stInst.U.nRd] = nPC + (int32)(stInst.U.nImm << 12);
					break;
				}
				case 0b0110011:	// ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
				{
					doArith(stInst);
					break;
				}
				case 0b0010011:	// ADDI, SLLI, SRLI, SRAI, SLTI, SLTIU, XORI, ORI, ANDI.
				{
					doArithImm(stInst);
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
							eMR = store(nAddr, (uint8)nVal);
							break;
						}
						case 0b001:  // SH
						{
							eMR = store(nAddr, (uint16)nVal);
							break;
						}
						case 0b010:  // SW
						{
							eMR = store(nAddr, (uint32)nVal);
							break;
						}
						CASE_UNDEFINED();
					}
					break;
				}
				case 0b0000011:	// LB, LH, LW, LBU, LHU
				{
					uint32 nSrcAddr = maRegs[stInst.I.nRs1] + stInst.I.nImm;
					switch (stInst.I.nFun)
					{
						case 0b000:	// LB
						{
							uint8 nRead;
							eMR = load(nSrcAddr, &nRead);
							maRegs[stInst.I.nRd] = (int8)nRead;
							break;
						}
						case 0b001:	// LH
						{
							uint16 nRead;
							eMR = load(nSrcAddr, &nRead);
							maRegs[stInst.I.nRd] = (int16)nRead;
							break;
						}
						case 0b010:	// LW
						{
							uint32 nRead;
							eMR = load(nSrcAddr, &nRead);
							maRegs[stInst.I.nRd] = nRead;
							break;
						}
						case 0b100:	// LBU
						{
							uint8 nRead;
							eMR = load(nSrcAddr, &nRead);
							maRegs[stInst.I.nRd] = nRead;
							break;
						}
						case 0b101:	// LHU
						{
							uint16 nRead;
							eMR = load(nSrcAddr, &nRead);
							maRegs[stInst.I.nRd] = nRead;
							break;
						}
						CASE_UNDEFINED();
					}
					break;
				}
				case 0b0110111:	// LUI. Load Upper Imm.
				{
					maRegs[stInst.U.nRd] = stInst.U.nImm << 12;
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
					break;
				}
				case 0b1100111:	// JALR.
				{
					if (0 != stInst.I.nRd)
					{
						maRegs[stInst.I.nRd] = nPC + 4;
					}
					mnPC = maRegs[stInst.I.nRs1] + stInst.I.nImm;
					break;
				}
				case 0b1100011: // BEQ, BNE, BLT, BLTU, BGEU
				{
					doBranch(stInst, nPC);
					break;
				}
				case 0b0101111: // LR.W, SC.W, AMO{SWAP, ADD, XOR, AND, OR, MIN, MAX, MINU, MAXU}.W
				{
					ASSERT(0b101 == stInst.R.nFun);
					doAtomic(stInst);
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
						CASE_UNDEFINED();
					}
					break;
				}
				case 0b1110011: // ECALL, EBREAK, CSRRW, CSRRS, CSRRC, CSRRWI, CSRRSI, CSRRCI
				{
					doSystem(stInst);
					break;
				}

				CASE_UNDEFINED();
			}
		}

		if (MR_OK != eMR) // Memory fault.
		{
			// Memory failure.
		}
	}
};

#define BIN_BASE	0x80000000

int main(int argc, char* argv[])
{
	RV32Core stCore;
	uint32 nMemSize = 0;
	stCore.Init(BIN_BASE);

	FILE* fpBin = fopen("baremetal.bin", "rb");
	fseek(fpBin, 0, SEEK_END);
	uint32 nBinSize = ftell(fpBin);
	nMemSize = nBinSize + 0x10000;
	fseek(fpBin, 0, SEEK_SET);

	uint8* pCodeMem = (uint8*)malloc(nMemSize);
	fread(pCodeMem, 1, nBinSize, fpBin);
	fclose(fpBin);
	stCore.AddMemChunk(BIN_BASE, nMemSize, pCodeMem);

	while (true)
	{
		stCore.Step();
	}
}
