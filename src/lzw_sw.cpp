// lzwtester.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
///In implementation, this might be a 13-bit or 14-bit type
typedef uint16_t CodeType;
#define NONEFOUND 0x3fff
#define MAXCHUNKLENGTH (256)//will be 8k in reality, using smaller for testing
#define INFILESIZE (MAXCHUNKLENGTH)
#define OUTFILESIZE (MAXCHUNKLENGTH)
#define MAXCHARVAL (4)//will be 256 in actuality, using smaller for testing

static CodeType table[MAXCHUNKLENGTH][MAXCHARVAL];

/**
Compresses numElements bytes from the array marked "input", outputs them into "output"
Returns the number of output elements
*/
int lzwCompress(uint8_t* input, int numElements, CodeType* output) {
	///index of the input element we're reading
	int iidx = 0;
	///index of the output element we're writing
	int oidx = 0;

	int curTableRow = input[iidx++];
	while (iidx < numElements) {
		uint8_t curChar = input[iidx++];
		CodeType currentTableValue = table[curTableRow][curChar];
		if (currentTableValue != NONEFOUND){
			curTableRow = currentTableValue;
			continue;
		}
		else {
			output[oidx++] = curTableRow;
			table[curTableRow][curChar] = oidx + MAXCHARVAL - 1;
			curTableRow = curChar;//reset back to initial block
		}

	}

	return oidx;
	

}//lzwCompress




int main()
{
	srand(0xbadbad12);
	//instantiate our code table with "none found"
	for (int i = 0; i < MAXCHUNKLENGTH; i++) {
		for (int j = 0; j < MAXCHARVAL; j++) {
			table[i][j] = NONEFOUND;
		}
	}

	//make input and output buffers, fill the former with random data
	uint8_t inbuffer[INFILESIZE];
	CodeType outbuffer[OUTFILESIZE];
	for (int i = 0; i < INFILESIZE; i++) {
		inbuffer[i] = (rand() % MAXCHARVAL);
	}

	int numOutput = lzwCompress(inbuffer, INFILESIZE, outbuffer);


	return 0;
}
