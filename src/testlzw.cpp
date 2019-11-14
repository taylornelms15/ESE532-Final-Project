/**
 * @file teslzw.cpp
 * @author Taylor Nelms
 */

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


static const char infileName[] = "/Users/taylo/csworkspace/ese532/final/Testfiles/Franklin.txt";
static const char outfileName[] = "/Users/taylo/csworkspace/ese532/final/Testfiles/Franklin.dat";
static const char outfileNameGold[] = "/Users/taylo/csworkspace/ese532/final/Testfiles/Franklin.datgold";

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
  unsigned int Size = 16000;
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

int main(int argc, char* argv[]){

	static const int chunkSize = 1800;

	uint8_t* buf = (uint8_t*)malloc(16000 * sizeof(uint8_t));
	unsigned int len = Load_data(buf);

	uint8_t* compress = (uint8_t*) malloc((chunkSize + 4) * sizeof(uint8_t));
	uint8_t* compress2 = (uint8_t*) malloc((chunkSize + 4) * sizeof(uint8_t));

	printf("Loaded the file probably, length %d\n", len);

	int compress_size, compress_size2;

    compress_size = lzwCompress(buf, chunkSize, compress);
    compress_size2 = lzwCompressWrapper(buf, chunkSize, &compress2[4]);

    printf("compress_size [%d],\tcompress_size2 [%d]\n", compress_size, compress_size2);
    uint32_t header = (compress_size2 - 4) << 1;
    memcpy((void*)&compress2[0], &header, 1 * sizeof(uint32_t));
    header = (compress_size - 4) << 1;
    memcpy((void*)&compress[0], &header, 1 * sizeof(uint32_t));//shouldn't be necessary, is for some reason
    int retval = compareArrays(compress2, compress, compress_size);




    free(compress);
    free(compress2);
	free(buf);
	return retval;
}//main
