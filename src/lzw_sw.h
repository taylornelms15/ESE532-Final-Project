/**
 * @file lzw_sw.h
 * @author Taylor Nelms
 */


#ifndef LZW_SW_H
#define LZW_SW_H

#define MAXCHUNKLENGTH 8192
#define MAXCHARVAL 256
#define NONEFOUND 0xFFFF



int lzwCompress(const uint8_t* input, int numElements, uint8_t* output);



#endif
