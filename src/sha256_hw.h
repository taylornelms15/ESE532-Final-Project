#if 0
#ifndef SHA256_HW
#define SHA256_HW

#include "common.h"
#include <stddef.h>
#include <stdint.h>
#include <hls_stream.h>
#include <ap_int.h>

void sha256_hw_wrapper(hls::stream<ap_uint<9>> &rabinToSHA, hls::stream< uint8_t > &shaToDeduplicate);

#endif	// SHA256_HW
#endif

#ifndef _SHA_256_HW
#define _SHA_256_HW

//#include <stdint.h>
#include "common.h"
#include <hls_stream.h>

void sha256_hw_wrapper(hls::stream<ap_uint<9> > &rabinToSHA, hls::stream< uint8_t > &shaToDeduplicate);
//void sha256_hw_wrapper(hls::stream<ap_uint<9> > &readerToRabin, hls::stream< uint8_t > &rabinToSHA);
//void sha256_hw_wrapper(hls::stream<ap_uint<9>>& rabinToSHA, hls::stream< uint8_t >& shaToDeduplicate);


#endif
