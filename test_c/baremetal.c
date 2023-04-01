#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#define BASE_UART				(0x20000000)

#define UART_CFG				(BASE_UART + 0x4)
#define UART_STATUS				(BASE_UART + 0x4)
#define UART_RX_BUF				(BASE_UART + 0x8)
#define UART_TX_BUF				(BASE_UART + 0xC)

#define UART_RX_RDY		(1 << 0)
#define UART_TX_BUSY	(1 << 1)

#define BASE_TIMER				(0x20001000)
#define TIMER_CFG				(BASE_TIMER + 0x00)
#define TIMER_PERIOD			(BASE_TIMER + 0x04)
#define TIMER_CUR_VAL			(BASE_TIMER + 0x08)
#define TIMER_TICK_CLK			(BASE_TIMER + 0x0C)




char uart_rx()
{
	while(1)
	{
		if((*(unsigned int*)UART_STATUS) & UART_RX_RDY)
		{
			break;
		}
		asm volatile("wfi");
	}
	return (char)*((char*)UART_RX_BUF);
}

volatile void uart_tx(char nCh)
{
	while(1)
	{
		if( 0 == ((*(unsigned int*)UART_STATUS) & UART_TX_BUSY))
		{
			break;
		}
		asm volatile("wfi");
	}
	*((char*)UART_TX_BUF) = nCh;
}


static void print(char* szString)
{
	while(*szString)
	{
		uart_tx(*szString);
		szString++;
	}
}

static void init_uart()
{
	*((unsigned int*)(BASE_UART)) = 1;
}

void get_line(char* pBuf)
{
	while(1)
	{
		char nIn = uart_rx();
		if(('\r' == nIn) || ('\n' == nIn))
		{
			*pBuf = 0;
			break;
		}
		else
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
static void lprint(int nId, const char* s )
{
//	asm volatile( "ecall %[dst], %[src], 0x10\n": [dst]"=r"(s) : [src]"r"(s));
	asm volatile("ecall");
}

static void pprint( intptr_t ptr )
{
	asm volatile("csrrw x0, 0x138, %0\n" : : "r" (ptr));
}

static void nprint( intptr_t ptr )
{
	asm volatile("csrrw x0, 0x136, %0\n" : : "r" (ptr));
}

static inline uint32_t get_cyc_count() {
	uint32_t ccount;
	asm volatile("csrr %0, 0xC00":"=r" (ccount));
	return ccount;
}

volatile uint32_t gnTemp;
volatile uint32_t gnTemp2;
__attribute__((interrupt("machine"))) void ISR_Uart(void)
{
	uint32_t nTmp = gnTemp + gnTemp2 + 1;
	gnTemp = nTmp;
	return;
}

__attribute__((interrupt("machine"))) void ISR_Timer(void)
{
	uint32_t nTmp = gnTemp + gnTemp2 + 1;
	gnTemp = nTmp;
	return;
}

__attribute__((interrupt("machine"))) void ISR_Dummy(void)
{
	uint32_t nTmp = gnTemp + gnTemp2 + 1;
	gnTemp = nTmp;
	return;
}


void (*gaISR[])() =
{
	ISR_Uart,
	ISR_Timer,
	ISR_Dummy,
};

void init_vectable()
{
	register int pnVector;
	asm volatile ("la %[vec], %[sym]\n\t" :[vec]"=r"(pnVector):[sym]"i"(gaISR));
//	asm volatile ("ori %[dst], %[vec], 3\n\t" :[dst]"=r"(pnVector) : [vec]"r"(pnVector));
	asm volatile ("csrw mtvec, %[vec]\n\t" ::[vec]"r"(pnVector));

//	asm volatile (".insn r 0x33, 0, 0, %[res], %[res], %[ptr]":[res]"+r"(result) : [ptr]"r"(ptr));
}


int main()
{
	init_uart();
	init_vectable();
	print("Test String\n");
	
	char aBuf[30];
	get_line(aBuf);
	print(aBuf);
	
	print("\nEnd get line\n");
	lprint(1, "\n");
	lprint(1, "Hello world from RV32 land.\n");
	lprint(1, "main is at: ");
	pprint( (intptr_t)main );
	lprint(1, "\nAssembly code: ");
	asm_demo_func();
	lprint(1, "\n");

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

	lprint(1, "Processor effective speed: ");
	nprint( cyclecount / timer );
	lprint(1, " Mcyc/s\n");

	lprint(1, "\n");
	while(1);
	
	SYSCON = 0x5555; // Power off
}

