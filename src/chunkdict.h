/**
 * @file chunkdict.h
 * @author Taylor Nelms
 */
#ifndef CHUNKDICT_H
#define CHUNKDICT_H

#include "sha_256.h"


int indexForShaVal(const BYTE input[SHA256_BLOCK_SIZE]);
void resetTable();



#endif
