/**
 * @file chunkdict.h
 * @author Taylor Nelms
 */
#ifndef CHUNKDICT_H
#define CHUNKDICT_H

//#include "sha_256.h"
//#include "common.h"

typedef unsigned char BYTE;

#define SHA256_BLOCK_SIZE 32

#define MINSIZE (1 * 1024)
#define MAXSIZE (6 * 1024)
#define MAXINPUTFILESIZE (200000000)//200MB
#define MAX_CHUNK_NUM (MAXINPUTFILESIZE / MINSIZE + 1)//may want to modify where/how this is declared

int indexForShaVal(const BYTE input[SHA256_BLOCK_SIZE]);
void resetTable();



#endif
