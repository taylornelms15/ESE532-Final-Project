/**
 * @file standinfuncts.cpp
 * @author Taylor Nelms
 */


#include "standinfuncts.h"

#define FAKECHUNKSIZE (1024 * 3)//3k fake chunk size

void rabin_hw_fake(hls::stream< ap_uint<9> > &readerToRabin, hls::stream< ap_uint<9> > &rabinToSHA, hls::stream< ap_uint<9> > &rabinToLZW, uint32_t numElements){

    int overflowAmount = numElements % FAKECHUNKSIZE;
    int numDivisions;
    if (overflowAmount < MINSIZE){
        numDivisions = (numElements / FAKECHUNKSIZE) - 1;
    }//if we'd have too small a last chunk
    else{
        numDivisions = (numElements / FAKECHUNKSIZE) - 0;
    }//if we'd have a bearable last chunk

    int numDivisionsMarked = 0;

    for(int i = 0; i < INBUFFER_SIZE; i++){
        #pragma HLS pipeline II=2
        ap_uint<9> nextVal = readerToRabin.read();

        rabinToSHA.write(nextVal);
        rabinToLZW.write(nextVal);

        if (i == numElements - 1){
            break;
        }
        if (i != 0 && i % FAKECHUNKSIZE == 0 && numDivisionsMarked < numDivisions){
            rabinToSHA.write(ENDOFCHUNK);
            rabinToLZW.write(ENDOFCHUNK);
            numDivisionsMarked++;
        }//if we're putting a fake chunk break in here

    }//for
    rabinToSHA.write(ENDOFFILE);
    rabinToLZW.write(ENDOFFILE);
    return;

}//rabin_hw_fake

int16_t PRNG(uint32_t nSeed[1]){
    #pragma HLS inline

    // Take the current seed and generate a new value from it
    // Due to our use of large constants and overflow, it would be
    // very hard for someone to predict what the next number is
    // going to be from the previous one.
    nSeed[0] = (8253729 * nSeed[0] + 2396403);

    // Take the seed and return a value between 0 and 32767 (15 bits)
    return (int16_t)(nSeed[0]);
}

static uint8_t chunkBuffer[MAXSIZE];

void sha_hw_fake(hls::stream< ap_uint<9> > &rabinToSHA, hls::stream< uint8_t > &shaToDeduplicate){

    //uint8_t chunkBuffer[MAXSIZE];
    uint8_t foundEndOfFile = 0;
    uint32_t nSeed[1] = {5323};//prng seed
    #pragma HLS array_partition variable=nSeed

    for(int j = 0; j < MAX_CHUNKS_IN_HW_BUFFER + 1; j++){
        //#pragma HLS loop_tripcount min=1000 max=2000
        //For each incoming chunk

        //read the input stream
        for (int i = 0; i < MAXSIZE + 1; i++){
            #pragma HLS pipeline II=2
            #pragma HLS loop_tripcount min=1024 max=6144 avg=2048
            ap_uint<9> nextVal = rabinToSHA.read();
            if (nextVal == ENDOFCHUNK || nextVal == ENDOFFILE){
                if (nextVal == ENDOFFILE) foundEndOfFile = 1;
                break;
            }
            else{
                chunkBuffer[i] = (uint8_t) nextVal;
            }//else
        }//for i

        //output 32 random bytes
        for (int i = 0; i < SHA256_SIZE; i++){
            #pragma HLS pipeline II=2
            uint8_t nextByte = (uint8_t) PRNG(nSeed);
            shaToDeduplicate.write(nextByte);

        }//for

        if (foundEndOfFile) return;
    }//for j


}//sha_hw_fake






