/**
@file lzw_hw.c
@author Taylor Nelms
*/

#include "lzw_hw.h"

#define NONEFOUND 0x1FFF
//Tunable design axis parameters
#define HDEPTH 24//depth of the hash bucket (capacity = (8k vals / 1k rows) * (3x capacity variance buffer))
#define ROWNUM (MAXCHUNKLENGTH)
#define ROWBITS 13//log2(number of rows)
#define COLNUM (MAXCHARVAL)
#define COLBITS 8//log2(number of columns)
#define HBUCKETS 1024//number of hash buckets
#define HBITS 10//log2(number of hash buckets)
#define KEYLEN (ROWBITS + COLBITS)
#define HRBITS (KEYLEN + ROWBITS)//number of bits in a "hash record" (key+val pairing)

/*
Thoughts about space:
8k total table rows (13b)
256 total table cols (8b)
So, 21b address needed, matching to a 13b value

21b address needs to reduce down to the 13b "where is my value" index

Match BRAM: need 3 (one per 9b of key) times 114 (1 per 72 entries) BRAM's

Value BRAM: 8k entries, each 13b wide
 */

//conceptually:
//ap_uint<8192> matchTable_Top[128];//top 7 bits of key
//ap_uint<8192> matchTable_Med[128];//med 7 bits of key
//ap_uint<8192> matchTable_Low[128];//low 7 bits of key
//
//for matchTable_Top:
//one BRAM can hold 4 sets of "rows" (512/128)
//brings it down to ap_uint<2048> matchTable_TopMod[512]
//so, given key `k`, matchTable_Top[k] = matchTable_TopMod[k] | (matchTable_TopMod[k + 128] << 2048) | (matchTable_TopMod[k + 256] << 4096) |  (matchTable_TopMod[k + 384] << 6144); 
//ap_uint<13> valTable[MAXCHUNKLENGTH];//should split into multiple BRAM's...? (2 of them, ish)
//static uint16_t table[MAXCHUNKLENGTH][MAXCHARVAL];

static const uint8_t rindBox[256] = {
  //0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 };






/*
Hash approach:
34-bit value pairs (key(21) plus val(13))
2 in each line of BRAM
2 BRAM's for 2-deep hash table (1024 entries total, 68 bits of value)
Target: hold 8k entries, so 4 2-deep tables
But, hash function not great, budget for 24k entries
So, 12 2-deep tables, meaning 24 BRAMs for it
*/

static ap_uint<HRBITS> hashTable [HDEPTH][HBUCKETS];//1 wide, 1024 tall, 24 deep)
static ap_uint<HDEPTH> validityTable[HBUCKETS];//bitmask for each of the hash table entries

static uint8_t boundary = 0;
static uint8_t currentOverflow = 0;



/**
 * Hash function to reduce a 20- or 21-bit key down to a 10-bit hash row index
 * Across a couple of empirical tests, this is not uniform, but allowed for a factor-of-3 difference between ideal hash and max number of elements in the buckets
 * As such, a factor of around 3 for the actual hash table should be able to hold most/all of the required values,
 * with a smaller associative memory handling the overflow
 */
ap_uint<HBITS> hashKey(const ap_uint<KEYLEN> key){


    uint8_t vlo4    = (key)         & 0xF;
    uint8_t lo4     = (key >> 4)    & 0xF;
    uint8_t hi4     = (key >> 8)    & 0xF;
    uint8_t vhi4    = (key >> 12)   & 0xF;
    uint8_t vvhi5   = (key >> 16)   & 0x1F;
    uint8_t sub8    = rindBox[((vvhi5 << 3) ^ vlo4)];
    uint8_t hiT4    = vhi4 ^ vlo4;
    uint8_t midT4   = (vlo4 ^ hi4 ^ vvhi5) & 0xF;
    uint8_t loT4    = lo4 ^ hi4;
    uint16_t retval = 0;
    retval          ^= loT4;
    retval          ^= (midT4 << 3);
    retval          ^= (sub8 << 1);
    retval          ^= (hiT4 << 6);
    retval          &= 0x3FF;
    return (ap_uint<HBITS>) retval;

}//hashKey

void resetValidityTable(){
	for(int i = 0; i < HBUCKETS; i++){
		validityTable[i] = 0;
	}
}//resetValidityTable

uint8_t writeToTable(const ap_uint<KEYLEN> key, const ap_uint<ROWBITS> val,
				const ap_uint<HDEPTH> validityMask, const ap_uint<HBITS> hashedKey){
	#pragma HLS inline
    #pragma HLS pipeline II=1
    int i;
    uint8_t freeSlot = HDEPTH;
    //want position of rightmost 0 bit of validityMask
    for(i = HDEPTH - 1; i >=0; i--){
        #pragma HLS unroll
        if (!(validityMask & (1 << i))){
            freeSlot = i;
        }//found a 0
    }//for
    //TODO: handle overflow, and going to associative memory?

    ap_uint<HRBITS> entryToWrite = ((( ap_uint<HRBITS> ) key) << ROWBITS) | (val);
    hashTable[freeSlot][hashedKey] = entryToWrite;

    return freeSlot;
    

}//writeToTable

/**
 * Reads a value from the LZW table
 * If not found, returns 0x1FFF
 */
ap_uint<ROWBITS> readFromTable(ap_uint<KEYLEN> key, ap_uint<HDEPTH> validityEntry, ap_uint<HBITS> hashedKey){
    #pragma HLS inline
	#pragma HLS pipeline II=1

    ap_uint<ROWBITS> retval = NONEFOUND;


    ap_uint<HRBITS> potentialValues[HDEPTH];
    #pragma HLS ARRAY_RESHAPE variable=potentialValues block factor=2 dim=1
    for(uint8_t i = 0; i < HDEPTH; i++){
        #pragma HLS unroll
        ap_uint<HRBITS> candidate = hashTable[i][hashedKey];
        potentialValues[i] = candidate;
    }//for
    
    for(uint8_t i = 0; i < HDEPTH; i++){
		#pragma HLS unroll
        ap_uint<KEYLEN> keyCandidate = (ap_uint<KEYLEN>)(potentialValues[i] >> ROWBITS);
        if (keyCandidate == key && (validityEntry & (1 << i))){
            retval = (ap_uint<ROWBITS>)(potentialValues[i]);
        }//if a match
    }//for


    return retval;
}//readFromTable

uint8_t writeToOutput(const ap_uint<ROWBITS> val, hls::stream< ap_uint<9> > &output ){
	#pragma HLS inline
	uint8_t numOutput = 0;
    ap_uint<9> valsToOutput1;
    ap_uint<9> valsToOutput2;
    if(boundary < 3){
        valsToOutput1 = (currentOverflow | ((val >> (5 + boundary)) & 0xFF));
        numOutput = 1;
        currentOverflow = (uint8_t)(val << (3 - boundary));
	}//0, 1, 2
	else if (boundary == 3){
        valsToOutput1 = (currentOverflow | ((val >> (5 + boundary)) & 0xFF));
        valsToOutput2 = (val & 0xFF);
        numOutput = 2;
        currentOverflow = 0;
	}//3
	else{
		valsToOutput1 = (currentOverflow | (val >> (5 + boundary)));
        valsToOutput2 = ((val >> ((5 + boundary) & 0x7)) & 0xFF);
        numOutput = 2;
        currentOverflow = (uint8_t)(val << (11 - boundary));
    }//5,6,7,8
    boundary = (boundary + 5) & 0x7;

    output.write(valsToOutput1);
    if (numOutput == 2){
    	output.write(valsToOutput2);
    }

    return numOutput;


}//writeToOutput

int lzwCompressHW(hls::stream< ap_uint<9> > &input, hls::stream< ap_uint<9> > &output){
	#pragma HLS array_partition variable=hashTable dim=1

    resetValidityTable();
    boundary = 0;//what bit of an output byte we'd write next
    currentOverflow = 0;//stores our extra bits

    ///keeps track of how many bytes we've written
    uint16_t oidx = 0;
    ///keeps track of how many records we've written
    uint16_t numWritten = 0;

    ap_uint<ROWBITS> curTableRow = (ap_uint<ROWBITS>)(input.read());
    for(uint16_t iidx = 1; iidx <= MAXCHUNKLENGTH; iidx++) {
		#pragma HLS loop_tripcount min=1024 max=8192 avg=4096
        #pragma HLS pipeline II=6//ideally 2 because we may need to write to output stream twice; using 6 to meet timing reqs
        ap_uint<9> readChar = input.read();
        if (readChar > 255){
            oidx += writeToOutput(curTableRow, output);
            numWritten++;

            break;
            //return oidx;//TODO: don't return, break and reset validity table...?
        }//if end-of-stream value
        else{
            uint8_t curChar = (uint8_t)readChar;
            ap_uint<KEYLEN> key = (((ap_uint<KEYLEN>) curTableRow) << COLBITS) | curChar;
            ap_uint<HBITS> hashedKey = hashKey(key);
            ap_uint<HDEPTH> validityEntry = validityTable[hashedKey];
            ap_uint<ROWBITS> currentTableValue = readFromTable(key, validityEntry, hashedKey);
            if (currentTableValue != NONEFOUND){
                curTableRow = currentTableValue;
            }
            else {
                oidx += writeToOutput(curTableRow, output);
                numWritten++;
                ap_uint<ROWBITS> valToWrite = numWritten + MAXCHARVAL - 1;
                uint8_t freeSlot = writeToTable(key, valToWrite, validityEntry, hashedKey);
                validityTable[hashedKey] = validityEntry | (1 << freeSlot);
                curTableRow = curChar;
            }
        }//else, legit value

    }
    if (boundary != 0){
    	output.write(currentOverflow);
    	oidx += 1;//not counting stop byte in our oidx
    }
    output.write(0x100);

    return oidx + 4;

}//lzwCompress

void inputToStream(const uint8_t input[MAXCHUNKLENGTH], int numElements, hls::stream< ap_uint<9> > &inHW){
	for(int i = 0; i < numElements; i++){
		#pragma HLS pipeline II=1
		#pragma HLS loop_tripcount min=1024 max=8192
		uint8_t nextInput = input[i];
		inHW.write((ap_uint<9>)(nextInput));
	}
	inHW.write((ap_uint<9>) 256);
}

void outputFromStream(hls::stream< ap_uint<9> > &outHW, uint8_t output[MAXCHUNKLENGTH]){
    //uint32_t header = (numOutput + 4) << 1;//bit 0 is 0 because LZW chunk, the rest is the size of the data. Subtracting 4 to get "size of LZW" part sans header

    //output[oidx++] = (uint8_t)(header >> 0);
    //output[oidx++] = (uint8_t)(header >> 8);
    //output[oidx++] = (uint8_t)(header >> 16);
    //output[oidx++] = (uint8_t)(header >> 24);
    int foundEnd = 0;

	for(int i = 0; i < MAXCHUNKLENGTH; i++){
		#pragma HLS pipeline II=1
		#pragma HLS loop_tripcount min=1 max=8192
		uint8_t valToOutput = 0;
		if (!foundEnd){
		    ap_uint<9> nextOutput = outHW.read();
		    if(nextOutput & 0x100){
		    	valToOutput = 0;
			    foundEnd = 1;
		    }
		    else{
			    valToOutput = nextOutput;
		    }
		}//reading from stream still
		else{
			valToOutput = 0;
		}
		output[i] = valToOutput;
	}

}

int lzwCompressWrapper(const uint8_t input[MAXCHUNKLENGTH], int numElements, uint8_t output[MAXCHUNKLENGTH]){
    #pragma HLS STREAM variable=input depth=16
    #pragma HLS STREAM variable=output depth=16
	static hls::stream< ap_uint<9> > inHW;
	static hls::stream< ap_uint<9> > outHW;


	#pragma HLS dataflow

	inputToStream(input, numElements, inHW);
	int numOutput = lzwCompressHW(inHW, outHW);
	outputFromStream(outHW, output);


	return numOutput;
}



