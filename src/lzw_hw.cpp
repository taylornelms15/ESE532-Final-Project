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

static const uint8_t rindBox[256] = {
  //0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 };




//ap_uint<13> valTable[MAXCHUNKLENGTH];//should split into multiple BRAM's...? (2 of them, ish)

//static uint16_t table[MAXCHUNKLENGTH][MAXCHARVAL];

/*
Hash approach:
34-bit value pairs (key(21) plus val(13))
2 in each line of BRAM
2 BRAM's for 2-deep hash table (1024 entries total, 68 bits of value)
Target: hold 8k entries, so 4 2-deep tables
But, hash function not great, budget for 24k entries
So, 12 2-deep tables, meaning 24 BRAMs for it
*/
ap_uint<68> hashTable [12][1024];//2 wide, 1024 tall, 12 deep
//val1and2 = hashTable[0][hashVal]; val3and4 = hashTable[1][hashVal]; etc.
//will want to use table of "valid" bits for quick table-resetting
//with 4k max chunk length, can use uint64_t, cut down to half the number of tables, meaning one valid-byte could cover a whole set of things


/**
 * Hash function to reduce a 20- or 21-bit key down to a 10-bit hash row index
 * Across a couple of empirical tests, this is not uniform, but allowed for a factor-of-3 difference between ideal hash and max number of elements in the buckets
 * As such, a factor of around 3 for the actual hash table should be able to hold most/all of the required values,
 * with a smaller associative memory handling the overflow
 */
ap_uint<10> hashKey(uint32_t key){


    uint8_t vlo4    = (key)         & 0xF;
    uint8_t lo4     = (key >> 4)    & 0xF;
    uint8_t hi4     = (key >> 8)    & 0xF;
    uint8_t vhi4    = (key >> 12)   & 0xF;
    uint8_t vvhi5   = (key >> 16)   & 0x1F;
    uint8_t sub8    = rindBox[((vvhi5 << 3) ^ vlo4)];
    uint8_t hiT4    = vhi4 ^ vlo4;
    uint8_t midT4   = (vlo4 ^ hi4 ^ vvhi5) & 0xF;
    uint8_t loT4    = lo4 ^ hi4;
    uint16_t retval = 0;
    retval          ^= loT4;
    retval          ^= (midT4 << 3);
    retval          ^= (sub8 << 1);
    retval          ^= (hiT4 << 6);
    retval          &= 0x3FF;
    return (ap_uint<10>) retval;

}//hashKey

void writeToTable(const ap_uint<13> row, const uint8_t col, const ap_uint<13> val){

}//writeToTable

/**
 * Reads a value from the LZW table
 * If not found, returns 0x1FFF
 */
ap_uint<13> readFromTable(const ap_uint<13> row, const uint8_t col){
    uint32_t key = ((uint32_t)(row) << 8) | col;
    ap_uint<10> hashedKey = hashKey(key);

    //TODO: add in the actual reading of values


    return (ap_uint<13>) 0;
}//readFromTable



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
        uint16_t currentTableValue = readFromTable(curTableRow, curChar);
        //uint16_t currentTableValue = table[curTableRow][curChar];
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
            ap_uint<13> valToWrite = oidx + MAXCHARVAL - 1;
            writeToTable(curTableRow, curChar, valToWrite);
            //table[curTableRow][curChar] = valToWrite;
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


