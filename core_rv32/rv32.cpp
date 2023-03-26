#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "macro.h"
#include "rv32.h"

#define INC_PC()			(mnPC += 4)
#define UN_SUPPORTED()		assert(false)
#define CASE_UNDEFINED()	default:{UN_SUPPORTED();break;}

class RV32Core: public CpuCore
{
private:
	uint32 mnPC;
	uint32 maRegs[NUM_GP];
	uint32 maCSR[NUM_CSR];

	uint8 mnCntMem;
	Memory* maChunk[MAX_MEM_CHUNK];

	uint32 mnTagedAddr;	///< Tag for atommic.

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
				else if(0b0100000 == stInst.R.nFunExt)// SRA : Shift Right Arithmatic(��ȣ ��Ʈ�� �츮�鼭)
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
		uint32 nImm = stInst.I.nImm;
		if (nImm & 0x800) nImm |= 0xFFFFF000;
		switch (stInst.I.nFun)
		{
			case 0b000: // ADDI.
			{
				maRegs[stInst.I.nRd] = (int32)maRegs[stInst.I.nRs1] + (int32)nImm;
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
				maRegs[stInst.I.nRd] = (int32)maRegs[stInst.I.nRs1] < (int32)nImm;
				break;
			}
			case 0b011: // SLTIU
			{
				maRegs[stInst.I.nRd] = maRegs[stInst.I.nRs1] < nImm;
				break;
			}
			case 0b100:	// XORI
			{
				maRegs[stInst.I.nRd] = maRegs[stInst.I.nRs1] ^ nImm;
				break;
			}
			case 0b110:	// ORI
			{
				maRegs[stInst.I.nRd] = maRegs[stInst.I.nRs1] | nImm;
				break;
			}
			case 0b111: // ANDI.
			{
				maRegs[stInst.I.nRd] = maRegs[stInst.I.nRs1] & nImm;
				break;
			}
			CASE_UNDEFINED();
		}
	}

	void doBranch(CmdFormat stInst, uint32 nPC)
	{
//		int32 nImm = stInst.S.nImm0 + (stInst.S.nImm1 << 5);
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
				if (maRegs[stInst.S.nRs1] == maRegs[stInst.S.nRs2]) mnPC = nPC + (int32)nImm;
				break;
			}
			case 0b001:	// BNE
			{
				if (maRegs[stInst.S.nRs1] != maRegs[stInst.S.nRs2]) mnPC = nPC + (int32)nImm;
				break;
			}
			case 0b100:	// BLT
			{
				if ((int32)maRegs[stInst.S.nRs1] < (int32)maRegs[stInst.S.nRs2]) mnPC = nPC + (int32)nImm;
				break;
			}
			case 0b101:	// BGE
			{
				if ((int32)maRegs[stInst.S.nRs1] >= (int32)maRegs[stInst.S.nRs2]) mnPC = nPC + (int32)nImm;
				break;
			}
			case 0b110:	// BLTU : Less than.
			{
				if (maRegs[stInst.S.nRs1] < maRegs[stInst.S.nRs2]) mnPC = nPC + (int32)nImm;
				break;
			}
			case 0b111:	// BGEU : Bigger or Equal Unsigned.
			{
				if (maRegs[stInst.S.nRs1] >= maRegs[stInst.S.nRs2]) mnPC = nPC + (int32)nImm;
				break;
			}
			CASE_UNDEFINED();
		}
	}

	/**
	* A extention.
	* �ΰ� ����.
	* Load Reserved�� load�� �����ѵ�, reservation�� ����Ѵٴ� ���� �ְ�, 
	* Store Conditional�� reserved�� memory�� �ٸ� ��ɿ� ���ؼ� update�Ǿ��ٸ�, updata�� �����Ѵ�.
	* ���� ������ tag���� �ɾ���Ƽ� update���� �ʾ����� �����ϰ� ���ٵ�, 
	* resource���� ������, memory��ü�� ���ؼ� 1���� tagging�� �ϰų�, 
	* memory ������ ����� ������ tagging�� �ϴ� ����� ���⵵ �Ѵ�. 
	* 
	* �� ������ multi coreȯ�濡�� �ǹ��ִ� ������, single core������ ���� �����ѵ�..
	*/
	void doAtomic(CmdFormat stInst)
	{
		uint32 nVal;
		uint32 nAddrS1 = maRegs[stInst.R.nRs1];

		static uint32 nResevedAddr;

		switch (stInst.R.nFunExt)
		{
			case 0b00010:	// LR.W : Load Reserved.
			{
				nResevedAddr = nAddrS1;
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = nVal;
				break;
			}
			case 0b00011:	// SC.W : Store Conditional.
			{
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
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = maRegs[stInst.R.nRs2];
				maRegs[stInst.R.nRs2] = nVal;
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			case 0b00000:	// ADD.W
			{
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = nVal + maRegs[stInst.R.nRs2];
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			case 0b00100:	// XOR.W
			{
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = nVal ^ maRegs[stInst.R.nRs2];
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			case 0b01100:	// AND.W
			{
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = nVal & maRegs[stInst.R.nRs2];
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			case 0b01000:	// OR.W
			{
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = nVal | maRegs[stInst.R.nRs2];
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			case 0b10000:	// MIN.W
			{
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = MIN((int32)nVal, (int32)maRegs[stInst.R.nRs2]);
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			case 0b10100:	// MAX.W
			{
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = MAX((int32)nVal, (int32)maRegs[stInst.R.nRs2]);
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			case 0b11000:	// MINU.W
			{
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = MIN(nVal, maRegs[stInst.R.nRs2]);
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			case 0b11100:	// MAXU.W
			{
				load(nAddrS1, &nVal);
				maRegs[stInst.R.nRd] = MAX(nVal, maRegs[stInst.R.nRs2]);
				store(nAddrS1, maRegs[stInst.R.nRd]);
				break;
			}
			CASE_UNDEFINED();
		}
		maRegs[stInst.R.nRd] = 0;
	}

	void doSystem(CmdFormat stInst)
	{
		switch (stInst.I.nFun)
		{
			case 0b000: // ECALL, EBREAK,
			{
				if (0 == stInst.I.nImm) // ECALL
				{
					if (0x2A == maRegs[GP_A0]) // 42
					{
						printf("\n\tSuccess\n\n");
						exit(0);
					}
					printf("\t\tFAIL with %d\n", maRegs[GP_GP]);
					ASSERT(false);
				}	
				else  // EBREAK
				{
				}
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
		memset(maRegs, 0, sizeof(maRegs));

		mnPC = nStart;
		maCSR[mcycle] = 0;
	}

	void AddMemory(Memory* pMem)
	{
		maChunk[mnCntMem] = pMem;
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
//			printf("0x%08X: %8X \n", nPC, stInst.nRaw);

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
					mnPC = nPC + (int32)(nImm20 * 2);	// �ּҴ� 2B aligned.
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
					ASSERT(0b010 == stInst.R.nFun);
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
		maRegs[0] = 0;

		if (MR_OK != eMR) // Memory fault.
		{
			// Memory failure.
		}
	}
};


CpuCore* CreateCore()
{
	return new RV32Core();
}