#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include “hw10.h”


/*****************************************
 * 2022 global variables
 ****************************************/
/*
 * Access to process
 */
typedef struct {
	char 			type;		// type of access I, L, S, M
	unsigned int	va;		// virtual address
	unsigned int	pa;		// phsical address
	unsigned int	frame;	// frame number
	unsigned int	page;		// page number
	unsigned int	l1Index; 	// index into L1
	unsigned int	l1Tag;   	// tag bits for L1
	unsigned int	l2Index; 	// l2 index 
	unsigned int	l2Tag;   	// tag bits for L2 
	unsigned int	offset;	// page offset
	int 			bytes;	// number of bytes in access
} access;

// create current access for processing
access	currAcc;

/*********************
 * Cache struct - used for L1I, L1D & L2
 ********************/
typedef struct {
	char		p;	// present bit
	char		d;	// dirty bit
	unsigned int	tag;	// logical or physical
} cacheline;

// instantiate both L1 caches
cacheline L1I[L1SIZE];		// L1 instruction cache
cacheline L1D[L1SIZE];		// L1 data cache

// instantiate L2 cache
cacheline L2[SETS][L2SIZE];	// L2 cache

/*********************
 * TLB struct 	&&&&&&& not used until stage 3
 ********************/
typedef struct {
	char		p;	// present
	unsigned int	page;	// page number
	unsigned int	frame;	// frame number
} tlb;

tlb TLB[TLBSIZE];

/*********************
 * Page Table struct &&&&&&&& not used until stage 3
 ********************/
typedef struct {
	char		p;	// present
	char		d;	// dirty frame
	unsigned int	frame;	// frame number
} pt;

pt PT[PTSIZE];

/*********************
 * Statistics
 ********************/
typedef struct {
	int		try;	// # of trys
	int		hit;	// # of hits
	int		miss;	// number of misses
} stats;

stats l1IStat;		// L1 I cache
stats l1DStat;		// L1 D cache
stats l2Stat;		// L2 cache
stats tlbStat;		// TLB		&&&& not used until later stage
stats ptStat;		// page table	"	""	"
stats bufferStat;	// hdd buffer	"	"	"

/*********************
 * Metrics
 ********************/
typedef struct {
	int	clock;		// global clock
	int	memAccess;	// number of memory accesses
	int	streamAccess;	// number of input lines processed
	int	multAccess;	// number of multiple accesses
	int	l1wb;		// number of L1 write backs
	int	l2wb;		// number of L2 write backs
	int	l1wbfound;	// number of L1 write backs found
	int	pagesLoaded;	// number of pages loaded &&&& used later
	int	pfCount;	// number of page faults	"	"
} metrics;

metrics sM;

/*********************
 * Controls
 ********************/
typedef struct {
	int	tlbPtr;		// points to next tlb entry 
	int	ptNextFree;	// next free frame
	int	lastSector;	// last sector on HDD accessed
	int	memPtr;		// next frame to be filled
	int	buffSector;	// sector number in buffer
} controls;

controls sC;

/*****************************************
 * Prototypes
 ****************************************/
void initialize();
void processAccess();
void parseCacheInfo();
int numXfers();
void l1Access();
void l2Access();
void l2UpdateDirty(unsigned int, unsigned int);
void report();




/*****************************************
 * print current access
 ****************************************/

void printAccess() {
	printf("Line Number = %d\n", sM.streamAccess);
	printf("type = %c\n", currAcc.type);
	printf("bytes = %d\n", currAcc.bytes);
	printf("va = 0x%x\n", currAcc.va);
	printf("pa = 0x%x\n", currAcc.pa);
	printf("page = 0x%x\n", currAcc.page);
	printf("frame = 0x%x\n", currAcc.frame);
	printf("offset = 0x%x\n", currAcc.offset);
	printf("l1 index = 0x%x\n", currAcc.l1Index);
	printf("l1 tag = 0x%x\n", currAcc.l1Tag);
	printf("l2 index = 0x%x\n", currAcc.l2Index);
	printf("l2 tag = 0x%x\n\n", currAcc.l2Tag);
} // end print access data

/****************************
 * Process the current access
 * check structures in progression
 ****************************/

void processAccess() {
	
	int prevClock = sM.clock;
	int delta;

	// if stage 1, just increment clock & return
	if (STAGE == 1) {
		// add 1 or 2 mem access times
		sM.clock = sM.clock + MEMCLOCKS;
		if (currAcc.type == 'M')
			sM.clock = sM.clock + MEMCLOCKS;
	} // end stage 1
	else {
		if(currAcc.type == 'M') {
			// perform read then convert to write
			currAcc.type = 'L';
			l1Access();
			currAcc.type = 'S';
		} // end M special case

		// Call L1 cache - if miss, it will call others
		l1Access();

	} // end stage 2 or greater
	
	if (TEST_CLKS) {
		delta = sM.clock - prevClock;
		printf("clocks = %d delta = %d\n", sM.clock, delta);
	}
} // end processAccess


/******************************** 
 * take one line of input and parse to type, va and bytes
 * we will use currAcc to process one access at a time
 *******************************/

void parseInput(char *str1) {
	char *address1;
	char *address2;
	address1 = str1 + 3;
	address2 = str1 + 12;

	// store access type
	if(str1[0] == 'I')
		currAcc.type = str1[0];
	else
		currAcc.type = str1[1];
	
	// convert va to the hex virtual address
	currAcc.va = strtoul(address1, NULL, 16);

	// parse cache index & tag for L1 & L2
	parseCacheInfo();
	
	// store the number of bytes
	currAcc.bytes = atoi(address2);

	if (TEST_PARSE) 
		printAccess();
}

/**************************** 
 * Main routine
 ***************************/
int main(int argc, char **argv) {

	// variables
	int count;
	char str[MAXCHAR];	// input access string
	FILE *fp;

	/*************************************
	 * intialize structures based on stage
	 *************************************/ 
	initialize();

	/************************************
	 * Open stream file or files
	 ************************************/

	// if no argumments - use default path
	if (argc == 1) {
		// open file
		char* filename = "./address1.txt";
		fp = fopen(filename, "r");
		if (fp == NULL) {
			printf("Couldn't Open File %s\n", filename);
			return 1;
		}

	} // end of default file

	// if > 1 file, give an error message & exit
	if (argc > 2) {
		printf("Too many files specified - limit = 1\n");
		return 1;
	} // end of too many files

	// file name specified
	if (argc == 2) {
		printf("READ NAMED FILE - argc = %d\n", argc);
		// printf("File name = %s\n", argv[1]);
		char* filename = argv[2];
		fp = fopen(argv[1], "r");
		if (fp == 0) {
			printf("Could not open file\n");
		} // end open files
	} //

	/*******************************************
	 * read line of access from current process
	 *******************************************/
 
	// read in accesses from file
	while (fgets(str,MAXCHAR, fp) != NULL) {
		if (TEST_READLN)
			printf("%s", str);

		sM.streamAccess++;

		// parse line and store in currentAddress
		parseInput(str);


		// determine number of transfers
		int xfers = numXfers();
		// printf("Number of transfers = %d\n", xfers);

		// perform the first transer
		processAccess();
		xfers--;
		// printf("print first\n");

		// if multiple accesses required
		while(xfers > 0) {
			currAcc.va = currAcc.va + BUSWIDTH;
			parseCacheInfo();
			processAccess();
			sM.multAccess++;
			xfers--;
			// printf("print access %d xfers\n", xfers);
		} // end of transfer loop

		if (TEST_PROCLN)
			printf("clocks = %d from %d accesses\n\n", sM.clock, count);
	} // end of while
	
	// close file
	fclose(fp);


	/****************************
	 * print resuts to file
	 ***************************/

	report();

	// print total clocks
	printf("\nSTAGE = %d\n", STAGE);
	printf("Access = %d, Multiple = %d\n", sM.streamAccess, sM.multAccess);
	printf("Total clocks = %'d\n\n", sM.clock);

	return 0;
} // end main


/**************************************
 * Determine the index and tag fields for the
 * current access for L1 & L2
 * store in the currAcc structure
 *************************************/

void parseCacheInfo() {

	unsigned int index;	// temp value
	unsigned int tag;	// temp value

	// set L1 index & tag fields for current access
	index = currAcc.va >> OFFSETL1I;
	currAcc.l1Index = index & MASKL1I;
	tag = currAcc.va >> OFFSETL1T;
	currAcc.l1Tag = tag & MASKL1T;
	
	// set L2 index & tag fields for current access
	/**********************
	 * WRITE CODE TO DEFINE VALUES FOR L2
	 *********************/

} // end parseCacheInfo

/**************************************
 * If L1D p&d set but tag miss, need to write
 * back to L2 & set dirty bit.
 *************************************/

void l2UpdateDirty(unsigned int oldTag, unsigned int oldIndex) {

	// local variables used to abreviate code
	unsigned int oldAddr = 0;


	unsigned int index = 0; 	// temp values
	unsigned int tag = 0;		// temp values
	unsigned int old = oldTag >> 2;
	int found = 0;			// flag to show if found
	int foundSet = 5;		// if found, what set

	// rebuild address in L1
	oldAddr = oldTag << OFFSETL1T;
	oldAddr = (oldAddr | (oldIndex << OFFSETL1I));
	// reparse old address to L2 index & tag
	index = oldAddr >> OFFSETL2I;
	index = index & MASKL2I;
	tag = oldAddr >> OFFSETL2T;
	tag = tag & MASKL2T;

	// check each of the 4 sets
	for(int i=0; i<SETS; i++) {
		if((L2[i][index].p == 1) && (L2[i][index].tag == old)) {
			L2[i][index].d = 1;
			sM.l1wbfound++;
			found = 1;
			foundSet = i;
		} // end dirty update
	} // end of sets check

	if(found == 0) {
		printf("\noldAddr = 0x%x oldTag = 0x%x, index = 0x%x\n", oldAddr, tag, index);
		printf("StreamAccess = %d\n", sM.streamAccess);
		for(int i=0; i<SETS; i++) {
			printf("set = %d,    ", i);
			printf("curTag = 0x%x\n", L2[i][index].tag);
		}
		
		// if not found, write to memory		
		sM.clock = sM.clock + MEMCLOCKS;
		sM.memAccess++;
	}

	sM.clock = sM.clock + L2CLOCKS;

} // end update dirty
	
/**************************************
 * Test if L2 contains the information
 * If so, update the clock, statistics and return.
 * If not a hit, access memory & update cache
 *************************************/

void l2Access() {

	// local variables used to abreviate code
	unsigned int index = currAcc.l2Index; 
	unsigned int tag = currAcc.l2Tag;
	int hit = 0;
	int hitSet = 0;
	// data for replacement algorithm
	int nextSet = 0;
	int emptySet = -1;
	int cleanSet = -1;

	// increment L2 try counter
	l2Stat.try++;

	// try all 4 sets
	for(int i=0; i<SETS; i++) {
		/**********************
		 * WRITE CODE TO DETERMINE IF THERE IS A HIT & WHICH SET HITS
		 *********************/	
		} // end found a hit

		/**********************
		 * EXPLAIN THE REPLACEMENT ALGORITHM (WRITTEN PROBLEM)
		 *********************/	
		// test p & d bits
		// select the highext priority
		if((L2[i][index].p == 1) && (L2[i][index].d == 0))
			cleanSet = i;
		if(L2[i][index].p == 0) 
			emptySet = i;

	} // end try sets	

	// determine the next set to write into if a miss
	// look for p=0 & d=0, then p=1 & d=0
	// if not found then pick next set 
	if(emptySet >= 0)
		nextSet = emptySet;
	else
		if(cleanSet >= 0)
			nextSet = cleanSet;

	// if hit
	if (hit == 1) {
		/**********************
		 * WRITE CODE TO SIMULATE A HIT
		 *********************/	
	} // end of hit		
	else {
		// if miss, determine if replacement set is dirty
		// l2setPtr points to next set to be replaced on a miss
		if((L2[nextSet][index].p == 1) && (L2[nextSet][index].d == 1)) {
			// need to write old line to memory
			sM.l2wb++;
			sM.memAccess++;
			sM.clock = sM.clock + MEMCLOCKS;
		} // end write dirty line

		// update next set with tag & control info
		L2[nextSet][index].tag = tag;
		L2[nextSet][index].p = 1;
		if(currAcc.type == 'S')
			L2[nextSet][index].d = 1;
		 
		// if miss, access memory
		/**********************
		 * WRITE CODE TO SIMULATE A MISS
		 *********************/	

	} // end miss

	// add clocks for L2 access
	sM.clock = sM.clock + L2CLOCKS;

} // end l2Access
	
/**************************************
 * Test if L1 (I or D) contains the information
 * If so, update the clock, statistics and return.
 * If not a hit, attempt an access to L2 and update L1
 *************************************/

void l1Access() {

	// local variables used to abreviate code
	unsigned int index = currAcc.l1Index; 
	unsigned int tag = currAcc.l1Tag;
	
	// process I & D caches separately
	if(currAcc.type == 'I') { // I cache
		// increment try
		l1IStat.try++;

		/**********************
		 * WRITE CODE TO DETERMINE L1I HIT/MISS
		 * SIMULATE L1I FOR BOTH CASES
		 *********************/	
	} // end I cache access
	else {			// D cache
		// increment try
		l1DStat.try++;
		
		// check if miiss - p bit = 0 or tags don't match
		if((L1D[index].p == 0) || (L1D[index].tag != tag)) {
			/**********************
			 * WRITE CODE TO DETERMINE IF WRITE BACK TO
			 * L2 IS REQUIRED. IF SO CALL l2UpdateDirty()
			 *********************/	

			l2Access();

			// fill in tag & p bit
			L1D[index].tag = tag;
			L1D[index].p = 1;

			l1DStat.miss++;
		} // end of L1 D miss
		else {		// L1D cache hit.
		/**********************
		 * WRITE CODE TO SIMULATE L1D HIT
		 *********************/	
		} // end of L1 D hit
		
	} // end D cache access

	// increment clock
	sM.clock = sM.clock + L1CLOCKS;

} // end l1Access

/**************************************
 * Determine the number of memory accesses
 * required to perform the stream access
 * return the number to main()
 *************************************/

int numXfers() {

	int bytePtr = 0;	// points to first byte
	int mask = 0;		// mask off upper bits
	int lastByte = 0;	// last byte to be written
	int xfers = 0;		// number of transfers required
	int span = 0;		// start + bytes

	// if Stage 1, then bus is 4 bytes wide (32-bits)
	if (STAGE == 1)	
		mask = 0x03;	// 4-byte bus / memory
	else 
		/**********************
		 * DETERMINE VALUE OF THE MASK
		 *********************/	
		mask = 0x0;	// 32-byte bus / memory &&&&&& modify.
	
	// point at byte (in this case 0-3)
	bytePtr = currAcc.va & mask;

	// cacluate number of transfers
	xfers = 1;
	span = bytePtr + currAcc.bytes;
	if(span > BUSWIDTH) {
		xfers = span / BUSWIDTH;
		if (span % BUSWIDTH != 0)
			xfers++;
	}
	
	return xfers;

} // end numXfers

/****************************
 * Initialize structures, metrics, statistics
 ****************************/

void initialize() {
	// initialize statistics
	l1IStat.try = 0;
	l1IStat.hit = 0;
	l1IStat.miss = 0;
	l1DStat.try = 0;
	l1DStat.hit = 0;
	l1DStat.miss = 0;
	l2Stat.try = 0;
	l2Stat.hit = 0;
	l2Stat.miss = 0;
	tlbStat.try = 0;
	tlbStat.hit = 0;
	tlbStat.miss = 0;
	ptStat.try = 0;
	ptStat.hit = 0;
	ptStat.miss = 0;
	bufferStat.try = 0;
	bufferStat.hit = 0;
	bufferStat.miss = 0;

	// initialize metrics
	sM.clock = 0;
	sM.memAccess = 0;
	sM.streamAccess = 0;
	sM.multAccess = 0;
	sM.l1wb = 0;
	sM.l2wb = 0;
	sM.pagesLoaded = 0;
	sM.pfCount = 0;

	// initialize controls
	sC.tlbPtr = 0;
	sC.ptNextFree = 0;
	sC.lastSector = -1;
	sC.memPtr = PTSIZE * 4;
	sC.buffSector = -1;

	// intialize caches, TLB & PT
	for(int i=0; i<L1SIZE; i++) {
		L1I[i].p = 0;
		L1D[i].p = 0;
	}
	for(int i=0; i<SETS; i++) 
		for(int j=0; j<L2SIZE; j++) 
			L2[i][j].p = 0;
	for(int i=0; i<TLBSIZE; i++) 
		TLB[i].p = 0;
	for(int i=0; i<PTSIZE; i++) 
		PT[i].p = 0;

}


/****************************
 * print out statistics
 ****************************/

void report() {

	// print out results
	FILE *fpOut;
	fpOut = fopen("Results.txt", "w");
	if(fpOut == NULL) {
		printf("Error opening output file\n");
		exit(1);
	}

	// print out results of simulation
	setlocale(LC_NUMERIC, "");
	fprintf(fpOut, "Total clocks ' %'d \n\n", sM.clock);

	int try;
	int hit;
	int miss;

	// L1I hit rate
	try = l1IStat.try;
	hit = l1IStat.hit;
	miss = l1IStat.miss;
	float L1IHR = (hit / (float) try) * 100;
	fprintf(fpOut, "L1I try = %d, miss = %d, hit = %d\n", try, miss, hit);
	fprintf(fpOut, "L1 I hit rate = %.2f%% \n\n", L1IHR);

	// L1D hit rate
	try = l1DStat.try;
	hit = l1DStat.hit;
	miss = l1DStat.miss;
	float L1DHR = (hit / (float) try) * 100;
	fprintf(fpOut, "L1D try = %d, miss = %d, hit = %d\n", try, miss, hit);
	fprintf(fpOut, "L1 write backs = %d\n", sM.l1wb);
	fprintf(fpOut, "L1 write backs found = %d\n", sM.l1wbfound);
	fprintf(fpOut, "L1 D hit rate = %.2f%% \n\n", L1DHR);

	// L2 hit rate
	try = l2Stat.try;
	hit = l2Stat.hit;
	miss = l2Stat.miss;
	float L2HR = (hit / (float) try) * 100;
	fprintf(fpOut, "L2 try = %d, miss = %d, hit = %d\n", try, miss, hit);
	fprintf(fpOut, "L2 write backs = %d\n", sM.l2wb);
	fprintf(fpOut, "L2 hit rate = %.2f%% \n\n", L2HR);

	// memory
	fprintf(fpOut, "Memory accesses = %d\n", sM.memAccess);

	fclose(fpOut);
}

doug@dougVM:~/Desktop$ 
