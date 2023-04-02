#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#define BIT(x)			(1 << (x))

#define SET_REG_4B(nAddr, nDW)		(*((unsigned int*)nAddr) = nDW)
#define GET_REG_4B(nAddr)			(*((unsigned int*)nAddr))
#define SET_REG_1B(nAddr, nDW)		(*((unsigned char*)nAddr) = nDW)
#define GET_REG_1B(nAddr)			(*((unsigned char*)nAddr))


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

#define TIMER_ENABLE			BIT(0)
#define TIMER_INT_EN			BIT(1)




char uart_rx()
{
	while(1)
	{
		if(GET_REG_4B(UART_STATUS) & UART_RX_RDY)
		{
			break;
		}
		asm volatile("wfi");
	}
	return (char)GET_REG_1B(UART_RX_BUF);
}

volatile void uart_tx(char nCh)
{
	while(1)
	{
		if( 0 == (GET_REG_4B(UART_STATUS) & UART_TX_BUSY))
		{
			break;
		}
		asm volatile("wfi");
	}
	SET_REG_1B(UART_TX_BUF, nCh);
}


static void uart_print(char* szString)
{
	while(*szString)
	{
		uart_tx(*szString);
		szString++;
	}
}

static void init_uart()
{
	SET_REG_4B(UART_CFG, 0x1);
}

void uart_gets(char* pBuf)
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

void init_timer()
{
	SET_REG_4B(TIMER_TICK_CLK, 10);
	SET_REG_4B(TIMER_PERIOD, 5);
	SET_REG_4B(TIMER_CFG, TIMER_ENABLE | TIMER_INT_EN);
}


// This is just a definition for a symbol found in the .S file.
void asm_demo_func();

// These will not turn into function calls, but instead will find a way
// of writing the assembly in-line
static void lprint(int nId, const char* s )
{
//	asm volatile( "ecall %[dst], %[src], 0x10\n": [dst]"=r"(s) : [src]"r"(s));
	asm volatile("ecall");
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
//	lprint(1, "TMR\n");
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
	init_timer();
	init_vectable();
	
	uart_print("Input some line\n");
	char aBuf[30];
	uart_gets(aBuf);
	uart_print(aBuf);
	
	uart_print("\nEnd get line\n");
	lprint(1, "\n");
	lprint(1, "Hello world from RV32 land.\n");
	lprint(1, "main is at: ");
	lprint(1, "\nAssembly code: ");
	asm_demo_func();
	lprint(1, "\n");

	// Wait a while.
	uint32_t cyclecount_initial = get_cyc_count();

	volatile int i;
	for( i = 0; i < 0xFF; i++ )
	{
		asm volatile( "nop" );
	}

	// Gather the wall-clock time and # of cycles
	uint32_t cyclecount = get_cyc_count() - cyclecount_initial;

	lprint(1, "Processor effective speed: ");
	lprint(1, " Mcyc/s\n");

	lprint(1, "\n");
	while(1);
}

