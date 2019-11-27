/**
@file lzw_hw.h
@author Taylor Nelms
*/

#ifndef LZW_HW_H
#define LZW_HW_H

#include "common.h"
#include <hls_stream.h>


#define MAXCHARVAL 256

void lzwCompressAllHW(hls::stream< ap_uint<9> > &rabinToLZW, hls::stream< ap_uint<9> > &lzwToDeduplicate);



#endif
