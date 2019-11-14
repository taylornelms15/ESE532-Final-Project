/**
 * @file chunkdict_hw.cpp
 * @author Taylor Nelms
 */




#include "chunkdict.h"
#include "ap_uint.h"

#define INDEXBITS 17
#define HASHROWS 1024
#define HASHBITS 10
#define SHANOTFOUND 0x1FFFF

static uint32_t currentIndex = 0;


//SHA256_BLOCK_SIZE - number of bytes in our SHA block (32)

// Table in DRAM wants to be storing something like 200k "rows"
// Each row is holding a pair of "ShaValue, Index"
// But, can perhaps afford being a little sparse here
// If each bucket has an ideal number of values, can pull that many "rows" from an area on the main memory
// With an extra-capacity depth factor X, will need to expand by that factor
// Example: let's say we have 128 potential rows, each storing an 8-bit key and a 7-bit index
// If hash key down to 6 bits (64 buckets), ideal would be 2 rows per bucket
// If our hashed value were "3", would pull rows at indices 6 and 7
//
// On-chip memory... actually need not exist. It's just a hash function.
// We can define the depth we need (call it 2?), and then



// Hash table - up to 200k entries in it total (less than 2^18, I'm even willing to assume 2^17 for bit purposes)
// Each one is 256 + <hashbits> long
// But I guess what we really need is just a way to store a bunch of indexes.
// Because table never cleared, no real worry about "not starting from 0"
// Once we find our indexes, can check
// One BRAM is 512 rows each storing 2ish row indexes
// Assuming even hash buckets (not unreasonable, given SHA input): for each


void storeNewValue(const BYTE newVal[SHA256_BLOCK_SIZE]){

    uint8_t storeBuffer[SHA256_BLOCK_SIZE + 4];//stores the SHA value, plus 4 bytes for the current index
    memcpy(storeBuffer, newVal, SHA256_BLOCK_SIZE);
    memcpy(storeBuffer + SHA256_BLOCK_SIZE, &currentIndex, 4);

    currentIndex++;
}//storeNewValue


int indexForShaValue_HW(const BYTE input[SHA256_BLOCK_SIZE]){



}//indexForShaValue_HW

void resetTable_HW(){

}//resetTable_HW












