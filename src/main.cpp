#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "common.h"

extern "C"
{
#include "rabin.h"
#include "sha_256.h"
//
#include "lzw_sw.h"
#include "chunkdict.h"
}
#include "sha256_hw.h"

//#include "lzw_hw.h"
#ifdef __SDSCC__
#include <ff.h>
#include <sds_lib.h>
#endif

//#define USING_LZW_HW


// 1MiB buffer
uint8_t* buf;
size_t bytes;
static const char infileName[] = "C:/Users/rgjus/Desktop/test.txt";
static const char outfileName[] = "C:/Users/rgjus/Desktop/output.txt";

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

  FRESULT Result = f_open(&File, "ucomp.txt", FA_READ);
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
bool compare(BYTE data1[SHA256_BLOCK_SIZE], BYTE data2[SHA256_BLOCK_SIZE])
{
	for (int i = 0; i < SHA256_BLOCK_SIZE; i++)
	{
		if (data1[i] != data2[i])
		{
			printf("outputs didn't match\n");
			return false;
		}
	}

	printf("outputs matched\n");
	return true;
}

int main(int argc, char *argv[]) {

#ifdef SHA_TEST
	SHA256_CTX ctx;
	const BYTE buf[] = {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopqabcdefghijhdsahd 63244e3demdbdbdcds cksjdhbr4reiwurfdnscmdncuiyu543nmnd kjsdnl;klsfd"};
	unsigned int chunks = 0;
	BYTE sha_buf[SHA256_BLOCK_SIZE];
	BYTE sha_buf_hw[SHA256_BLOCK_SIZE];

	sha256_hw_compute(buf, strlen((const char*)buf), sha_buf_hw);

	sha256_init(&ctx);
	sha256_update(&ctx, buf, strlen((const char*)buf));
	sha256_final(&ctx, sha_buf);

	bool res = compare(sha_buf, sha_buf_hw);

	if (res)
		printf("outputs matched\n");
	else
		printf("outputs didn't match\n");

#else
	struct rabin_t *hash;
    hash = rabin_init();
    SHA256_CTX ctx;
    unsigned int chunks = 0;
    BYTE sha_buf[SHA256_BLOCK_SIZE];
    BYTE sha_buf_hw[SHA256_BLOCK_SIZE];
    uint8_t compress[MAXSIZE];

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

    FRESULT Result = f_open(&File, "compress.dat", FA_WRITE | FA_CREATE_ALWAYS);
    Check_error(Result != FR_OK, "Could not open output file.");

#else
    FILE* File = fopen(outfileName, "wb");
    if (File == NULL)
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
//#ifdef USING_SHA_HW
            sha256_hw_compute(&buf[last_chunk.start], last_chunk.length, sha_buf_hw);
//#else
            sha256_init(&ctx);
            sha256_update(&ctx, &buf[last_chunk.start], last_chunk.length); 
            sha256_final(&ctx, sha_buf);

            compare(sha_buf, sha_buf_hw);
//#endif
            int shaIndex = indexForShaVal(sha_buf);
            if(shaIndex == -1){
#ifdef USING_LZW_HW
                int compress_size = lzwCompressWrapper(&buf[last_chunk.start], last_chunk.length, &compress[4]);
                uint32_t header = (compress_size - 4) << 1;
                memcpy((void*)&compress[0], &header, 1 * sizeof(uint32_t));
#else
                int compress_size = lzwCompress(&buf[last_chunk.start], last_chunk.length, compress);
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

//#ifdef USING_SHA_HW
        sha256_hw_compute(&buf[last_chunk.start], last_chunk.length, sha_buf_hw);
//#else
        sha256_init(&ctx);
        sha256_update(&ctx, &buf[last_chunk.start], last_chunk.length);
        sha256_final(&ctx, sha_buf);

        compare(sha_buf, sha_buf_hw);
//#endif


        int shaIndex = indexForShaVal(sha_buf);
        if(shaIndex == -1){
#ifdef USING_LZW_HW
            int compress_size = lzwCompressWrapper(&buf[last_chunk.start], last_chunk.length, &compress[4]);
            uint32_t header = (compress_size - 4) << 1;
            memcpy((void*)&compress[0], &header, 1 * sizeof(uint32_t));
#else
            int compress_size = lzwCompress(&buf[last_chunk.start], last_chunk.length, compress);
#endif


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
    }

#ifdef __SDSCC
    f_close(&File);
#else
    fclose(File);
#endif
    free(buf);

    unsigned int avg = 0;
    if (chunks > 0)
        avg = bytes / chunks;
    printf("%d chunks, average chunk size %d\n", chunks, avg);

#endif // SHA_TEST

    return 0;
}
