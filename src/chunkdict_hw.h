/**
 * @file chunkdict.h
 * @author Taylor Nelms
 */
#ifndef CHUNKDICT_HW_H
#define CHUNKDICT_HW_H

#include "common.h"



#pragma SDS data zero_copy(tableLocation[0:SHA256TABLESIZE])
#pragma SDS data copy(input[0:SHA256_SIZE])
#pragma SDS data access_pattern(input:RANDOM, tableLocation:RANDOM)
int indexForShaVal_HW(const uint8_t input[SHA256_SIZE], uint8_t tableLocation[SHA256TABLESIZE]);


/**
 * Resets the table using memset. Only used in software
 */
void resetTable(uint8_t tableLocation[SHA256TABLESIZE]);
/**
 * Resets the table by looping through. Likely unused.
 */
void resetTable_HW(uint8_t tableLocation[SHA256TABLESIZE]);



#endif
