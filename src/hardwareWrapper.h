/**
 * @file hardwareWrapper.h
 * @author Taylor Nelms
 */

#ifndef HARDWARE_WRAPPER_H
#define HARDWARE_WRAPPER_H

#include "common.h"

#define INBUFFER_SIZE 1000000 //1MB incoming buffer
#define OUTBUFFER_SIZE (INBUFFER_SIZE)



/**
 * Takes in a buffer full of data, streams it through our processing, and fills an output buffer
 * Will want to make this the top-level function when synthesizing our FPGA things
 */
uint32_t processBuffer(uint8_t input[INBUFFER_SIZE], uint8_t output[OUTBUFFER_SIZE], uint8_t* shaDictLocation);




















#endif//HARDWARE_WRAPPER_H
