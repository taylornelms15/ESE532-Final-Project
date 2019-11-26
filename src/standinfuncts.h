/**
 * @file standinfuncts.h
 * @author Taylor Nelms
 */

#ifndef STANDINFUNCTS_H
#define STANDINFUNCTS_H


#include "common.h"
#include <hls_stream.h>


void rabin_hw_fake(hls::stream< ap_uint<9> > &readerToRabin, hls::stream< ap_uint<9> > &rabinToSHA, hls::stream< ap_uint<9> > &rabinToLZW, uint32_t numElements);


void sha_hw_fake(hls::stream< ap_uint<9> > &rabinToSHA, hls::stream< uint8_t > &shaToDeduplicate);












#endif
