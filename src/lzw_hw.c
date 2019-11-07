/**
@file lzw_hw.c
@author Taylor Nelms
*/

#include "lzw_hw.h"
#include "ap_int.h"

/*
Thoughts about space:
8k total table rows (13b)
256 total table cols (8b)
So, 21b address needed, matching to a 13b value

21b address needs to reduce down to the 13b "where is my value" index

Match BRAM: need 3 (one per 9b of key) times 114 (1 per 72 entries) BRAM's

Value BRAM: 8k entries, each 13b wide
 */

//conceptually:
//ap_uint<8192> matchTable_Top[128];//top 7 bits of key
//ap_uint<8192> matchTable_Med[128];//med 7 bits of key
//ap_uint<8192> matchTable_Low[128];//low 7 bits of key
//
//for matchTable_Top:
//one BRAM can hold 4 sets of "rows" (512/128)
//brings it down to ap_uint<2048> matchTable_TopMod[512]
//so, given key `k`, matchTable_Top[k] = matchTable_TopMod[k] | (matchTable_TopMod[k + 128] << 2048) | (matchTable_TopMod[k + 256] << 4096) |  (matchTable_TopMod[k + 384] << 6144); 

ap_uint<13> valTable[MAXCHUNKLENGTH];//should split into multiple BRAM's...? (2 of them, ish)


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


