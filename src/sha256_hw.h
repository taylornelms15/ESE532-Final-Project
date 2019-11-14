#ifndef SHA256_HW
#define SHA256_HW

#include <stddef.h>

#define SHA256_BLOCK_SIZE 32

typedef unsigned char BYTE;

void sha256_hw_compute(const BYTE* data, size_t len, BYTE hash[SHA256_BLOCK_SIZE]);

#endif	// SHA256_HW
