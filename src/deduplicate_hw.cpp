/**
 * @file deduplicate_hw.cpp
 * @author Taylor Nelms
 */

#include "chunkdict_hw.h"
#include "deduplicate_hw.h"

//#define SHA_CACHE

#ifdef SHA_CACHE
#define SHA_CACHE_SIZE	10
uint8_t sha_cache_itr = 0;
#endif

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
        }
    }//for


}//outputPacket

void deduplicate_hw(hls::stream< uint8_t > &shaToDeduplicate,
                    hls::stream< ap_uint<9> > &lzwToDeduplicate,
                    hls::stream< ap_uint<9> > &deduplicateToOutput,
                    uint8_t tableLocation[SHA256TABLESIZE]){


    uint8_t shaBuffer[SHA256_SIZE];
    uint8_t wasEndOfFile[1] = {0};//keeps track of if the previous chunk ended with ENDOFFILE, and can be used as an inout variable

#ifdef SHA_CACHE
    /** Small cache for storing SHA values and corresponding chunk index */
#pragma HLS array_partition variable=sha_val_cache
#pragma HLS array_partition variable=sha_index_cache dim=0

    static uint8_t sha_val_cache[SHA_CACHE_SIZE][SHA256_SIZE];
    static int sha_index_cache[SHA_CACHE_SIZE];
    bool sha_cache_match;
#endif

    for(int j = 0; j < MAX_CHUNKS_IN_HW_BUFFER; j++){

    	printf("chunk no: %d\n", j);
        //read in our LZW chunk (need to do this regardless of SHA found, to clear the stream)
        uint32_t packetSize = readFromLzw(lzwToDeduplicate, wasEndOfFile);//don't bother giving it the first 4 header bytes
        uint32_t lzwChunkSize = packetSize - 5;//write this value (modified) to the header

        //read in our SHA value
        readFromSha(shaToDeduplicate, shaBuffer);

        int shaIndex;
#ifdef SHA_CACHE
        /** Check if this sha val is present in the SHA cache */
        int  i;
        for(i = 0; i < SHA_CACHE_SIZE; i++)
        {
        	sha_cache_match = 1;
#pragma HLS unroll
        	for (int j = 0; j < SHA256_SIZE; j++)
        	{
        		if (sha_val_cache[i][j] != shaBuffer[j])
        		{
        			sha_cache_match = 0;
        			//printf("sha value at %d din't match\n", i);
        			break;
        		}
        	}

        	if (sha_cache_match == 1)
        	{
        		printf("found sha in cache - sha index : %d\n", sha_index_cache[i]);
        		break;
        	}
        }

        if (sha_cache_match)
        {
        	shaIndex = sha_index_cache[i];
        }
        else
        {
        	shaIndex = indexForShaVal_HW(shaBuffer, tableLocation);
        	if (shaIndex >= 0)
        		printf("Chunk found in table - sha index : %d\n", shaIndex);
        }

        //printf("sha index : %d\n", shaIndex);
        /** Store the value in sha cache */
        if (!sha_cache_match && shaIndex >= 0)
        {
			sha_cache_itr %= SHA_CACHE_SIZE;
			//printf("sha cache iter: %u\n", sha_cache_itr);
			for (int k = 0; k < SHA256_SIZE; k++)
			{
#pragma HLS unroll
				sha_val_cache[sha_cache_itr][k] = shaBuffer[k];
			}
			sha_index_cache[sha_cache_itr] = shaIndex;
			sha_cache_itr++;
		}
#else
        shaIndex = indexForShaVal_HW(shaBuffer, tableLocation);
#endif

        uint8_t foundSha;//stand-in for a boolean
        if (shaIndex < 0) foundSha = 0;
        else foundSha = 1;
        
        //fill in the header appropriately
        uint32_t packetSendSize = fillHeaderBuffer(foundSha, shaIndex, lzwChunkSize, wasEndOfFile[0]);

        outputPacket(deduplicateToOutput, packetSendSize);

        if (wasEndOfFile[0]){
            return;
        }//if the last run was our final one

    }//for each incoming chunk



}//deduplicate_hw

