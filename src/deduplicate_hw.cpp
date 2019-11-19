/**
 * @file deduplicate_hw.cpp
 * @author Taylor Nelms
 */

#include "deduplicate_hw.h"
#include "chunkdict_hw.h"

void deduplicate_hw(hls::stream< uint8_t > &shaToDeduplicate,
                    hls::stream< ap_uint<9> > &lzwToDeduplicate,
                    hls::stream< ap_uint<9> > &deduplicateToOutput,
                    uint8_t tableLocation[SHA256TABLESIZE]){

    uint8_t lzwOutputBuffer[MAXSIZE + 4];
    uint8_t shaBuffer[SHA256_SIZE];

    for (uint8_t i = 0; i < SHA256_SIZE; i++){
        uint8_t nextVal = shaToDeduplicate.read();
        shaBuffer[i] = nextVal;
    }//for





}//deduplicate_hw
