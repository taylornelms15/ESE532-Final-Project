/**
 * @file common.h
 * @author Taylor Nelms
 * A header file for access by multiple other source files, for various purposes
 */

#define HALF_ENABLE_CPP11_CMATH 0

#ifndef COMMON_H
#define COMMON_H

#define MINSIZE (1 * 1024)
#define MAXSIZE (8 * 1024)

#define MAXINPUTFILESIZE (1 << 28)//270MB
#define MAX_CHUNK_NUM (MAXINPUTFILESIZE / MINSIZE + 1)//may want to modify where/how this is declared

//Codes for use with 9-bit streams to denote the end of the chunk and/or the end of the file
#define ENDOFCHUNK  256
#define ENDOFFILE   257







#endif
