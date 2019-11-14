#ifndef SHA256_HW
#define SHA256_HW

#if 0
#include <stddef.h>

#define SHA256_BLOCK_SIZE 32

typedef unsigned char BYTE;
typedef unsigned int  WORD_SHA;             // 32-bit word, change to "long" for 16-bit machines

void sha256_hw_compute(const BYTE* data, size_t len, BYTE hash[SHA256_BLOCK_SIZE]);
#endif
#endif	// SHA256_HW
