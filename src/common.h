/**
 * @file common.h
 * @author Taylor Nelms
 * A header file for access by multiple other source files, for various purposes
 */

#ifndef COMMON_H
#define COMMON_H

#define MINSIZE (1 * 1024)
#define MAXSIZE (8 * 1024)

#define MAXINPUTFILESIZE (1 << 16)//64KB
#define MAX_CHUNK_NUM (MAXINPUTFILESIZE / MINSIZE + 1)//may want to modify where/how this is declared








#endif
