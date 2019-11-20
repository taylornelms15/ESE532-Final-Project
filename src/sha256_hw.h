#ifndef SHA256_HW
#define SHA256_HW

#include <stddef.h>
#include "sha_256.h"

void sha256_hw_compute(const BYTE* data, int len, BYTE hash[SHA256_BLOCK_SIZE]);

#endif	// SHA256_HW
