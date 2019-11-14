/**
@file lzw_hw.h
@author Taylor Nelms
*/

#ifndef LZW_HW_H
#define LZW_HW_H

#include "common.h"
#include "ap_int.h"
#include <stdint.h>
#include <hls_stream.h>
#define MAXCHUNKLENGTH (MAXSIZE)


#define MAXCHARVAL 256




#pragma SDS data copy(input[0:numElements], output[0:MAXCHUNKLENGTH])
#pragma SDS data access_pattern(input:SEQUENTIAL, output:SEQUENTIAL)
#pragma SDS data mem_attribute(input:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)

int lzwCompressWrapper(const uint8_t input[MAXCHUNKLENGTH], int numElements, uint8_t output[MAXCHUNKLENGTH]);





#endif
