#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "lzw_sw.h"
#include <string.h>

//in implementation, may be able to trim to smaller data type
static uint16_t table[MAXCHUNKLENGTH][MAXCHARVAL];

/**
 * Converts a buffer full of larger-than-byte values (here represented as shorts; logically, they are 13-bit values)
 * into a buffer of bytes.
 *
 * Additionally, creates a header for the whole packet, as per the specification in the project's Decoder.cpp file
 * @param buffer Buffer full of LZW table values
 * @param output Location to which to write the finished LZW chunk
 * @param numElements How many LZW table values we're writing
 * @return Number of bytes for total LZW-encoded chunk
 */
int xferBufferToOutput(const uint16_t* buffer, uint8_t* output, int numElements){
    int outIndex = 4;//will later backfill indices 0-3 with the header information
    uint8_t boundary = 0;//The bitwise location we're writing to within a byte
    //Effectively, the 'bit index' we write to is (outIndex * 8 + boundary)

    for(int i = 0; i < numElements; i++){
        uint16_t val = buffer[i];//13-bit value to encode
        switch(boundary){
        case 0://two bytes, first byte is exact
            output[outIndex++] = (uint8_t)(val >> 5);
            output[outIndex] = (uint8_t)(val << 3);
            break;
        case 1:
        case 2://two bytes
            output[outIndex++] |= (uint8_t)(val >> (boundary + 5));
            output[outIndex] = (uint8_t)(val << (3 - boundary));
            break;
        case 3://two bytes, last byte is exact
            output[outIndex++] |= (uint8_t)(val >> 8);
            output[outIndex++] = (uint8_t)(val << 0);
            break;
        case 4:
        case 5:
        case 6:
        case 7:
        default://three bytes
            output[outIndex++] |= (uint8_t)(val >> (boundary + 5));
            output[outIndex++] = (uint8_t)(val >> ((boundary + 5) % 8));
            output[outIndex] = (uint8_t)(val << (11 - boundary));
            break;
        }//switch
        boundary = (boundary + 5) % 8;

    }//for each buffer entry

    if(boundary == 0) outIndex -= 1;

    uint32_t numOutput = outIndex + 1;
    uint32_t header = (numOutput - 4) << 1;//bit 0 is 0 because LZW chunk, the rest is the size of the data. Subtracting 4 to get "size of LZW" part sans header
    memcpy((void*) &output[0], &header, 1 * sizeof(uint32_t));//put header into the top of the output table


    return numOutput;

}//xferBufferToOutput

/**
Compresses numElements bytes from the array marked "input", outputs them into "output"
Returns the number of output bytes
*/
int lzwCompress(const uint8_t* input, int numElements, uint8_t* output) {
    uint16_t outBuffer[MAXCHUNKLENGTH];
    memset((void*) table, 0xFF, MAXCHUNKLENGTH * MAXCHARVAL * sizeof(uint16_t));//just set to all ones

	///index of the input element we're reading
	int iidx = 0;
	///index of the outBuffer element we're writing
	int oidx = 0;

	int curTableRow = input[iidx++];
	while (iidx < numElements) {
		uint8_t curChar = input[iidx++];
		uint16_t currentTableValue = table[curTableRow][curChar];
		if (currentTableValue != NONEFOUND){
			curTableRow = currentTableValue;
			if (iidx == numElements){//fixes a "missing last code" problem
				outBuffer[oidx++] = curTableRow;
				break;
			}
			continue;
		}
		else {
			outBuffer[oidx++] = curTableRow;
			table[curTableRow][curChar] = oidx + MAXCHARVAL - 1;
			curTableRow = curChar;//reset back to initial block
			if (iidx == numElements){//fixes a "missing last code" problem
				outBuffer[oidx++] = curTableRow;
				break;
			}
		}

	}

    //printf("oidx: %d\n", oidx);
    int bytesOutput = xferBufferToOutput(outBuffer, output, oidx);
    //printf("Ending bytes: %d\n", bytesOutput);
	return bytesOutput;
	

}//lzwCompress


