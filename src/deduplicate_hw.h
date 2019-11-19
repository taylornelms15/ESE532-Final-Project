/**
 * @file deduplicate_hw.h
 * @author Taylor Nelms
 */

#ifndef DEDUPLICATE_HW_H
#define DEDUPLICATE_HW_H

#include "common.h"
#include <ap_int.h>
#include <hls_stream.h>


void deduplicate_hw(hls::stream< uint8_t > &shaToDeduplicate,
                    hls::stream< ap_uint<9> > &lzwToDeduplicate,
                    hls::stream< ap_uint<9> > &deduplicateToOutput,
                    uint8_t tableLocation[SHA256TABLESIZE]);






#endif
