/**
 * @file chunkdict.c
 * @author Taylor Nelms
 * Stand-in to represent a "hash table" storing the order in which we output LZW-compressed chunks
 */

#include "chunkdict.h"
#include <string.h>
#define MAX_CHUNK_NUM (1024)//may want to modify where/how this is declared

typedef struct ShaVal{

    BYTE sha_buf[SHA256_BLOCK_SIZE];

} ShaVal;

static ShaVal table[MAX_CHUNK_NUM];//table storing our SHA256 hash values
static int curIndex = 0;//current max index for the table

//If we go a c++ way, could operator overload; likely not necessary
bool shaValEqual(const ShaVal* a, const ShaVal* b){

    bool retval = true;
    for(unsigned char i = 0; i < SHA256_BLOCK_SIZE; i++){
       if (a->sha_buf[i] != b->sha_buf[i]) retval = false; 
    }//for
    return retval;

}//equality for SHA256 values

/**
 * Searches for the hash value in our "table", albeit VERY naively
 * If it is not found, it is added to the table, and we return -1
 * Otherwise, we return the index at which we found it
 * 
 * @param input SHA256 value for which to search
 * @return Index of the found value, or -1 if not found
 */
int indexForShaVal(const BYTE input[SHA256_BLOCK_SIZE]){

    ShaVal compare;
    memcpy(compare.sha_buf, input, SHA256_BLOCK_SIZE);

    int foundIndex = -1;
    for(int i = 0; i < curIndex; i++){
        if (shaValEqual(&table[i], &compare)){
            foundIndex = i;
            break;
        }
    }//for

    if(foundIndex == -1){
       table[curIndex++] = compare; 
    }//if


    return foundIndex;

}//indexForShaVal

/**
 * Resets our table, making it effectively empty. (Just sets the max index back to 0)
 */
void resetTable(){
    curIndex = 0;
}//resetTable
