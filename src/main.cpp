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
#include <string.h>
}
#include "lzw_hw.h"
#include "sha256_hw.h"
#include "chunkdict_hw.h"
#ifdef __SDSCC__
#include <sds_lib.h>
#endif

#ifdef __SDSCC__
#define MEASURING_LATENCY 1
#else
#define MEASURING_LATENCY 0
#endif
#define READING_FROM_SERVER 1

#if READING_FROM_SERVER
#include "server.h"
#endif


// 1MiB buffer
uint8_t* buf;
size_t bytes;
static const char infileName[] = "C:/Users/rgjus/Desktop/test.txt";
static const char outfileName[] = "compress.dat";

extern unsigned long long mod_table[256];
extern unsigned long long out_table[256];
//static const char infileName[] = "/home/nishanth/University/ESE_532/Final_project/HLS/ESE532-Final-Project/Testfiles/Franklin.txt";
//static const char outfileName[] = "/home/nishanth/University/ESE_532/Final_project/HLS/ESE532-Final-Project/Testfiles/compress.dat";
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



//Commenting out Load_data since Linux implementation uses read from server.
#if READING_FROM_SERVER

#else

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
#endif

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
    BYTE sha_buf_hw[SHA256_BLOCK_SIZE];
    BYTE sha_buf[SHA256_BLOCK_SIZE];

#if READING_FROM_SERVER
    //Server things
    ESE532_Server Server = ESE532_Server();
    Server.setup_server();
#endif

    //Allocation things
    uint8_t* compress = Allocate(MAXSIZE + 4);
    printf("Compress allocated at %x\n", compress);

#ifdef USING_CHUNKDICT_HW
    uint8_t* chunkDictTable = Allocate(SHA256TABLESIZE);
#endif

    printf("Starting main function\n");

    //Forcefully using malloc, since sds_alloc cannot allocate 256MB of contiguous memory
    buf = (uint8_t *)malloc(MAXINPUTFILESIZE * sizeof(uint8_t));
    printf("buf allocated at %x\n", buf);



    
#if MEASURING_LATENCY
    unsigned long long numBytesUncompressed = 0;//number of bytes fed into LZW
    unsigned long long numBytesCompressed   = 0;//number of bytes fed out of LZW
    unsigned long long timeInLZW            = 0;//how many cycles we spent in LZW
    uint64_t rabin_start = 0;
    uint64_t rabin_end = 0;
    uint64_t rabin_dur = 0;
    uint64_t sha_start = 0;
    uint64_t sha_end = 0;
    uint64_t sha_dur = 0;
    uint64_t dedup_start = 0;
    uint64_t dedup_end = 0;
    uint64_t dedup_dur = 0;
    uint64_t lzw_start = 0;
    uint64_t lzw_end = 0;
    uint64_t lzw_dur = 0;
#endif

//Doing a single read till end of packets to reduce complexity as of now.
#if READING_FROM_SERVER
    //Fill buffer from server
    uint32_t currentIndex = 0;
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
        memcpy(buf + currentIndex, &pkt[HEADER], thisPacketBytes);
        currentIndex += thisPacketBytes;
        bytes += thisPacketBytes;
        
    }//while
    unsigned int len = currentIndex;

#else
    //read data from file system
    unsigned int len = Load_data(buf);
    bytes += len;

#ifdef __SDSCC__
    FATFS FS;

    Check_error(f_mount(&FS, "0:/", 0) != FR_OK, "Could not mount SD-card");
#endif



#endif//not reading from server

    //Set up output file

#if READING_FROM_SERVER
 	FILE* File = fopen(outfileName, "wb");
    if (File == NULL)
        Exit_with_error();
#else
	#ifdef __SDSCC__
    	FIL File;
    	unsigned int bytes_written;

    	FRESULT Result = f_open(&File, linuxOutfileName, FA_WRITE | FA_CREATE_ALWAYS);
    	Check_error(Result != FR_OK, "Could not open output file.");
	#else
    	FILE* File = fopen(outfileName, "wb");
        if (File == NULL)
            Exit_with_error();
	#endif


#endif


    //actually run our program

    uint8_t *ptr = buf;

    //sds_alloc needed for arguments to HW functions 
    uint8_t *chunk = Allocate(MAXSIZE);
    int remaining;
    unsigned int count = hash->count;
    unsigned int pos = hash->pos;
    uint64_t digest = hash->digest;
    unsigned int buf_len = 0;

    //   printf("len: %d\n", len);
        while (len > 0) {
#if MEASURING_LATENCY
        rabin_start = sds_clock_counter();
#endif

#ifdef USING_RABIN_HW
        	if(len >= MAXSIZE) {
        	  buf_len = MAXSIZE;	
          } else {
            buf_len = len;
          }
            remaining = rabin_next_chunk_HW(ptr, chunk, out_table, mod_table, buf_len);


#else
        	if(len >= MAXSIZE)
        		remaining = rabin_next_chunk_SW(hash, ptr, chunk, out_table, mod_table, MAXSIZE);
        	else
        		remaining = rabin_next_chunk_SW(hash, ptr, chunk, out_table, mod_table, len);

#endif
#if MEASURING_LATENCY
        rabin_end = sds_clock_counter();
        rabin_dur += rabin_end - rabin_start;
#endif

            if (remaining < 0) {
                break;
            }

            len -= remaining;
            ptr += remaining;
#ifdef USING_SHA_HW
#if MEASURING_LATENCY
        sha_start = sds_clock_counter();
#endif
            sha256_hw_compute(&chunk[0], remaining, sha_buf_hw);
            printf("hash values: \n");
            	for(int i = 0; i < SHA256_BLOCK_SIZE; i++)
            		printf("%x ", sha_buf_hw[i]);
            	printf("\n");
#if MEASURING_LATENCY
        sha_end = sds_clock_counter();
        sha_dur += sha_end - sha_start;
#endif
#else

            sha256_init(&ctx);
            sha256_update(&ctx, &chunk[0], remaining);
            sha256_final(&ctx, sha_buf);

            //compare(sha_buf, sha_buf_hw);
#endif
#if MEASURING_LATENCY
        dedup_start = sds_clock_counter();
#endif

#ifdef USING_CHUNKDICT_HW
            int shaIndex = indexForShaVal_HW(sha_buf_hw, chunkDictTable);
#else
            int shaIndex = indexForShaVal(sha_buf_hw);
#endif

#if MEASURING_LATENCY
        dedup_end = sds_clock_counter();
        dedup_dur += dedup_end - dedup_start;
#endif
            if(shaIndex == -1){

            	printf("duplicate packet not found\n\n");
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
#if MEASURING_LATENCY
        lzw_start = sds_clock_counter();
#endif
                int compress_size = lzwCompress(&chunk[0], remaining, compress);
#if MEASURING_LATENCY
        lzw_end = sds_clock_counter();
        lzw_dur += lzw_end - lzw_start;
#endif

#endif


                //compareArrays(compress2, compress, compress_size);


#if READING_FROM_SERVER
                fwrite(compress, sizeof(uint8_t), compress_size, File);
#else
	#ifdef __SDSCC__
                f_write(&File, compress, compress_size, &bytes_written);
	#else
                fwrite(compress, sizeof(uint8_t), compress_size, File);
	#endif
#endif
            }//if not found in table
            else{
            	printf("duplicate packet found\n\n");
                uint32_t dupPacket = shaIndex;
                dupPacket <<= 1;
                dupPacket |= 0x1;//bit 0 becomes a 1 to indicate a duplicate
#if READING_FROM_SERVER
                fwrite(&dupPacket, sizeof(uint32_t), 1, File);
#else
	#ifdef __SDSCC__
                f_write(&File, &dupPacket, 4, &bytes_written);
	#else
                fwrite(&dupPacket, sizeof(uint32_t), 1, File);
	#endif
#endif

            }//if found in table

            chunks++;
//Code below is for processing+reading from NW implementation
#if READING_FROM_SERVER
        /*    if(!server_rd_stp) {
              uint8_t pkt[MAXPKTSIZE+HEADER];
              uint16_t thisPacketBytes = Server.get_packet(pkt);
              if (thisPacketBytes < 0){
                Exit_with_error();
              }//end of transmission
              if((pkt[1] & 0x80) == 128) 
                server_rd_stp = 1;
              thisPacketBytes = (((uint16_t)(pkt[1] << 8 | pkt[0])) & 0x7fff) ;
              printf("pkt[0] : %x\n", pkt[0]);
              printf("pkt[1] : %x\n", pkt[1]);
              printf("pkt len: %d\n", thisPacketBytes);
              memcpy(buf + currentIndex, &pkt[HEADER], thisPacketBytes);
              currentIndex += thisPacketBytes;
              bytes += thisPacketBytes;
              len += thisPacketBytes;


            }
            */
#endif
        }//while(true)

#if MEASURING_LATENCY
       printf("Rabin avg duration: %d", rabin_dur/chunks);
       printf("SHA avg duration: %d", sha_dur/chunks);
       printf("DEDUP avg duration: %d", dedup_dur/chunks);
       printf("lzw avg duration: %d", lzw_dur/chunks);
#endif

    if (len > 0) {
        chunks++;

#ifdef USING_SHA_HW
        sha256_hw_compute(&chunk[0], len, sha_buf_hw);
#else
        sha256_init(&ctx);
        sha256_update(&ctx, &chunk[0], len);
        sha256_final(&ctx, sha_buf);
        printf("Chunk_length fhash: %d\n", len);

        //compare(sha_buf, sha_buf_hw);
#endif


#ifdef USING_CHUNKDICT_HW
        int shaIndex = indexForShaVal_HW(sha_buf_hw, chunkDictTable);
#else
        int shaIndex = indexForShaVal(sha_buf_hw);
#endif

        if(shaIndex == -1){

#ifdef USING_LZW_HW

            int compress_size = lzwCompressWrapper(&chunk[0], len, &compress[4]);
            uint32_t header = (compress_size - 4) << 1;
            memcpy((void*)&compress[0], &header, 1 * sizeof(uint32_t));
#else
            int compress_size = lzwCompress(&chunk[0], len, compress);
#endif


#if READING_FROM_SERVER
            fwrite(compress, sizeof(uint8_t), compress_size, File);
            printf("Compress fhash size: %d\n", compress_size);
#else

  #ifdef __SDSCC__
            f_write(&File, compress, compress_size, &bytes_written);
  #else
            fwrite(compress, sizeof(uint8_t), compress_size, File);
            printf("Compress fhash size: %d\n", compress_size);
  #endif
#endif
        }//if not found in table
        else{

            uint32_t dupPacket = shaIndex;
            dupPacket <<= 1;
            dupPacket |= 0x1;//bit 0 becomes a 1 to indicate a duplicate
#if READING_FROM_SERVER
            fwrite(&dupPacket, sizeof(uint32_t), 1, File);
#else            
  #ifdef __SDSCC__
            f_write(&File, &dupPacket, 4, &bytes_written);
  #else
            fwrite(&dupPacket, sizeof(uint32_t), 1, File);
  #endif
#endif

        }//if found in table
    }

/*#ifdef __SDSCC__
    f_close(&File);
#else*/
    fclose(File);
//#endif
#ifdef USING_CHUNKDICT_HW
    Free(chunkDictTable);
#endif
    Free(buf);
    Free(compress);

    unsigned int avg = 0;
    if (chunks > 0)
        avg = bytes / chunks;
    printf("%d chunks, average chunk size %d\n", chunks, avg);

#endif // SHA_TEST

    return 0;
}

