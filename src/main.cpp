#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
extern "C"{
#include "rabin.h"
#include "sha_256.h"
#include "lzw_sw.h"
#include "chunkdict.h"
}
#include "lzw_hw.h"
#ifdef __SDSCC__
#include <ff.h>
#include <sds_lib.h>
#endif

//#define USING_LZW_HW

#ifdef __SDSCC__
#define MEASURING_LATENCY 1
#else
#define MEASURING_LATENCY 0
#endif


#include "common.h"

// 1MiB buffer
uint8_t* buf;
size_t bytes;

extern uint64_t mod_table[256];
extern uint64_t out_table[256];
static const char infileName[] = "/home/nishanth/University/ESE_532/Final_project/HLS/ESE532-Final-Project/Testfiles/ESE532.tar";
static const char outfileName[] = "/home/nishanth/University/ESE_532/Final_project/HLS/ESE532-Final-Project/Testfiles/ESE532.dat";
static const char linuxInfileName[] = "linux.tar";
static const char linuxOutfileName[] = "linux.dat";

void Check_error(int Error, const char * Message)
{
  if (Error)
  {
    fputs(Message, stderr);
    exit(EXIT_FAILURE);
  }
}

void Exit_with_error(void)
{
  perror(NULL);
  exit(EXIT_FAILURE);
}

uint8_t* Allocate(int Size){
  uint8_t* Frame = (uint8_t*)
#ifdef __SDSCC__
      sds_alloc(Size);
#else
      malloc(Size);
#endif
  if (Frame == NULL){
	  printf("ERROR: Could not allocate memory\n");
	  exit(EXIT_FAILURE);
  }

  return Frame;
}

void Free(uint8_t* Frame){
#ifdef __SDSCC__
  sds_free(Frame);
#else
  free(Frame);
#endif
}




unsigned int Load_data(unsigned char * Data)
{
  unsigned int Size = MAXINPUTFILESIZE;
  unsigned int Bytes_read;

#ifdef __SDSCC__
  FIL File;

  FRESULT Result = f_open(&File, linuxInfileName, FA_READ);
  Check_error(Result != FR_OK, "Could not open input file.");

  Result = f_read(&File, Data, Size, &Bytes_read);
  Check_error(Result != FR_OK, "Could not read input file.");

  Check_error(f_close(&File) != FR_OK, "Could not close input file.");
#else
  FILE * File = fopen(infileName, "rb");
  if (File == NULL){
	  printf("Could not open input file\n");
      Exit_with_error();
  }

  Bytes_read = fread(Data, 1, Size, File);
  if (Bytes_read < 1){
	  printf("None read, result %d\n", Bytes_read);
      Exit_with_error();
  }

  if (fclose(File) != 0)
    Exit_with_error();
#endif
  return Bytes_read;
}

int compareArrays(const uint8_t* cand, const uint8_t* gold, const int numElements){
	int numMistakes = 0;
	int maxMistakes = 8;
	for(int i = 0; i < numElements; i++){
		if (cand[i] != gold[i]){
			printf("B\tCandidate value of 0x%02x differed from golden value of 0x%02x at position [%d]\n", cand[i], gold[i], i);
			numMistakes++;
			if (numMistakes >= maxMistakes){
			    return 1;
			}
		}
		else{
			//printf("G\tCandidate value of 0x%02x matched golden value of 0x%02x at position [%d]\n", cand[i], gold[i], i);
		}
	}//for
	//printf("Two arrays equal!!!!!!!!!!\n");
	return 0;
}

int main(int argc, char *argv[]) {

	struct rabin_t *hash;
	hash = rabin_init();
    SHA256_CTX ctx;
    sha256_init(&ctx);
    unsigned int chunks = 0;
    BYTE sha_buf[SHA256_BLOCK_SIZE];

    uint8_t* compress = Allocate(MAXSIZE + 4);
    printf("Compress allocated at %x\n", compress);


    printf("Starting main function\n");

    buf = Allocate(MAXINPUTFILESIZE * sizeof(uint8_t));
    printf("buf allocated at %x\n", buf);

#if MEASURING_LATENCY
    unsigned long long numBytesUncompressed = 0;//number of bytes fed into LZW
    unsigned long long numBytesCompressed   = 0;//number of bytes fed out of LZW
    unsigned long long timeInLZW            = 0;//how many cycles we spent in LZW
#endif

#ifdef __SDSCC__
    FATFS FS;
    unsigned int bytes_read;

    Check_error(f_mount(&FS, "0:/", 0) != FR_OK, "Could not mount SD-card");
#endif

    unsigned int len = Load_data(buf);

#ifdef __SDSCC__
    FIL File;

    FRESULT Result = f_open(&File, linuxOutfileName, FA_WRITE | FA_CREATE_ALWAYS);
    Check_error(Result != FR_OK, "Could not open output file.");

#else
    FILE* File = fopen(outfileName, "wb");
    if (File == NULL)
        Exit_with_error();

#endif

    uint8_t *ptr = buf;
    uint8_t chunk[MAXSIZE];
    bytes += len;
    int remaining;

        while (len > 0) {

        	if(len > MAXSIZE)
        		remaining = rabin_next_chunk(hash, ptr, chunk, out_table, mod_table, len);
        	else
        		remaining = rabin_next_chunk(hash, ptr, chunk, out_table, mod_table, len);
        		printf("remaining: %d\n", remaining);
        		printf("Chunk_length: %d\n", last_chunk.length);

            if (remaining < 0) {
                break;
            }

            len -= remaining;
            ptr += remaining;

            sha256_init(&ctx);
            sha256_update(&ctx, &chunk[0], remaining);
            sha256_final(&ctx, sha_buf);

            int shaIndex = indexForShaVal(sha_buf);
            if(shaIndex == -1){

#ifdef USING_LZW_HW

                #if MEASURING_LATENCY
            	    numBytesUncompressed += last_chunk.length;
                    unsigned long long lzw_start = sds_clock_counter();
                #endif

				int compress_size = lzwCompressWrapper(&chunk[0], remaining, compress + 4);
				unsigned long long thisTimeForLZW;
                #if MEASURING_LATENCY
                    thisTimeForLZW = sds_clock_counter() - lzw_start;
                    timeInLZW += thisTimeForLZW;
                    numBytesCompressed += compress_size;
                    printf("LZW compressed a chunk from\t[%d] to\t[%d] bytes in\t[%d] cycles\n", remaining, compress_size, thisTimeForLZW);
                #endif
                uint32_t header = (compress_size - 4) << 1;
                memcpy((void*)&compress[0], &header, 1 * sizeof(uint32_t));
#else
                int compress_size = lzwCompress(&chunk[0], remaining, compress);

#endif


                //compareArrays(compress2, compress, compress_size);


#ifdef __SDSCC__
                f_write(&File, compress, compress_size, &bytes_read);
#else
                fwrite(compress, sizeof(uint8_t), compress_size, File);

#endif
            }//if not found in table
            else{
                uint32_t dupPacket = shaIndex;
                dupPacket <<= 1;
                dupPacket |= 0x1;//bit 0 becomes a 1 to indicate a duplicate
#ifdef __SDSCC__
                f_write(&File, &dupPacket, 4, &bytes_read);
#else
                fwrite(&dupPacket, sizeof(uint32_t), 1, File);
#endif

            }//if found in table

            chunks++;
        }//while(true)

    if (rabin_finalize(hash) != NULL) {
        chunks++;

        sha256_init(&ctx);
        sha256_update(&ctx, &chunk[0], last_chunk.length);
        sha256_final(&ctx, sha_buf);
        printf("Chunk_length fhash: %d\n", last_chunk.length);

        int shaIndex = indexForShaVal(sha_buf);
        if(shaIndex == -1){

#ifdef USING_LZW_HW

            int compress_size = lzwCompressWrapper(&chunk[0], last_chunk.length, &compress[4]);
            uint32_t header = (compress_size - 4) << 1;
            memcpy((void*)&compress[0], &header, 1 * sizeof(uint32_t));
#else
            int compress_size = lzwCompress(&chunk[0], last_chunk.length, compress);
#endif



#ifdef __SDSCC__
            f_write(&File, compress, compress_size, &bytes_read);
#else
            fwrite(compress, sizeof(uint8_t), compress_size, File);
            printf("Compress fhash size: %d\n", compress_size);
#endif
        }//if not found in table
        else{
            uint32_t dupPacket = shaIndex;
            dupPacket <<= 1;
            dupPacket |= 0x1;//bit 0 becomes a 1 to indicate a duplicate
#ifdef __SDSCC__
            f_write(&File, &dupPacket, 4, &bytes_read);
#else
            fwrite(&dupPacket, sizeof(uint32_t), 1, File);
#endif

        }//if found in table
    }

#ifdef __SDSCC__
    f_close(&File);
#else
    fclose(File);
#endif
    Free(buf);
    Free(compress);

    unsigned int avg = 0;
    if (chunks > 0)
        avg = bytes / chunks;
    printf("%d chunks, average chunk size %d\n", chunks, avg);

    return 0;
}
