/**
 * @file chunkdict.h
 * @author Taylor Nelms
 */
#ifndef CHUNKDICT_HW_H
#define CHUNKDICT_HW_H

#include "common.h"



//#pragma SDS data zero_copy(tableLocation[0:SHA256TABLESIZE])
//#pragma SDS data mem_attribute(tableLocation:PHYSICAL_CONTIGUOUS)
int indexForShaVal_HW(const uint8_t input[SHA256_SIZE], uint8_t tableLocation[SHA256TABLESIZE]);



#endif
