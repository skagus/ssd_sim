#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#define BASE_UART	(0x20000000)

static void print(char* szString)
{
	while(*szString)
	{
		*((unsigned char*)(BASE_UART + 4)) = *szString;
		szString++;
	}
}

static void init_uart()
{
	*((unsigned int*)(BASE_UART)) = 1;
}


static unsigned int getkey()
{
	return *((unsigned int*)(BASE_UART + 4));
}

void get_line(char* pBuf)
{
	while(1)
	{
		unsigned int nIn = getkey();
		if(('\r' == nIn) || ('\n' == nIn))
		{
			*pBuf = 0;
			break;
		}
		else if(nIn < 0xFF)
		{
			*pBuf = nIn;
			pBuf++;
		}
	}
}

// These are going to be bound to memory addresses in the linker script.
extern uint32_t SYSCON;
extern uint32_t TIMERL;



// This is just a definition for a symbol found in the .S file.
void asm_demo_func();

// These will not turn into function calls, but instead will find a way
// of writing the assembly in-line
static void lprint( const char * s )
{
	asm volatile( "csrrw x0, 0x138, %0\n" : : "r" (s));
}

static void pprint( intptr_t ptr )
{
	asm volatile( "csrrw x0, 0x137, %0\n" : : "r" (ptr));
}

static void nprint( intptr_t ptr )
{
	asm volatile( "csrrw x0, 0x136, %0\n" : : "r" (ptr));
}

static inline uint32_t get_cyc_count() {
	uint32_t ccount;
	asm volatile("csrr %0, 0xC00":"=r" (ccount));
	return ccount;
}

int main()
{
	init_uart();
	print("Test String\n");
	
	char aBuf[30];
	get_line(aBuf);
	print(aBuf);
	
	lprint("\n");
	lprint("Hello world from RV32 land.\n");
	lprint("main is at: ");
	pprint( (intptr_t)main );
	lprint("\nAssembly code: ");
	asm_demo_func();
	lprint("\n");

	// Wait a while.
	uint32_t cyclecount_initial = get_cyc_count();
	uint32_t timer_initial = TIMERL;

	volatile int i;
	for( i = 0; i < 0xFF; i++ )
	{
		asm volatile( "nop" );
	}

	// Gather the wall-clock time and # of cycles
	uint32_t cyclecount = get_cyc_count() - cyclecount_initial;
	uint32_t timer = TIMERL - timer_initial;

	lprint( "Processor effective speed: ");
	nprint( cyclecount / timer );
	lprint( " Mcyc/s\n");

	lprint("\n");
	while(1);
	
	SYSCON = 0x5555; // Power off
}

