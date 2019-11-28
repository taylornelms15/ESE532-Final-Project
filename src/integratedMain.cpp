/**
 * @file integratedMain.cpp
 * @author Taylor Nelms
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


#include "common.h"
#include "hardwareWrapper.h"
#include "rabin.h"

//#define READING_FROM_SERVER 1
//#define __linux__

#if READING_FROM_SERVER
#include "server.h"
#endif


#ifdef __SDSCC__
#ifdef __linux__
#else
#include <ff.h>
#endif
#include <sds_lib.h>
#endif

#ifdef __SDSCC__
#define MEASURING_LATENCY 1
#else
#define MEASURING_LATENCY 0
#endif


static const char hostInfileName[] = "C:/Users/rgjus/Desktop/franklin.txt";
static const char hostOutfileName[] = "C:/Users/rgjus/Desktop/op_franklin.txt";
static const char gold_hostOutfileName[] = "/home/nishanth/University/ESE_532/Final_project/HLS/ESE532-Final-Project/Testfiles/LittlePrince_golden.compress";
//static const char deviceInfileName[] = "LittlePrince.txt";
char deviceInfileName[50];
static const char deviceOutfileName[] = "compress.dat";
unsigned long long *out_table;
unsigned long long *mod_table;

void resetTable(uint8_t tableLocation[SHA256TABLESIZE]);

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

#if READING_FROM_SERVER
unsigned int read_NW(unsigned char *Data) {
	ESE532_Server Server = ESE532_Server();
	Server.setup_server();
	unsigned int bytes_read = 0;
	unsigned int currentIndex = 0;
	uint8_t server_rd_stp = 0;
	while(!server_rd_stp && (currentIndex < MAXINPUTFILESIZE)) {
	      	uint8_t pkt[MAXPKTSIZE+HEADER];
	        uint16_t thisPacketBytes = Server.get_packet(pkt);
	        if (thisPacketBytes < 0){
	            printf("Read %d total bytes from network\n", currentIndex + 1);
	            break;
	        }//end of transmission

	        if((pkt[1] & 0x80) == 128)
	          server_rd_stp = 1;
	        thisPacketBytes = (((uint16_t)(pkt[1] << 8 | pkt[0])) & 0x7fff) ;
	        printf("pkt[0] : %x\n", pkt[0]);
			printf("pkt[1] : %x\n", pkt[1]);
	        printf("pkt len: %d\n", thisPacketBytes);
	        memcpy(Data + currentIndex, &pkt[HEADER], thisPacketBytes);
	        currentIndex += thisPacketBytes;
	        bytes_read += thisPacketBytes;

	    }//while

	return bytes_read;
}
#endif

unsigned int Load_data_linux(unsigned char*  Data, const char* fileName){
  unsigned int Bytes_read;
  unsigned int Size = MAXINPUTFILESIZE;
  FILE * File = fopen(fileName, "rb");
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
  return Bytes_read;
}

unsigned int Load_data(unsigned char * Data)
{
  unsigned int Size = MAXINPUTFILESIZE;
  unsigned int Bytes_read;

#ifdef __SDSCC__
#ifdef __linux__
  Bytes_read = Load_data_linux(Data, deviceInfileName);
#else

  FIL File;

  FRESULT Result = f_open(&File, deviceInfileName, FA_READ);
  if (Result != FR_OK){
    printf("File open: Result %d\n", Result);
  }
  Check_error(Result != FR_OK, "Could not open input file (SDSCC).");
  Result = f_read(&File, Data, Size, &Bytes_read);
  Check_error(Result != FR_OK, "Could not read input file.");
  Check_error(f_close(&File) != FR_OK, "Could not close input file.");
#endif
#else
  Bytes_read = Load_data_linux(Data, hostInfileName);
  printf("Read %d bytes\n", Bytes_read);
#endif
  return Bytes_read;
}

unsigned int Store_Data_linux(uint8_t* Data, uint32_t dataSize, const char* fileName){
  unsigned int Bytes_written;
  FILE * File = fopen(fileName, "wb");
  if (File == NULL){
      printf("Could not open output file\n");
      Exit_with_error();
  }

  Bytes_written = fwrite(Data, 1, dataSize, File);
  if (Bytes_written < 1){
      printf("None written, result %d\n", Bytes_written);
      Exit_with_error();
  }

  if (fclose(File) != 0)
    Exit_with_error();
  return Bytes_written;
}

unsigned int Store_Data(uint8_t* Data, uint32_t dataSize){
  unsigned int Bytes_written;
#ifdef __SDSCC__
#ifdef __linux__
  Bytes_written = Store_Data_linux(Data, dataSize, deviceOutfileName);
#else
  FIL File;

  FRESULT Result = f_open(&File, deviceOutfileName, FA_WRITE | FA_CREATE_ALWAYS);
  if (Result != FR_OK){
    printf("Output code %d\n", Result);
  }
  Check_error(Result != FR_OK, "Could not open output file.");
  Result = f_write(&File, Data, dataSize, &Bytes_written);
  Check_error(Result != FR_OK, "Could not read output file.");
  Check_error(f_close(&File) != FR_OK, "Could not close output file.");
#endif
#else
  Bytes_written = Store_Data_linux(Data, dataSize, hostOutfileName);
#endif
  return Bytes_written;
}//Store_Data

/**
 * This function encapsulates reading the next bits of data into our buffer for hardware processing
 * It will either read from an input file, or read from the server, depending on the defines
 *
 * @return Number of elements read into our hardware buffer, or 0 if we've hit the end.
 */
#if READING_FROM_SERVER
uint32_t readDataIntoBuffer(uint8_t* hwBuffer, uint8_t *packetBuffer, uint32_t packetOffset, uint32_t recvBytes){
	 uint32_t remainingSize = recvBytes - (packetOffset);
	    if (remainingSize == 0) return 0;
	    else if (remainingSize > INBUFFER_SIZE && (remainingSize % INBUFFER_SIZE < MINSIZE)){
	        memcpy(hwBuffer, packetBuffer + packetOffset, remainingSize / 2);
	        return remainingSize / 2;
	    }//need to have a smaller buffer this time
	    else if (remainingSize < INBUFFER_SIZE){
	        memcpy(hwBuffer, packetBuffer + packetOffset, remainingSize);
	        return remainingSize;
	    }
	    else{
	        memcpy(hwBuffer, packetBuffer + packetOffset, INBUFFER_SIZE);
	        return INBUFFER_SIZE;
	    }
#else
uint32_t readDataIntoBuffer(uint8_t* hwBuffer, uint8_t* fileBuffer, uint32_t fileOffset, uint32_t fileSize){
    uint32_t remainingSize = fileSize - (fileOffset);
    if (remainingSize == 0) return 0;
    else if (remainingSize > INBUFFER_SIZE && (remainingSize % INBUFFER_SIZE < MINSIZE)){
        memcpy(hwBuffer, fileBuffer + fileOffset, remainingSize / 2);
        return remainingSize / 2;
    }//need to have a smaller buffer this time
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

    #ifdef __SDSCC__//mount SD card
    #ifndef __linux__
    FATFS FS;
    Check_error(f_mount(&FS, "0:/", 0) != FR_OK, "Could not mount SD-card");
    #endif
    #endif

    if(argc == 2)
    	strcpy(deviceInfileName, argv[1]);
    else
    	strcpy(deviceInfileName, "LittlePrince.txt");

    uint8_t* chunkTable = Allocate(SHA256TABLESIZE);
    printf("Allocated chunkTable at %p\n", chunkTable);
    uint8_t* hwBuffer = Allocate(INBUFFER_SIZE);
    printf("Allocated hardware processing buffer at %p\n", hwBuffer);
    uint8_t* output = Allocate(MAXINPUTFILESIZE);//will eventually write this to a file
    uint32_t outputOffset = 0;
    printf("Allocated output memory location at %p\n", output);
    out_table = (unsigned long long *)Allocate(sizeof(uint64_t) * 256);
    printf("Allocated output memory location at %p\n", out_table);
    mod_table =  (unsigned long long *)Allocate(sizeof(uint64_t) * 256);
    printf("Allocated output memory location at %p\n", mod_table);

    struct rabin_t *hash = rabin_init();
    resetTable(chunkTable);

#if READING_FROM_SERVER
    uint8_t* packetBuffer 	= Allocate(MAXINPUTFILESIZE);
    uint32_t recvBytes =  	read_NW(packetBuffer);
    uint32_t packetOffset = 0;
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
            readDataIntoBuffer(hwBuffer, packetBuffer, packetOffset, recvBytes);
        	packetOffset += nextBufferSize;
        #else
            readDataIntoBuffer(hwBuffer,fileBuffer, fileOffset, fileSize);
            fileOffset += nextBufferSize;
        #endif
        if (nextBufferSize == 0){
            break;
        }
        printf("Starting processing on buffer of size %d\n", nextBufferSize);
        uint32_t hwOutputSize = processBuffer(hwBuffer, output + outputOffset, chunkTable, nextBufferSize, out_table, mod_table);
        outputOffset += hwOutputSize;
        printf("Processed buffer, ending size %d\n", hwOutputSize);
    }

    #if MEASURING_LATENCY
    unsigned long long overallEnd = sds_clock_counter();
    printf("Overall latency %lld\n", overallEnd - overallStart);
    #endif

    Store_Data(output, outputOffset + 1);
    printf("Stored data successfully\n");

    Free(chunkTable);
    Free(hwBuffer);
    Free(output);
    Free((uint8_t*)out_table);
    Free((uint8_t*)mod_table);
#if !READING_FROM_SERVER
    Free(fileBuffer);
#else
    Free(packetBuffer);
#endif
    /*
    char diff_str[250];
    sprintf(diff_str, "diff -w %s %s", hostOutfileName, gold_hostOutfileName);
    int ret = system(diff_str);
    printf("diff_str : %s\n", diff_str);
    printf("ret: %d\n", ret);
*/
    //return ret;
    return 1;
}//main


/*
 * These are extra software functions, moving them out of hardware cpp files
 *
 */

/**
 * Software clear of table contents
 */
void resetTable(uint8_t tableLocation[SHA256TABLESIZE]){
    memset(tableLocation, 0xFF, SHA256TABLESIZE);
}

