/**
 * @file hardwareWrapper.cpp
 * @author Taylor Nelms
 */

#include "hardwareWrapper.h"
#include "deduplicate_hw.h"
#include "lzw_hw.h"
#include "standinfuncts.h"

void readIntoRabin(uint8_t input[INBUFFER_SIZE], hls::stream< ap_uint<9> > &readerToRabin, uint32_t numElements){

    for (int i = 0; i < INBUFFER_SIZE; i++){
        #pragma HLS pipeline II=1
        uint8_t nextValue = input[i];
        if (i < numElements){
            readerToRabin.write( (ap_uint<9>) nextValue);
        }//normal write

    }//for
    readerToRabin.write(ENDOFFILE);//currently, is not keeping chunk-state between adjacent inputs. Probably cleaner this way.

}//readIntoRabin

uint32_t finalOutput(hls::stream< ap_uint<9> > &deduplicateToOutput, uint8_t output[OUTBUFFER_SIZE], uint32_t numElements){

    uint32_t numOutput = 0;
    uint8_t foundEnd = 0;

    for(int i = 0; i < OUTBUFFER_SIZE; i++){//fake for-loop; will likely break somewhere in middle
        uint8_t valToWrite = 0;
        if(foundEnd){
            valToWrite = 0;
        }
        else{
            ap_uint<9> nextByte = deduplicateToOutput.read();
            if (nextByte < 256){
                valToWrite = (uint8_t) nextByte;
                numOutput++;
            }//if valid piece of data
            else{
                valToWrite = 0;
                foundEnd = 1;
                numOutput++;//now reflects length, not index
            }//else
        }
        output[i] = valToWrite;

    }//for


    return numOutput;

}//finalOutput

uint32_t processBuffer(uint8_t input[INBUFFER_SIZE], uint8_t output[OUTBUFFER_SIZE], 
                       uint8_t tableLocation[SHA256TABLESIZE], uint32_t numElements){
    #pragma HLS STREAM variable=input //depth=32//not sure if good number
    #pragma HLS STREAM variable=output //depth=32//not sure if good number

    static hls::stream< ap_uint<9> > readerToRabin;
    static hls::stream< ap_uint<9> > rabinToSHA; 
    static hls::stream< ap_uint<9> > rabinToLZW;
    static hls::stream< ap_uint<9> > lzwToDeduplicate;
    static hls::stream< uint8_t > shaToDeduplicate;
    static hls::stream< ap_uint<9> > deduplicateToOutput;



    #pragma HLS dataflow
    readIntoRabin(input, readerToRabin, numElements);
    rabin_hw_fake(readerToRabin, rabinToSHA, rabinToLZW, numElements);
    sha_hw_fake(rabinToSHA, shaToDeduplicate);
    lzwCompressAllHW(rabinToLZW, lzwToDeduplicate);
    deduplicate_hw(shaToDeduplicate, lzwToDeduplicate, deduplicateToOutput, tableLocation);
    uint32_t numOutput = finalOutput(deduplicateToOutput, output, numElements);


    return numOutput;
}//processBuffer






