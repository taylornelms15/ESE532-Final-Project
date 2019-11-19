/**
 * @file hardwareWrapper.h
 * @author Taylor Nelms
 */

#ifndef HARDWARE_WRAPPER_H
#define HARDWARE_WRAPPER_H

#include "common.h"
#include "ap_int.h"
#include <hls_stream.h>

/**
 * Takes in a buffer full of data, streams it through our processing, and fills an output buffer
 * Will want to make this the top-level function when synthesizing our FPGA things
 *
 * @param input Input buffer from which we're processing
 * @param output Output buffer to which we're writing
 * @param tableLocation Location in memory for the chunk-ID table (for deduplication)
 * @param numElements Number of elements we'll read if we don't read the whole buffer (only relevant for last section)
 * @return Number of bytes written by this iteration of processing
 */
#pragma SDS data copy(input[0:INBUFFER_SIZE])
#pragma SDS data copy(output)//not sure of length, trying to avoid errors because we did not write enough
#pragma SDS data zero_copy(tableLocation[0:SHA256TABLESIZE])
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL, tableLocation:RANDOM)
#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS, tableLocation:NON_PHYSICAL_CONTIGUOUS)
uint32_t processBuffer(uint8_t input[INBUFFER_SIZE], uint8_t output[OUTBUFFER_SIZE], 
                       uint8_t tableLocation[SHA256TABLESIZE], uint32_t numElements);

// INTERFACE FORMATTING
/*
 * This section is an attempt to formalize the sort of arguments we will want from our hardware functions
 * Hopefully, this will provide a good roadmap for how we integrate pieces together
 */

/**
 * This should take in a stream of data and output two streams of data.
 * The input stream will be terminated with ENDOFFILE, which will happen after no more than INBUFFER_SIZE elements
 * The output streams will be the input stream, with ENDOFCHUNK values interspersed in between the chunks
 * Notably, we will want to make sure we don't divide the chunks in such a way as to leave a too-small chunk at the end
 * As such, if possible, we would like to make an arbitrary split, if we can, towards the end of INBUFFER_SIZE
 * We lose a bit of veracity for chunk deduplication, but gain a hell of a lot in not having to deal with leftover data complexity
 */
//rabin_hw(readerToRabin, rabinToSHA, rabinToLZW);


/**
 * This should take in a stream of chunk data, delimited by an ENDOFCHUNK value
 * It will output 32-byte packets of the resultant SHA digest
 * It can return once it hits an ENDOFFILE value
 */
//sha_hw(rabinToSHA, shaToDeduplicate);


/**
 * This will move data from one stream to another, in a loop, until it hits ENDOFFILE
 * It will not need to keep track of how much data it is outputting
 */
//lzw_hw(rabinToLZW, lzwToDeduplicate);


/**
 * Combining a few operations into this one function may be tricky
 * Conceptually, it will, until it receives some value from one of its streams 
 *     (likely an ENDOFFILE, not sure which stream will send it),
 * read in 32 bytes from the SHA stream, and as many bytes until it hits ENDOFCHUNK from lzw (likely into a local buffer of some kind)
 * It will compare the SHA bytes to the table in DRAM at tableLocation
 * That comparison will tell it whether to output the length of the LZW's output, and then the LZW itself,
 * or if it will output a packet with the SHA's index (found in tableLocation)
 */
//deduplicate_hw(shaToDeduplicate, lzwToDeduplicate, deduplicateToOutput, tableLocation);

















#endif//HARDWARE_WRAPPER_H
