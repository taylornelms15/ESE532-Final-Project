/**
 * @file common.h
 * @author Taylor Nelms
 * A header file for access by multiple other source files, for various purposes
 */
#include <stdint.h>

#define HALF_ENABLE_CPP11_CMATH 0

#ifndef COMMON_H
#define COMMON_H

#define MINSIZE (1 * 1024)
#define MAXSIZE (6 * 1024)

#define MAXINPUTFILESIZE (200000000)//200MB
#define MAX_CHUNK_NUM (MAXINPUTFILESIZE / MINSIZE + 1)//may want to modify where/how this is declared

#define INBUFFER_SIZE 2000000 //2MB incoming buffer
#define OUTBUFFER_SIZE (INBUFFER_SIZE)

#define MAX_CHUNKS_IN_HW_BUFFER ((INBUFFER_SIZE + 1) / MINSIZE)

//Codes for use with 9-bit streams to denote the end of the chunk and/or the end of the file
#define ENDOFCHUNK  256
#define ENDOFFILE   257

//Chunk dictionary in HW
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
#define SHA256TABLESIZE (NUMHASHBUCKETS * DRAM_PULL_SIZE)





#endif
