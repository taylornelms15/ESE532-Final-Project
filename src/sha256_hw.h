#ifndef SHA256_HW
#define SHA256_HW

#include "common.h"
#include <stddef.h>
#include <hls_stream.h>
//#include <ap_int.h>
#include "sha_256.h"

void sha256_hw_compute(const BYTE* data, int len, BYTE hash[SHA256_BLOCK_SIZE]);
void sha256_hw_wrapper(hls::stream<ap_uint<9>>& rabinToSHA, hls::stream< uint8_t >& shaToDeduplicate);

#endif	// SHA256_HW
