/**
 * @file chunkdict_hw.cpp
 * @author Taylor Nelms
 */

#include "chunkdict_hw.h"
#include "ap_int.h"

static uint32_t currentIndex = 0;

// Table in DRAM wants to be storing something like 200k "rows"
// Each row is holding a pair of "ShaValue, Index"
// But, can perhaps afford being a little sparse here
// If each bucket has an ideal number of values, can pull that many "rows" from an area on the main memory
// With an extra-capacity depth factor X, will need to expand by that factor
// Example: let's say we have 128 potential rows, each storing an 8-bit key and a 7-bit index
// If hash key down to 6 bits (64 buckets), ideal would be 2 rows per bucket
// If our hashed value were "3", would pull rows at indices 6 and 7


/**
 * Makes a hash of the incoming SHA256 value
 * Just does a series of XOR operations; assuming reasonable spread of the SHA256 values (roughly equal entropy for each bit)
 * If we like 16 as a number of hash bits, this equation can be simplified to do byte-level XOR operations, rather than bit-level (will undo a lot of compute load)
 */
ap_uint<HASHBITS> hashShaKey(const uint8_t input[SHA256_SIZE]){
    #pragma HLS inline

    ap_uint<HASHBITS> retval = 0;
    uint8_t currentHashBit  = 0;

    hashkeyloop:for(uint8_t i = 0; i < SHA256_SIZE; i++){
        #pragma HLS unroll
        uint8_t currentShaByte = input[i];
        for(uint8_t j = 0; j < 8; j++){
            ap_uint<HASHBITS> valToXor = 1 << currentHashBit;
            if (currentShaByte & (1 << j)){
                //do nothing
            }//if 1
            else{
                valToXor = 0;
            }//else 0
            retval ^= valToXor;        

            currentHashBit = (currentHashBit + 1) % HASHBITS;
        }//for j (per sha byte's bit)
    }//for i (per sha byte)

    return retval;
    

}//hashShaKey


/**
 * Gets the address offset on the DRAM side to look for our entries
 */
uint32_t getDramOffset(ap_uint<HASHBITS> hashVal){
    uint32_t rowNum = hashVal * ((uint32_t) NUM_ENTRIES_PER_HASH_VALUE);
    return rowNum * BYTES_PER_ROW;
}//getDramOffset

/**
 * Stand-ins for memcpy (relevant for synthesis)
 */
void memcpyDRAM(uint8_t dst[DRAM_PULL_SIZE], const uint8_t src[DRAM_PULL_SIZE]){
    #pragma HLS inline
    memcpydram:for(int i = 0; i < DRAM_PULL_SIZE; i++){
        #pragma HLS unroll
    	dst[i] = src[i];
    }
}
void memcpySha(uint8_t dst[SHA256_SIZE], const uint8_t src[SHA256_SIZE]){
    #pragma HLS inline
    memcpysha:for(int i = 0; i < SHA256_SIZE; i++){
        #pragma HLS unroll
        dst[i] = src[i];
    }
}
void memcpyRow(uint8_t dst[BYTES_PER_ROW], const uint8_t src[BYTES_PER_ROW]){
    #pragma HLS inline
    memcpyrow:for(int i = 0; i < BYTES_PER_ROW; i++){
        #pragma HLS unroll
    	dst[i] = src[i];
    }
}
void memcpy4F(uint8_t* dst, const uint32_t src){
	#pragma HLS inline
	dst[0] = (uint8_t) src >> 24;
	dst[1] = (uint8_t) src >> 16;
	dst[2] = (uint8_t) src >> 8;
	dst[3] = (uint8_t) src >> 0;
}
void memcpy4B(uint32_t* dst, const uint8_t* src){
	#pragma HLS inline
	*dst = (((uint32_t) src[0]) << 24) |
           (((uint32_t) src[1]) << 16) |
           (((uint32_t) src[2]) << 8) |
           (((uint32_t) src[3]) << 0);
}


void storeNewValue(const uint8_t newVal[SHA256_SIZE], uint8_t tableLocation[SHA256TABLESIZE], ap_uint<HASHBITS> key, uint8_t candidates[DRAM_PULL_SIZE]){
    #pragma HLS inline
    uint8_t storeBuffer[BYTES_PER_ROW];//stores the SHA value, plus 4 bytes for the current index
    #pragma HLS array_partition variable=storeBuffer
    memcpySha(storeBuffer, newVal);
    memcpy4F(storeBuffer + SHA256_SIZE, currentIndex);

    uint32_t offset = getDramOffset(key);

    int rowOffset = -1;
    findfreeindexloop:for (int i = 0; i < NUM_ENTRIES_PER_HASH_VALUE; i += BYTES_PER_ROW){
        #pragma HLS unroll
        uint32_t candIndex;
        uint8_t* indexPortion = &candidates[i + SHA256_SIZE];
        memcpy4B(&candIndex, indexPortion);
        if (candIndex >= SHANOTFOUND){
            rowOffset = i;
            break;
        }//found a free part of the table
    }//for each candidate
    if (rowOffset < 0){
        //TODO: handle a hash overflow here!!!!!
    }//if hash overflow

    uint32_t storageOffset = offset + rowOffset;
    memcpyRow(tableLocation + storageOffset, storeBuffer);

    currentIndex++;
}//storeNewValue

/**
 * Pulls all possible records of our hash-index pair from DRAM memory
 */
void pullRecordsFromTable(const uint8_t* tableLocation, uint8_t* destination, ap_uint<HASHBITS> key){
    #pragma HLS inline
    uint32_t offset = getDramOffset(key);

    memcpyDRAM(destination, tableLocation + offset);
}//pullRecordsFromTable

/**
 * @return 1 if buffers equal, 0 otherwise
 */
int shaRecordsEqual(const uint8_t record1[SHA256_SIZE], const uint8_t record2[SHA256_SIZE]){
    #pragma HLS inline
    int retval = 1;
    recordsequalloop:for(int i = 0; i < SHA256_SIZE; i++){
        #pragma HLS unroll
        if (record1[i] != record2[i]){
            retval = 0;
        }//if
    }//for

    return retval;
}//shaRecordsEqual

int indexForShaVal_HW(const uint8_t input[SHA256_SIZE], uint8_t tableLocation[SHA256TABLESIZE]){
    ap_uint<HASHBITS> key = hashShaKey(input);
    uint8_t candidates[DRAM_PULL_SIZE];

    pullRecordsFromTable(tableLocation, candidates, key);


    uint32_t indexVal = SHANOTFOUND;
    findequalrecordloop:for (int i = 0; i < NUM_ENTRIES_PER_HASH_VALUE; i += BYTES_PER_ROW){
        #pragma HLS unroll
        uint8_t* shaPortion = &candidates[i];
        uint8_t* indexPortion = &candidates[i + SHA256_SIZE];
        if (shaRecordsEqual(input, shaPortion)){
            memcpy4B(&indexVal, indexPortion);
        }//if
    }//for each candidate

    if (indexVal >= SHANOTFOUND){//should cover weird case where all-ones triggers weird stuff
        storeNewValue(input, tableLocation, key, candidates);

        return -1;
    }//if
    else{
        return indexVal;
    }//else



    return 0;
}//indexForShaValue_HW


/**
 * Likely unused
 */
void resetTable_HW(uint8_t tableLocation[SHA256TABLESIZE]){
    for (int i = 0; i < SHA256TABLESIZE; i++){
        tableLocation[i] = 0xFF;
    }
}//resetTable



