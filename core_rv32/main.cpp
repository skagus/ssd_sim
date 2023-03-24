// Copyright 2022 Charles Lohr, you may use this file or any portions herein under any of the BSD, MIT, or CC0 licenses.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "default64mbdtc.h"
#include "peri.h"
#include "rv32ima.h"

// Just default RAM amount is 64MB.
uint32_t gnSizeRAM = 64*1024*1024;
int fail_on_all_faults = 0;

int64_t SimpleReadNumberInt( const char * number, int64_t defaultNumber );
uint32_t HandleException( uint32_t ir, uint32_t retval );
void HandleOtherCSRWrite( uint8_t * image, uint16_t csrno, uint32_t value );



uint8_t * ram_image = 0;
struct MiniRV32IMAState * gstCoreRegs;
static void DumpState(struct MiniRV32IMAState* gstCoreRegs, uint8_t * ram_image);


int main( int argc, char ** argv )
{
	int i;
	long long instct = -1;
	int bShowHelp = 0;
	int time_divisor = 1;
	int fixed_update = 0;
	int do_sleep = 1;
	int single_step = 0;
	int dtb_ptr = 0;
	const char * image_file_name = 0;
	const char * dtb_file_name = 0;
	for( i = 1; i < argc; i++ )
	{
		const char * param = argv[i];
		int param_continue = 0; // Can combine parameters, like -lpt x
		do
		{
			if( param[0] == '-' || param_continue )
			{
				switch( param[1] )
				{
					case 'm':
					{
						if (++i < argc)
						{
							gnSizeRAM = SimpleReadNumberInt(argv[i], gnSizeRAM);
						}
						break;
					}
					case 'c':
					{
						if (++i < argc)
						{
							instct = SimpleReadNumberInt(argv[i], -1);
						}
						break;
					}
					case 'f':
					{
						image_file_name = (++i < argc) ? argv[i] : 0;
						break;
					}
					case 'b':
					{
						dtb_file_name = (++i < argc) ? argv[i] : 0;
						break;
					}
					case 'l':
					{
						param_continue = 1;
						fixed_update = 1;
						break;
					}
					case 'p':
					{
						param_continue = 1;
						do_sleep = 0;
						break;
					}
					case 's':
					{
						param_continue = 1;
						single_step = 1;
						break;
					}
					case 'd':
					{
						param_continue = 1;
						fail_on_all_faults = 1;
						break;
					}
					case 't':
					{
						if (++i < argc)
						{
							time_divisor = SimpleReadNumberInt(argv[i], 1);
						}
						break;
					}
					default:
					{
						if (param_continue)
							param_continue = 0;
						else
							bShowHelp = 1;
						break;
					}
				}
			}
			else
			{
				bShowHelp = 1;
				break;
			}
			param++;
		} while( param_continue );
	}
	if( bShowHelp || image_file_name == 0 || time_divisor <= 0 )
	{
		fprintf( stderr,
				"./mini-rv32imaf [parameters]\n"\
					"\t-m [ram amount]\n"\
					"\t - f[running image]\n"\
					"\t - b[dtb file, or 'disable']\n"\
					"\t - c instruction count\n"\
					"\t - s single step with full processor state\n"\
					"\t - t time divion base\n"\
					"\t - l lock time base to instruction count\n"\
					"\t - p disable sleep when wfi\n"\
					"\t - d fail out immediately on all faults\n" );
		return 1;
	}

	ram_image = (uint8_t*)malloc( gnSizeRAM );
	if( !ram_image )
	{
		fprintf( stderr, "Error: could not allocate system image.\n" );
		return -4;
	}

restart:
	{
		FILE * f = fopen( image_file_name, "rb" );
		if( !f || ferror( f ) )
		{
			fprintf( stderr, "Error: \"%s\" not found\n", image_file_name );
			return -5;
		}
		fseek( f, 0, SEEK_END );
		long flen = ftell( f );
		fseek( f, 0, SEEK_SET );
		if( flen > gnSizeRAM )
		{
			fprintf( stderr, "Error: Could not fit RAM image (%ld bytes) into %d\n", flen, gnSizeRAM );
			return -6;
		}

		memset( ram_image, 0, gnSizeRAM );
		if( fread( ram_image, flen, 1, f ) != 1)
		{
			fprintf( stderr, "Error: Could not load image.\n" );
			return -7;
		}
		fclose( f );

		if( dtb_file_name )
		{
			if( strcmp( dtb_file_name, "disable" ) == 0 )
			{
				// No DTB reading.
			}
			else
			{
				f = fopen( dtb_file_name, "rb" );
				if( !f || ferror( f ) )
				{
					fprintf( stderr, "Error: \"%s\" not found\n", dtb_file_name );
					return -5;
				}
				fseek( f, 0, SEEK_END );
				long dtblen = ftell( f );
				fseek( f, 0, SEEK_SET );
				dtb_ptr = gnSizeRAM - dtblen - sizeof( struct MiniRV32IMAState );
				if( fread( ram_image + dtb_ptr, dtblen, 1, f ) != 1 )
				{
					fprintf( stderr, "Error: Could not open dtb \"%s\"\n", dtb_file_name );
					return -9;
				}
				fclose( f );
			}
		}
		else
		{
			// Load a default dtb.
			dtb_ptr = gnSizeRAM - sizeof(default64mbdtb) - sizeof( struct MiniRV32IMAState );
			memcpy( ram_image + dtb_ptr, default64mbdtb, sizeof( default64mbdtb ) );
		}

	}

	CaptureKeyboardInput();

	// The gstCoreRegs lives at the end of RAM.
	gstCoreRegs = (struct MiniRV32IMAState *)(ram_image + gnSizeRAM - sizeof( struct MiniRV32IMAState ));
	gstCoreRegs->pc = MINIRV32_RAM_IMAGE_OFFSET;
	gstCoreRegs->aReg[10] = 0x00; //hart ID
	gstCoreRegs->aReg[11] = dtb_ptr?(dtb_ptr+MINIRV32_RAM_IMAGE_OFFSET):0; //dtb_pa (Must be valid pointer) (Should be pointer to dtb)
	gstCoreRegs->extraflags |= 3; // Machine-mode.

	if( dtb_file_name == 0 )
	{
		// Update system ram size in DTB (but if and only if we're using the default DTB)
		// Warning - this will need to be updated if the skeleton DTB is ever modified.
		uint32_t * dtb = (uint32_t*)(ram_image + dtb_ptr);
		if( dtb[0x13c/4] == 0x00c0ff03 )
		{
			uint32_t validram = dtb_ptr;
			dtb[0x13c/4] = (validram>>24) | ((( validram >> 16 ) & 0xff) << 8 ) | (((validram>>8) & 0xff ) << 16 ) | ( ( validram & 0xff) << 24 );
		}
	}

	// Image is loaded.
	uint64_t rt;
	uint64_t lastTime = (fixed_update)?0:(GetTimeMicroseconds()/time_divisor);
	int instrs_per_flip = single_step?1:1024;
	for( rt = 0; rt < instct+1 || instct < 0; rt += instrs_per_flip )
	{
		uint64_t * this_ccount = ((uint64_t*)&gstCoreRegs->cyclel);
		uint32_t elapsedUs = 0;
		if( fixed_update )
			elapsedUs = *this_ccount / time_divisor - lastTime;
		else
			elapsedUs = GetTimeMicroseconds()/time_divisor - lastTime;
		lastTime += elapsedUs;

		if( single_step )
			DumpState( gstCoreRegs, ram_image);

		int ret = MiniRV32IMAStep( gstCoreRegs, ram_image, 0, elapsedUs, instrs_per_flip ); // Execute upto 1024 cycles before breaking out.
		switch( ret )
		{
			case 0: break;
			case 1:
			{
				if (do_sleep)
				{
					MiniSleep();
				}
				*this_ccount += instrs_per_flip;
				break;
			}
			case 3:
			{
				instct = 0;
				break;
			}
			case 0x7777:
			{
				goto restart;	//syscon code for restart
			}
			case 0x5555:
			{
				printf("POWEROFF@0x%08x%08x\n", gstCoreRegs->cycleh, gstCoreRegs->cyclel);
				return 0; //syscon code for power-off
			}
			default:
			{
				printf("Unknown failure\n");
				break;
			}
		}
	}

	DumpState( gstCoreRegs, ram_image);
}

//////////////////////////////////////////////////////////////////////////
// Rest of functions functionality
//////////////////////////////////////////////////////////////////////////



int64_t SimpleReadNumberInt( const char * number, int64_t defaultNumber )
{
	if (number && number[0])
	{
		int radix = 10;
		if (number[0] == '0')
		{
			char nc = number[1];
			number += 2;
			if (nc == 0) return 0;
			else if (nc == 'x') radix = 16;
			else if (nc == 'b') radix = 2;
			else { number--; radix = 8; }
		}
		char* endptr;
		uint64_t ret = strtoll(number, &endptr, radix);
		if (endptr != number)
		{
			return ret;
		}
	}
	return defaultNumber;
}

void DumpState( struct MiniRV32IMAState * gstCoreRegs, uint8_t * ram_image )
{
	uint32_t pc = gstCoreRegs->pc;
	uint32_t pc_offset = pc - MINIRV32_RAM_IMAGE_OFFSET;
	uint32_t ir = 0;

	printf( "PC: %08x ", pc );
	if( pc_offset >= 0 && pc_offset < gnSizeRAM - 3 )
	{
		ir = *((uint32_t*)(&((uint8_t*)ram_image)[pc_offset]));
		printf( "[0x%08x] ", ir ); 
	}
	else
		printf( "[xxxxxxxxxx] " ); 
	uint32_t * aReg = gstCoreRegs->aReg;
	printf( "Z:%08x ra:%08x sp:%08x gp:%08x tp:%08x t0:%08x t1:%08x t2:%08x s0:%08x s1:%08x a0:%08x a1:%08x a2:%08x a3:%08x a4:%08x a5:%08x ",
		aReg[0], aReg[1], aReg[2], aReg[3], aReg[4], aReg[5], aReg[6], aReg[7],
		aReg[8], aReg[9], aReg[10], aReg[11], aReg[12], aReg[13], aReg[14], aReg[15] );
	printf( "a6:%08x a7:%08x s2:%08x s3:%08x s4:%08x s5:%08x s6:%08x s7:%08x s8:%08x s9:%08x s10:%08x s11:%08x t3:%08x t4:%08x t5:%08x t6:%08x\n",
		aReg[16], aReg[17], aReg[18], aReg[19], aReg[20], aReg[21], aReg[22], aReg[23],
		aReg[24], aReg[25], aReg[26], aReg[27], aReg[28], aReg[29], aReg[30], aReg[31] );
}

