/**
 * @file hardwareWrapper.h
 * @author Taylor Nelms
 */

#ifndef HARDWARE_WRAPPER_H
#define HARDWARE_WRAPPER_H

#include "common.h"

#define INBUFFER_SIZE 1000000 //1MB incoming buffer
#define OUTBUFFER_SIZE (INBUFFER_SIZE)


#ifndef CHUNKDICT_H//when we integrate that branch, these parameters will already be included
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
#endif

/**
 * Takes in a buffer full of data, streams it through our processing, and fills an output buffer
 * Will want to make this the top-level function when synthesizing our FPGA things
 *
 * @param input Input buffer from which we're processing
 * @param output Output buffer to which we're writing
 * @param tableLocation Location in memory for the chunk-ID table (for deduplication)
 * @return Number of bytes written by this iteration of processing
 */
#pragma SDS data copy(input[0:INBUFFER_SIZE])
#pragma SDS data copy(output)
#pragma SDS data zero_copy(tableLocation[0:SHA256TABLESIZE])
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL, tableLocation:RANDOM)
#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS, tableLocation:NON_PHYSICAL_CONTIGUOUS)
uint32_t processBuffer(uint8_t input[INBUFFER_SIZE], uint8_t output[OUTBUFFER_SIZE], uint8_t tableLocation[SHA256TABLESIZE]);




















#endif//HARDWARE_WRAPPER_H
