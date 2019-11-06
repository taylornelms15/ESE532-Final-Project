/**
@file lzw_hw.c
@author Taylor Nelms
*/

#include "lzw_hw.h"


static uint16_t table[MAXCHUNKLENGTH][MAXCHARVAL];


int lzwCompress(const uint8_t* input, int numElements, uint8_t* output){


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


