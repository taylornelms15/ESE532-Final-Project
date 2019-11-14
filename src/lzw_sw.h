/**
 * @file lzw_sw.h
 * @author Taylor Nelms
 */


#ifndef LZW_SW_H
#define LZW_SW_H

#include "common.h"

#define MAXCHUNKLENGTH (MAXSIZE)
#define MAXCHARVAL 256




int lzwCompress(const uint8_t* input, int numElements, uint8_t* output);



#endif
