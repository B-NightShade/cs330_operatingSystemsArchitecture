#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

// constant values
#define MAXCHAR 20

// Test flags to print status
#define TEST_PARSE 0		// test parse function
#define TEST_READLN 0		// test read line
#define TEST_MULTIBYTE 0	// test multibyte analysis
#define TEST_PROCLN 0		// test processed lines
#define TEST_CLKS 0		// test clocks assigned
#define TEST_L1 0		// test L1 cache
#define TEST_L2 0		// test L2 cache
// #define TEST_TLB 0		// test TLB
// #define TEST_PT 0		// Test page table
// #define TEST_PF 0		// Test page fault
// #define TEST_HD 0		// Test HDD & buffer

/******************
 * Student defined size, mask & shift values
 * @@@@@@@@@@@@
 *****************/
#define L1SIZE 0		// L1 number of lines
#define L2SIZE 0		// L2 lines per set
#define SETS 0		// Number of sets in L2
#define TLBSIZE 0	// TLB number of lines
#define PTSIZE 0		//

#define MASKL1I 0x0	// L1 index mask <x:x> x bits
#define MASKL1T 0x0	// L1 tag mask <x:x> x bits
#define MASKL2I 0x0	// L2 index mask <x:x> x bits
#define MASKL2T 0x0	// L2 tag mask <x:x> x bits
// #define MASKOFFSET 0x0	// page mask

// shift values
#define OFFSETL1I 0	// L1 index offset <x:x>
#define OFFSETL1T 0	// L1 tag offset <x:x>
#define OFFSETL2I 0	// L2 index offset <x:x>
#define OFFSETL2T 0	// L2 tag offset <x:x>
// #define OFFSETPage 0	// page offset 4k page

/******************
 * 2022 constants
 *****************/
#define STAGE 2 
#define MAXPROC 1
#define L1CLOCKS 0
#define L2CLOCKS 0
#define TLBCLOCKS 15 
#define MEMCLOCKS 0
#define BUFFERCLOCKS 400
#define HDDCLOCKS 5000
#define BUSWIDTH 0		// number of bytes wide

/******************
 * End of Student defined size, mask & shift values
 * @@@@@@@@@@@@
 *****************/

