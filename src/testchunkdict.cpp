/**
 * @file testchunkdict.cpp
 * @author Taylor Nelms
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>


#ifdef __SDSCC__
#include <ff.h>
#include <sds_lib.h>
#endif

#include "common.h"
#include "chunkdict_hw.h"
extern "C"{
#include "chunkdict.h"
}




void fillRandom(uint8_t* dst, int size){
    for(int i = 0; i < size; i++){
    	uint8_t nextRand = (uint8_t) rand();
        dst[i] = nextRand;
    }
}

void pullFromTestSha(int index, const uint8_t* randomTable, uint8_t* dst){
	int offset = index * SHA256_SIZE;
	for(int i = 0; i < SHA256_SIZE; i++){
		dst[i] = randomTable[offset + i];
	}
}

#define NUMTESTSHA 256

int main(int argc, char* argv[]){
	srand(0xbad1bad2);
	printf("Starting\n");

    uint8_t* tableLocation = (uint8_t*) malloc(SHA256TABLESIZE);
    uint8_t* shaToPullFrom = (uint8_t*) malloc(SHA256_SIZE * NUMTESTSHA);
    printf("mallocs complete\n");
    fillRandom(shaToPullFrom, SHA256_SIZE * NUMTESTSHA);
    printf("fillRandom complete\n");
    resetTable_HW(tableLocation);
    printf("resetTableHW complete\n");
    resetTable();
    printf("resetTable complete\n");

    for(int i = 0; i < 1024; i++){
    	uint8_t thisSha[SHA256_SIZE];
    	int thisIndex = rand() % NUMTESTSHA;
    	pullFromTestSha(thisIndex, shaToPullFrom, thisSha);

    	printf("%d: Pulling from sha index %d\n", i, thisIndex);
    	int swIndex = indexForShaVal(thisSha);
    	int hwIndex = indexForShaVal_HW(thisSha, tableLocation);
    	printf("\tsw:%d\thw:%d\n", swIndex, hwIndex);
    	if (swIndex != hwIndex){
    		printf("For hash value at index %d, iteration %d, sw result %d differed from hw result %d\n");
    		return 1;
    	}//if

    }//for




    free(shaToPullFrom);
    free(tableLocation);
    return 0;
}//main
