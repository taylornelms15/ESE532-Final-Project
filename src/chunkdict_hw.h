/**
 * @file chunkdict.h
 * @author Taylor Nelms
 */
#ifndef CHUNKDICT_H
#define CHUNKDICT_H

#include "common.h"
#define SHA256_SIZE 32//32-byte digest
#define BYTES_PER_ROW (SHA256_SIZE + 4)
#define INDEXBITS 17
#define HASHROWS 1024
#define HASHBITS 16
#define HASHDEPTH 2
#define SHANOTFOUND 0x1FFFF
#define NUM_ENTRIES_PER_HASH_VALUE ((1 << (INDEXBITS - HASHBITS)) * HASHDEPTH)
#define DRAM_PULL_SIZE (BYTES_PER_ROW * NUM_ENTRIES_PER_HASH_VALUE)
#define NUMHASHBUCKETS (1 << HASHBITS)
#define SHA256TABLESIZE (NUMHASHBUCKETs * DRAM_PULL_SIZE)


#pragma SDS data zero_copy(tableLocation[0:SHA256TABLESIZE])
#pragma SDS data copy(input[0:SHA256_SIZE])
#pragma SDS data access_pattern(input:RANDOM, tableLocation:RANDOM)
int indexForShaVal_HW(const uint8_t input[SHA256_SIZE], uint8_t* tableLocation);



void resetTable_HW(uint8_t* tableLocation);



#endif
