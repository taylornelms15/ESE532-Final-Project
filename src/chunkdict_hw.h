/**
 * @file chunkdict.h
 * @author Taylor Nelms
 */
#ifndef CHUNKDICT_HW_H
#define CHUNKDICT_HW_H

#include "common.h"



int indexForShaVal_HW(const uint8_t input[SHA256_SIZE], uint8_t tableLocation[SHA256TABLESIZE],
		uint32_t currentDictIndex, uint32_t outputDictIndex[0]);



#endif
