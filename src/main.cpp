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


#include "common.h"

// 1MiB buffer
uint8_t* buf;
size_t bytes;
static const char infileName[] = "/Users/taylo/csworkspace/ese532/final/Testfiles/Franklin.txt";
static const char outfileName[] = "/Users/taylo/csworkspace/ese532/final/Testfiles/Franklin.dat";
static const char outfileNameGold[] = "/Users/taylo/csworkspace/ese532/final/Testfiles/Franklin.datgold";

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



unsigned int Load_data(unsigned char * Data)
{
  unsigned int Size = MAXINPUTFILESIZE;
  unsigned int Bytes_read;

#ifdef __SDSCC__
  FIL File;

  FRESULT Result = f_open(&File, "Input.bin", FA_READ);
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
    uint8_t compress[MAXSIZE];
    uint8_t compress2[MAXSIZE];
    printf("Starting main function\n");

    buf = (uint8_t*)malloc(MAXINPUTFILESIZE * sizeof(uint8_t));

#ifdef __SDSCC__
    FATFS FS;
    unsigned int bytes_read;

    Check_error(f_mount(&FS, "0:/", 0) != FR_OK, "Could not mount SD-card");
#endif

    unsigned int len = Load_data(buf);

#ifdef __SDSCC__
    FIL File;

    FRESULT Result = f_open(&File, "Output.bin", FA_WRITE | FA_CREATE_ALWAYS);
    Check_error(Result != FR_OK, "Could not open output file.");

#else
    FILE* File = fopen(outfileName, "wb");
    if (File == NULL)
        Exit_with_error();
    FILE* FileGold = fopen(outfileNameGold, "wb");
    if (FileGold == NULL)
        Exit_with_error();

#endif

    uint8_t *ptr = buf;
    bytes += len;

        while (1) {
            int remaining = rabin_next_chunk(hash, ptr, len);

            if (remaining < 0) {
                break;
            }

            len -= remaining;
            ptr += remaining;

            sha256_init(&ctx);
            sha256_update(&ctx, &buf[last_chunk.start], last_chunk.length); 
            sha256_final(&ctx, sha_buf);

            int shaIndex = indexForShaVal(sha_buf);
            if(shaIndex == -1){
                //printf("Input:\t[0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x...\n\tSize:%d\n",
                //		buf[last_chunk.start + 0], buf[last_chunk.start + 1], buf[last_chunk.start + 2],
				//		buf[last_chunk.start + 3], buf[last_chunk.start + 4], buf[last_chunk.start + 5],
				//		last_chunk.length);
                int compress_size = lzwCompress(&buf[last_chunk.start], last_chunk.length, compress);
                int compress_size2 = lzwCompressWrapper(&buf[last_chunk.start], last_chunk.length, &compress2[4]);

                printf("compress_size [%d],\tcompress_size2 [%d]\n", compress_size, compress_size2);
                uint32_t header = (compress_size2 - 4) << 1;
                memcpy((void*)&compress2[0], &header, 1 * sizeof(uint32_t));
                header = (compress_size - 4) << 1;
                memcpy((void*)&compress[0], &header, 1 * sizeof(uint32_t));//shouldn't be necessary, is for some reason
                compareArrays(compress2, compress, compress_size);
#ifdef __SDSCC__
                //f_write(&File, compress, compress_size, &bytes_read);
                f_write(&File, compress2, compress_size2, &bytes_read);
#else
                fwrite(compress, sizeof(uint8_t), compress_size, FileGold);
                fwrite(compress2, sizeof(uint8_t), compress_size2, File);
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
                fwrite(&dupPacket, sizeof(uint32_t), 1, FileGold);
#endif

            }//if found in table

            chunks++;
        }//while(true)

    if (rabin_finalize(hash) != NULL) {
        chunks++;

        sha256_init(&ctx);
        sha256_update(&ctx, &buf[last_chunk.start], last_chunk.length);
        sha256_final(&ctx, sha_buf);


        int shaIndex = indexForShaVal(sha_buf);
        if(shaIndex == -1){
            int compress_size = lzwCompress(&buf[last_chunk.start], last_chunk.length, compress);
#ifdef __SDSCC__
            f_write(&File, compress, compress_size, &bytes_read);
#else
            fwrite(compress, sizeof(uint8_t), compress_size, File);
            fwrite(compress, sizeof(uint8_t), compress_size, FileGold);
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
            fwrite(&dupPacket, sizeof(uint32_t), 1, FileGold);
#endif

        }//if found in table
    }

#ifdef __SDSCC
    f_close(&File);
#else
    fclose(File);
    fclose(FileGold);
#endif
    free(buf);

    unsigned int avg = 0;
    if (chunks > 0)
        avg = bytes / chunks;
    printf("%d chunks, average chunk size %d\n", chunks, avg);

    return 0;
}
