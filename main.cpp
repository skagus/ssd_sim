
#include <stdio.h>
#include <stdlib.h>
#include "core.h"
#include "uart.h"

#define BASE_SRAM	(0x80000000)
#define BASE_UART	(0x20000000)

int main(int argc, char* argv[])
{
	CpuCore* pstCore = CreateCore();
	uint32 nMemSize = 0;
	pstCore->Init(BASE_SRAM);

	/*
	* beq_bne_loop.bin
	* bltu.bin
	* lb_sb.in
	* lh_sh.bin
	* lw_sw_offset.bin
	* memory.bin
	*
	*/

	// FILE* fpBin = fopen(argv[1], "rb");
	FILE* fpBin = fopen("../test_c/baremetal.bin", "rb");
	//	FILE* fpBin = fopen("../test/memory.bin", "rb");

	fseek(fpBin, 0, SEEK_END);
	uint32 nBinSize = ftell(fpBin);
	nMemSize = nBinSize + 0x10000;
	fseek(fpBin, 0, SEEK_SET);
	uint8* pCodeMem = (uint8*)malloc(nMemSize);
	fread(pCodeMem, 1, nBinSize, fpBin);
	fclose(fpBin);
	
	SRAM* pSRAM = new SRAM(BASE_SRAM, nMemSize, pCodeMem);
	pstCore->AddMemory(pSRAM);

	Memory* pUart = UART_CreateHW(BASE_UART);
	pstCore->AddMemory(pUart);

	while (true)
	{
		pstCore->Step();
	}
}
