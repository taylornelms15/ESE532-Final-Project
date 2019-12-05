/**
 * @file deduplicate_hw.cpp
 * @author Taylor Nelms
 */

#include "chunkdict_hw.h"
#include "deduplicate_hw.h"
#include "stdint.h"

int counter_SHA = 0;
int counter_LZW = 0;
int counter_Dwr = 0;
int counter_Dtot = 0;
int chunknumDedup = 0;

static ap_uint<9> lzwOutputBuffer[LZWMAXSIZE + 4 + 1];//4 extra for the header, 1 extra for an ending ENDOFCHUNK or ENDOFFILE

/**
 * Reads the entire incoming buffer from LZW, writes it into our output buffer
 * Notably, want the position 4 away from the start (leaves room for header)
 * @return Size of actual chunk, with header and ending byte (will either ENDOFCHUNK or ENDOFFILE at the end)
 */
uint32_t readFromLzw(hls::stream< ap_uint<9> > &lzwToDeduplicate, uint8_t wasEndOfFile[1]){
    #pragma HLS inline
	ap_uint<9>* buffer = &lzwOutputBuffer[4];
    for(uint32_t i = 0; i < LZWMAXSIZE + 1; i++){
        #pragma HLS pipeline
        ap_uint<9> nextVal = lzwToDeduplicate.read();
        counter_LZW++;
        if (nextVal == ENDOFCHUNK || nextVal == ENDOFFILE){
            buffer[i] = nextVal;
            if (nextVal == ENDOFFILE) wasEndOfFile[0] = 1;
            else wasEndOfFile[0] = 0;
            return i + 5;//1 for this last byte, 4 for the header (not yet written)
        }//if the end of the line
        else{
            buffer[i] = nextVal;
        }
    }//for

    return 0;//to make our compiler happy?
}//readFromLzw

void readFromSha(hls::stream< uint8_t > &shaToDeduplicate, uint8_t shaBuffer[SHA256_SIZE]){
    #pragma HLS inline
    for (uint8_t i = 0; i < SHA256_SIZE; i++){
        uint8_t nextVal = shaToDeduplicate.read();
        counter_SHA++;
        shaBuffer[i] = nextVal;
    }//for
}

uint32_t fillHeaderBuffer(uint8_t foundSha, int shaIndex, uint32_t lzwChunkSize, uint8_t wasEndOfFile){
    #pragma HLS inline
	ap_uint<9>* headerBuffer = lzwOutputBuffer;
    uint32_t shiftedIndex;
    uint32_t sizeOfEndPacket;
    if (foundSha){
        shiftedIndex = shaIndex << 1;
        shiftedIndex |= 0x1;
        sizeOfEndPacket = 4;
    }//if writing sha index
    else{
        shiftedIndex = lzwChunkSize << 1;
        sizeOfEndPacket = lzwChunkSize + 4;//by default, not sending a last ENDOFCHUNK
    }//else writing LZW length

    //should be little-endian
    headerBuffer[0] = (ap_uint<9>)(0xFF & (shiftedIndex >> 0));
    headerBuffer[1] = (ap_uint<9>)(0xFF & (shiftedIndex >> 8));
    headerBuffer[2] = (ap_uint<9>)(0xFF & (shiftedIndex >> 16));
    headerBuffer[3] = (ap_uint<9>)(0xFF & (shiftedIndex >> 24));


    if (wasEndOfFile){
        sizeOfEndPacket++;
        if (foundSha) {
            headerBuffer[4] = ENDOFFILE;
        }
    }//if end of file

    return sizeOfEndPacket;

}//fillHeaderBuffer

void outputPacket(hls::stream< ap_uint<9> > &deduplicateToOutput,
                  uint32_t                  packetSendSize){
    #pragma HLS inline
    for(uint32_t i = 0; i < LZWMAXSIZE + 4 + 1; i++){
        #pragma HLS pipeline
        ap_uint<9> nextVal = lzwOutputBuffer[i];
        if (i < packetSendSize){
            deduplicateToOutput.write(nextVal);
            counter_Dwr++;
            counter_Dtot++;
        }
    }//for


}//outputPacket

void deduplicate_hw(hls::stream< uint8_t > &shaToDeduplicate,
                    hls::stream< ap_uint<9> > &lzwToDeduplicate,
                    hls::stream< ap_uint<9> > &deduplicateToOutput,
                    uint8_t tableLocation[SHA256TABLESIZE],
					uint32_t currentDictIndex,
					uint32_t outputDictIndex[1]){
	uint32_t currentIndex = currentDictIndex;
	uint32_t outputIndex;

    chunknumDedup = 0;
    uint8_t shaBuffer[SHA256_SIZE];
    uint8_t wasEndOfFile[1] = {0};//keeps track of if the previous chunk ended with ENDOFFILE, and can be used as an inout variable

    for(int j = 0; j < MAX_CHUNKS_IN_HW_BUFFER; j++){
        counter_SHA = 0;
        counter_LZW = 0;
        counter_Dwr = 0;
        chunknumDedup++;

        //read in our LZW chunk (need to do this regardless of SHA found, to clear the stream)
        uint32_t packetSize = readFromLzw(lzwToDeduplicate, wasEndOfFile);//don't bother giving it the first 4 header bytes
        uint32_t lzwChunkSize = packetSize - 5;//write this value (modified) to the header

        //read in our SHA value
        readFromSha(shaToDeduplicate, shaBuffer);

        int shaIndex = indexForShaVal_HW(shaBuffer, tableLocation, currentIndex, &outputIndex);
        currentIndex = outputIndex;
        uint8_t foundSha;//stand-in for a boolean
        if (shaIndex < 0) foundSha = 0;
        else foundSha = 1;
        

        //fill in the header appropriately
        uint32_t packetSendSize = fillHeaderBuffer(foundSha, shaIndex, lzwChunkSize, wasEndOfFile[0]);

        outputPacket(deduplicateToOutput, packetSendSize);

        /*HLS_PRINTF("DEDUP\t%d\tR SHA %d\n", chunknumDedup, counter_SHA);
        HLS_PRINTF("DEDUP\t%d\tR LZW %d\n", chunknumDedup, counter_LZW);
        HLS_PRINTF("DEDUP\t%d\tW OUT %d\n", chunknumDedup, counter_Dwr);
*/
        if (wasEndOfFile[0]){
   //         HLS_PRINTF("DEDUP\t%d\tW TOTAL %d\n", chunknumDedup, counter_Dtot);
            *outputDictIndex = outputIndex;
            return;
        }//if the last run was our final one

    }//for each incoming chunk



}//deduplicate_hw

