#ifndef SHA256_HW
#define SHA256_HW

#include "common.h"
#include <stddef.h>
#include <stdint.h>
#include <hls_stream.h>
#include <ap_int.h>

void sha256_hw_wrapper(hls::stream<ap_uint<9>> &rabinToSHA, hls::stream< uint8_t > &shaToDeduplicate);

#endif	// SHA256_HW
