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

#define MAXINPUTFILESIZE (1 << 28)//256MB
#define MAX_CHUNK_NUM (MAXINPUTFILESIZE / MINSIZE + 1)//may want to modify where/how this is declared

#define MAXPKTSIZE 4096
#define HEADER 2

//#define HWIMPL

#ifdef HWIMPL
	#define USING_LZW_HW
	#define USING_RABIN_HW
#endif




#endif

