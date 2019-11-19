/**
 * @file integratedMain.cpp
 * @author Taylor Nelms
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>


#include "common.h"
#include "hardwareWrapper.h"
#include "chunkdict_hw.h"

#define READING_FROM_SERVER 0

#if READING_FROM_SERVER
#include "server.h"
#endif


#ifdef __SDSCC__
#include <ff.h>
#include <sds_lib.h>
#endif

#ifdef __SDSCC__
#define MEASURING_LATENCY 1
#else
#define MEASURING_LATENCY 0
#endif


static const char infileName[] = "/Users/taylo/csworkspace/ese532/final/Testfiles/Franklin.txt";
static const char outfileName[] = "/Users/taylo/csworkspace/ese532/final/Testfiles/Franklin.dat";
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

/**
 * This function encapsulates reading the next bits of data into our buffer for hardware processing
 * It will either read from an input file, or read from the server, depending on the defines
 *
 * @return Number of elements read into our hardware buffer, or 0 if we've hit the end.
 */
#if READING_FROM_SERVER
uint32_t readDataIntoBuffer(uint8_t* hwBuffer){
    //TODO: put in server reads here
#else
uint32_t readDataIntoBuffer(uint8_t* hwBuffer, uint8_t* fileBuffer, uint32_t fileOffset, uint32_t fileSize){
    uint32_t remainingSize = fileSize - (fileOffset + 1);
    if (remainingSize == 0) return 0;
    else if (remainingSize < INBUFFER_SIZE){
        memcpy(hwBuffer, fileBuffer + fileOffset, remainingSize);
        return remainingSize;
    }
    else{
        memcpy(hwBuffer, fileBuffer + fileOffset, INBUFFER_SIZE);
        return INBUFFER_SIZE;
    }
#endif
}//readDataIntoBuffer


int main(int argc, char* argv[]){

    uint8_t* chunkTable = Allocate(SHA256TABLESIZE);
    printf("Allocated chunkTable at %p\n", chunkTable);
    uint8_t* hwBuffer = Allocate(INBUFFER_SIZE);
    printf("Allocated hardware processing buffer at %p\n", hwBuffer);
    uint8_t* output = Allocate(MAXINPUTFILESIZE);//will eventually write this to a file
    uint32_t outputOffset = 0;
    printf("Allocated output memory location at %p\n", output);

    resetTable(chunkTable);

#if READING_FROM_SERVER
    //TODO: init server here
#else
    uint8_t* fileBuffer 	= Allocate(MAXINPUTFILESIZE);
    uint32_t fileSize 		= Load_data(fileBuffer);
    uint32_t fileOffset 	= 0;
#endif
	
    #if MEASURING_LATENCY
    unsigned long long overallStart = sds_clock_counter();
    #endif

    while(true){
        uint32_t nextBufferSize =
        #if READING_FROM_SERVER
                readDataIntoBuffer(hwBuffer);
        #else
                readDataIntoBuffer(hwBuffer,fileBuffer, fileOffset, fileSize);
                fileOffset += nextBufferSize;
        #endif
        if (nextBufferSize == 0){
            break;
        }
        uint32_t hwOutputSize = processBuffer(hwBuffer, output + outputOffset, chunkTable, nextBufferSize);
        outputOffset += hwOutputSize;
    }

    #if MEASURING_LATENCY
    unsigned long long overallEnd = sds_clock_counter();
    printf("Overall latency %lld\n", overallEnd - overallStart;
    #endif

    Free(chunkTable);
    Free(hwBuffer);
#if !READING_FROM_SERVER
    Free(fileBuffer);
#endif

    return 0;
}//main

