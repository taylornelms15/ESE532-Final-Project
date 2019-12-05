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
#include <pthread.h>


#include "common.h"
#include "hardwareWrapper.h"
#include "rabin.h"

#define READING_FROM_SERVER 1

#define SEPARATE_OUTPUT_THREAD 0

#define __linux__ 1

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


static const char hostInfileName[] = "C:/Users/taylo/csworkspace/ese532/final/Testfiles/linuzPartial.dat";
static const char hostOutfileName[] = "C:/Users/taylo/csworkspace/ese532/final/Testfiles/linuzPartial.compress";
//static const char deviceInfileName[] = "LittlePrince.txt";
char deviceInfileName[50];
static const char deviceOutfileName[] = "compress.dat";
unsigned long long *out_table;
unsigned long long *mod_table;
uint32_t recvBytes = 0;

//Interthread variables
int8_t go_nw = 0;
int8_t go_core = 0;
int8_t go_nw_exit = 0;
uint8_t go_write = 0;
uint8_t end_write = 0;

//Mutexes for interthread variables
pthread_mutex_t m_go_nw;
pthread_mutex_t m_go_nw_exit;
pthread_mutex_t m_go_write;
pthread_mutex_t m_end_write;
pthread_mutex_t m_recvbytes;


uint32_t dataSize = 0;
uint8_t writeFileBuf[OUTBUFFER_SIZE];



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

unsigned long long* AllocateULL(int Size){
    unsigned long long* Frame = (unsigned long long*)
#ifdef __SDSCC__
    sds_alloc(Size * sizeof(unsigned long long));
#else
    malloc(Size * sizeof(unsigned long long));
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

void FreeULL(unsigned long long* Frame){
#ifdef __SDSCC__
    sds_free(Frame);
#else
    free(Frame);
#endif
}

#if READING_FROM_SERVER
//##################
//READ THREAD
//##################
void *read_NW(void *arg) {
	ESE532_Server Server = ESE532_Server();
	Server.setup_server();

    //the arg is a pointer to our data buffer
    unsigned char *Data = (unsigned char *)arg;
    //starting to read into it at the first byte
	unsigned int currentIndex = 0;
	uint8_t server_rd_stp = 0;
	printf("read_NW thread\n");
    while(!server_rd_stp && (currentIndex < MAXINPUTFILESIZE)) {
            //make a little packet buffer, put data into it from server
	      	uint8_t pkt[MAXPKTSIZE+HEADER];
	        uint16_t thisPacketBytes = Server.get_packet(pkt);
            //if we got no more data, we're done, stop reading from server
	        if (thisPacketBytes < 0){
	            printf("Read %d total bytes from network\n", currentIndex + 1);
	            break;
	        }//end of transmission

            //if this packet contains the "stop" bit, mark that we'll stop reading
            //otherwise, use its header to note the total size of the packet
	        if((pkt[1] & 0x80) == 128)
	          server_rd_stp = 1;
	        thisPacketBytes = (((uint16_t)(pkt[1] << 8 | pkt[0])) & 0x7fff) ;
            //copy as many bytes as we need into our data read buffer
	        memcpy(Data + currentIndex, &pkt[HEADER], thisPacketBytes);

            //our read-into-location changes
	        currentIndex += thisPacketBytes;
            //total received bytes increases by the same amount
	        pthread_mutex_lock(&m_recvbytes);
	        recvBytes += thisPacketBytes;
	        pthread_mutex_unlock(&m_recvbytes);
            //if we've received enough bytes to tell the hardware to go again, increase the "number of times to go" counter
	        pthread_mutex_lock(&m_go_nw);
	        if(recvBytes >= (go_nw + 1) * INBUFFER_SIZE) {
	        	go_nw++;
	        }
	        pthread_mutex_unlock(&m_go_nw);


	    }//while
    pthread_mutex_lock(&m_go_nw);
    pthread_mutex_lock(&m_go_nw_exit);
    go_nw++;
	go_nw_exit = 1;
	pthread_mutex_unlock(&m_go_nw_exit);
    pthread_mutex_unlock(&m_go_nw);


	printf("Exiting NW thread\n");
	pthread_exit(NULL);
}
//##################
// END THREAD
//##################
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

#if SEPARATE_OUTPUT_THREAD
//##################
//STORE THREAD
//##################
void *Store_Data_linux_thread(void *arg){
  unsigned int Bytes_written;
  printf("Inside store data linux thread\n");
//  uint32_t dataSize = *(uint32_t *)arg;
  FILE * File = fopen(deviceOutfileName, "wb");
  if (File == NULL){
      printf("Could not open output file\n");
      Exit_with_error();
  }

  //while(end_write != 2) {
  while(1){
	  //if end_write is 2, break
	  pthread_mutex_lock(&m_end_write);
	  if (end_write == 2){
		  pthread_mutex_unlock(&m_end_write);
		  break;
	  }
	  pthread_mutex_unlock(&m_end_write);

	  //if go_write is 0, continue
	  pthread_mutex_lock(&m_go_write);
	  if(go_write == 0)
		  pthread_mutex_unlock(&m_go_write);
		  continue;
      pthread_mutex_unlock(&m_go_write);

      //else, if go_write is not 0:
	  printf("dataSize is %d\n", dataSize);
	  Bytes_written = fwrite(writeFileBuf, 1, dataSize, File);
	  if (Bytes_written < 1){
		  printf("None written, result %d\n", Bytes_written);
		  Exit_with_error();
	  }
	  pthread_mutex_lock(&m_go_write);
	  go_write = 0;
	  pthread_mutex_unlock(&m_go_write);
  }

  if (fclose(File) != 0)
    Exit_with_error();

  printf("Stored data successfully\n");
  printf("Exiting writeFile thread\n");
  pthread_exit(NULL);
}
//##################
// END THREAD
//##################
#endif

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
uint32_t readDataIntoBuffer(uint8_t* hwBuffer, uint8_t *packetBuffer, uint32_t packetOffset, uint32_t recvBytesCur){
	 uint32_t remainingSize = recvBytesCur - (packetOffset);
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

	printf("VERSION NUMBER 0.0.4\n");

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

    //#############################
    // MEMORY ALLOCATIONS
    //#############################
    uint8_t* chunkTable = Allocate(SHA256TABLESIZE);
    printf("Allocated chunkTable at %p\n", chunkTable);
    uint8_t* hwBuffer = Allocate(INBUFFER_SIZE);
    printf("Allocated hardware processing buffer at %p\n", hwBuffer);
    uint8_t* output = Allocate(OUTBUFFER_SIZE);//will eventually write this to a file
    uint32_t outputOffset = 0;
    printf("Allocated output memory location at %p\n", output);
    out_table = AllocateULL(256);
    printf("Allocated out_table memory location at %p\n", out_table);
    mod_table = AllocateULL(256);
    printf("Allocated mod_table memory location at %p\n", mod_table);
    #if READING_FROM_SERVER
    uint8_t* packetBuffer 	= Allocate(MAXINPUTFILESIZE);
    #endif
    #if !SEPARATE_OUTPUT_THREAD
    uint8_t* writeout_buffer = (uint8_t*)malloc(MAXINPUTFILESIZE);
    #endif


    resetTable(chunkTable);

    #ifdef __SDSCC__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(2, &cpuset);

    #if SEPARATE_OUTPUT_THREAD
    cpu_set_t cpuset2;
    CPU_ZERO(&cpuset2);
    CPU_SET(3, &cpuset2);
    #endif
    #endif


#if READING_FROM_SERVER
    pthread_t nwId;

    //calls read_NW in separate thread
    int thread_ret = pthread_create(&nwId, NULL, &read_NW, packetBuffer);
    if(thread_ret < 0) {
        printf("pthread create error\n");
        exit(0);
    }
    thread_ret = pthread_setaffinity_np(nwId, sizeof(cpu_set_t), &cpuset);
        if(thread_ret < 0) {
        	printf("pthread set affinity error\n");
        	exit(0);
    }


    #if SEPARATE_OUTPUT_THREAD
    pthread_t writeFileId;
    //calls Store_Data_linux_thread in a separate thread
    thread_ret = pthread_create(&writeFileId, NULL, &Store_Data_linux_thread, packetBuffer);
        if(thread_ret < 0) {
          printf("pthread create error\n");
          exit(0);
        }
    thread_ret = pthread_setaffinity_np(writeFileId, sizeof(cpu_set_t), &cpuset2);
        if(thread_ret < 0) {
            printf("pthread set affinity error\n");
            exit(0);
        }
    #endif


   //   	read_NW(packetBuffer);
    uint32_t packetOffset = 0;
    uint8_t measure_flag = 0;
#else
    uint8_t* fileBuffer 	= Allocate(MAXINPUTFILESIZE);
    uint32_t fileSize 		= Load_data(fileBuffer);
    uint32_t fileOffset 	= 0;
#endif

unsigned long long overallStart;

#if !READING_FROM_SERVER
	#if MEASURING_LATENCY
        		overallStart = sds_clock_counter();
	#endif
#endif

    uint32_t currentDictIndex = 0;
    uint32_t outputDictIndex = 0;

    //MAIN LOOP
    while(true) {
    #if READING_FROM_SERVER

    	pthread_mutex_lock(&m_go_nw);
    	pthread_mutex_lock(&m_go_nw_exit);
        if(go_core >= go_nw && go_nw_exit != 1){
            pthread_mutex_unlock(&m_go_nw_exit);
            pthread_mutex_unlock(&m_go_nw);
            continue;
        }
        pthread_mutex_unlock(&m_go_nw_exit);
        pthread_mutex_unlock(&m_go_nw);

        pthread_mutex_lock(&m_recvbytes);
        uint32_t recvBytesCur = recvBytes;
        pthread_mutex_unlock(&m_recvbytes);

	    #if MEASURING_LATENCY
        	if(measure_flag == 0) {
        		overallStart = sds_clock_counter();
        		measure_flag = 1;
        	}
	    #endif
    #endif

    	uint32_t nextBufferSize =
        #if READING_FROM_SERVER
            readDataIntoBuffer(hwBuffer, packetBuffer, packetOffset, recvBytesCur);
        	packetOffset += nextBufferSize;
        	go_core++;

        #else
            readDataIntoBuffer(hwBuffer,fileBuffer, fileOffset, fileSize);
            fileOffset += nextBufferSize;
        #endif
        if (nextBufferSize == 0){
            pthread_mutex_lock(&m_end_write);
            end_write = 2;
            pthread_mutex_unlock(&m_end_write);
        	break;
        }
        printf("Starting processing on buffer of size %d\n", nextBufferSize);
        //#########################
        // ACTUAL HARDWARE CALL
        //#########################
        uint32_t hwOutputSize = processBuffer(hwBuffer, output, chunkTable, nextBufferSize,
                                              out_table, mod_table,
                                              currentDictIndex, &outputDictIndex);
        currentDictIndex = outputDictIndex;//update between runs

    #if SEPARATE_OUTPUT_THREAD
        memcpy(writeFileBuf, output, hwOutputSize);
        dataSize = hwOutputSize;

        //stall while go_write is 1 (???????)

        while(go_write == 1);
        	go_write = 1;
    #else
        memcpy(writeout_buffer + outputOffset, output, hwOutputSize);
    #endif

        outputOffset += hwOutputSize;
        printf("Processed buffer, ending size %d\n", hwOutputSize);
    }

    #if MEASURING_LATENCY
    unsigned long long overallEnd = sds_clock_counter();
    printf("Overall latency %lld\n", overallEnd - overallStart);
    #endif

#if !SEPARATE_OUTPUT_THREAD
    Store_Data(writeout_buffer, outputOffset);//TODO: maybe add +1?
    printf("Stored data successfully\n");
    free(writeout_buffer);
#endif

    Free(chunkTable);
    Free(hwBuffer);
    Free(output);
    FreeULL(out_table);
    FreeULL(mod_table);
#if READING_FROM_SERVER
    Free(packetBuffer);
    pthread_join(nwId, NULL);
    #if SEPARATE_OUTPUT_THREAD
    pthread_join(writeFileId, NULL);
    #endif
#else
    Free(fileBuffer);
#endif

#if MEASURING_LATENCY
    printf("recvBytes : %d Bytes\n", recvBytes);
    double timeTaken = (overallEnd - overallStart) / (1.2 * 1000000000);//in seconds
    double numMegabits = recvBytes * 8.0 / 1000000.0;//using 1MB=1e6, rather than 2^20, here
    float throughputMb = numMegabits / timeTaken;
    printf("throughput achieved : %f Mbps\n", throughputMb);
#endif
    return 0;
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
