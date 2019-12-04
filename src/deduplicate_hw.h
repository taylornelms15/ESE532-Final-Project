/**
 * @file deduplicate_hw.h
 * @author Taylor Nelms
 */

#ifndef DEDUPLICATE_HW_H
#define DEDUPLICATE_HW_H

#include "common.h"
#include <hls_stream.h>

//#pragma SDS data zero_copy(tableLocation[0:SHA256TABLESIZE])
//#pragma SDS data mem_attribute(tableLocation:PHYSICAL_CONTIGUOUS)
void deduplicate_hw(hls::stream< uint8_t > &shaToDeduplicate,
                    hls::stream< ap_uint<9> > &lzwToDeduplicate,
                    hls::stream< ap_uint<9> > &deduplicateToOutput,
                    uint8_t tableLocation[SHA256TABLESIZE],
					uint32_t currentDictIndex,
					uint32_t outputDictIndex[1]);






#endif
