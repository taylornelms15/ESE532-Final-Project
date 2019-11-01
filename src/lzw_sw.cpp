// lzwtester.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <stdint.h>
#include "lzw_sw.h"
///In implementation, this might be a 13-bit or 14-bit type
static uint16_t table[MAXCHUNKLENGTH][MAXCHARVAL];

/**
Compresses numElements bytes from the array marked "input", outputs them into "output"
Returns the number of output elements
*/
int lzwCompress(uint8_t* input, int numElements, uint16_t* output) {
	///index of the input element we're reading
	int iidx = 0;
	///index of the output element we're writing
	int oidx = 0;

    printf("numElements: %d\n", numElements);
	int curTableRow = input[iidx++];
	while (iidx < numElements) {
		uint8_t curChar = input[iidx++];
		uint16_t currentTableValue = table[curTableRow][curChar];
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

    printf("oidx: %d\n", oidx);
	return oidx;
	

}//lzwCompress




int lzw_init()
{
	//instantiate our code table with "none found"
	for (int i = 0; i < MAXCHUNKLENGTH; i++) {
		for (int j = 0; j < MAXCHARVAL; j++) {
			table[i][j] = NONEFOUND;
		}
	}

	//make input and output buffers, fill the former with random data
/*	uint8_t inbuffer[INFILESIZE];
	CodeType outbuffer[OUTFILESIZE];
	for (int i = 0; i < INFILESIZE; i++) {
		inbuffer[i] = (rand() % MAXCHARVAL);
	}

	int numOutput = lzwCompress(inbuffer, INFILESIZE, outbuffer);
*/

	return 0;
}

