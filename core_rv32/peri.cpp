#include <stdint.h>
#include <stdio.h>
#include "peri.h"



#include <windows.h>
#include <conio.h>

#define strtoll _strtoi64

void CaptureKeyboardInput()
{
	system(""); // Poorly documented tick: Enable VT100 Windows mode.
}

void ResetKeyboardInput()
{
}

void MiniSleep()
{
	Sleep(1);
}

uint64_t GetTimeMicroseconds()
{
	static LARGE_INTEGER lpf;
	LARGE_INTEGER li;

	if (!lpf.QuadPart)
		QueryPerformanceFrequency(&lpf);

	QueryPerformanceCounter(&li);
	return ((uint64_t)li.QuadPart * 1000000LL) / (uint64_t)lpf.QuadPart;
}


int IsKBHit()
{
	return _kbhit();
}

int ReadKBByte()
{
	// This code is kind of tricky, but used to convert windows arrow keys
	// to VT100 arrow keys.
	static int is_escape_sequence = 0;
	int r;
	if (is_escape_sequence == 1)
	{
		is_escape_sequence++;
		return '[';
	}

	r = _getch();

	if (is_escape_sequence)
	{
		is_escape_sequence = 0;
		switch (r)
		{
			case 'H': return 'A'; // Up
			case 'P': return 'B'; // Down
			case 'K': return 'D'; // Left
			case 'M': return 'C'; // Right
			case 'G': return 'H'; // Home
			case 'O': return 'F'; // End
			default: return r; // Unknown code.
		}
	}
	else
	{
		switch (r)
		{
			case 13: return 10; //cr->lf
			case 224: is_escape_sequence = 1; return 27; // Escape arrow keys
			default: return r;
		}
	}
}


uint32_t HandleControlStore(uint32_t addy, uint32_t val)
{
	if (addy == 0x10000000) //UART 8250 / 16550 Data Buffer
	{
		printf("%c", val);
		fflush(stdout);
	}
	return 0;
}


uint32_t HandleControlLoad(uint32_t addy)
{
	// Emulating a 8250 / 16550 UART
	if (addy == 0x10000005)
		return 0x60 | IsKBHit();
	else if (addy == 0x10000000 && IsKBHit())
		return ReadKBByte();
	return 0;
}