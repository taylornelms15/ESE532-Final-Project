#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "rabin.h"
#include "sha_256.h"
#include "lzw_sw.h"
#include "chunkdict.h"
#ifdef __SDSCC__
#include <ff.h>
#include <sds_lib.h>
#endif


#include "common.h"

// 1MiB buffer
uint8_t* buf;
size_t bytes;

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
  Check_error(Result != FR_OK || Bytes_read != Size, "Could not read input file.");

  Check_error(f_close(&File) != FR_OK, "Could not close input file.");
#else
  FILE * File = fopen("/Users/taylo/csworkspace/ese532/final/Testfiles/gtk+.tar", "rb");
  if (File == NULL)
    Exit_with_error();

  Bytes_read = fread(Data, 1, Size, File);
  if (Bytes_read < 1)
    Exit_with_error();

  if (fclose(File) != 0)
    Exit_with_error();
#endif
  return Bytes_read;
}

int main(int argc, char *argv[]) {

	struct rabin_t *hash;
    hash = rabin_init();
    SHA256_CTX ctx;
    sha256_init(&ctx);
    unsigned int chunks = 0;
    BYTE sha_buf[SHA256_BLOCK_SIZE];
    uint8_t compress[MAXSIZE];//TODO: replace this with some kind of max chunk size?
    unsigned int bytes_read;

#ifdef __SDSCC__
  FATFS FS;

  Check_error(f_mount(&FS, "0:/", 0) != FR_OK, "Could not mount SD-card");
#endif

  buf = (uint8_t*)malloc(MAXINPUTFILESIZE * sizeof(uint8_t));
  unsigned int len = Load_data(buf);

#ifdef __SDSCC__
      FIL File;

        FRESULT Result = f_open(&File, "Output.bin", FA_WRITE | FA_CREATE_ALWAYS);
        Check_error(Result != FR_OK, "Could not open output file.");

#else
        FILE* File = fopen("/Users/taylo/csworkspace/ese532/final/Testfiles/gtk+.tar.out", "wb");
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
        }
    //}

    if (rabin_finalize(hash) != NULL) {
        chunks++;
        //printf("%d %016llx\n",
        //    last_chunk.length,
        //    (long long unsigned int)last_chunk.cut_fingerprint);

            sha256_init(&ctx);
            sha256_update(&ctx, &buf[last_chunk.start], last_chunk.length); 
            sha256_final(&ctx, sha_buf);
            //printf("SHA: ");
            //for(int i = 0; i < SHA256_BLOCK_SIZE; i++)
            //    printf("%x", sha_buf[i]);

            printf("\n");
            int shaIndex = indexForShaVal(sha_buf);
            if(shaIndex == -1){
                int compress_size = lzwCompress(&buf[last_chunk.start], last_chunk.length, compress); 
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

    return 0;
}
