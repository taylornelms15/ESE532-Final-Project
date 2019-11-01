#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "rabin.h"
#include "sha_256.h"
#include "lzw_sw.h"
#include "chunkdict.h"

// 1MiB buffer
uint8_t buf[1024 * 1024];
size_t bytes;

int main(int argc, char *argv[]) {
    struct rabin_t *hash;
    FILE *fp = NULL;
    FILE *fp2 = NULL;
    hash = rabin_init();
    lzw_init();
    SHA256_CTX ctx;
    sha256_init(&ctx);
    unsigned int chunks = 0;
    BYTE sha_buf[SHA256_BLOCK_SIZE];
    uint8_t compress[8192];//TODO: replace this with some kind of max chunk size?

    if(argc > 1) {
    fp = fopen(argv[1], "r");
      if(fp == NULL) {
        printf("File could not be opened. Exiting program!\n");
        exit(-1);
      }
    fp2 = fopen("Output.txt", "a+");
      if(fp2 == NULL) {
        printf("File could not be opened. Exiting program!\n");
        exit(-1);
      }
    } else {
      printf("USAGE: ./rabin-cdc <file to be chunked>\n");
      exit(0);
    }
    while (!feof(fp)) {
        size_t len = fread(buf, 1, sizeof(buf), fp);
        uint8_t *ptr = buf;

        bytes += len;

        while (1) {
            int remaining = rabin_next_chunk(hash, ptr, len);

            if (remaining < 0) {
                break;
            }

            len -= remaining;
            ptr += remaining;

            printf("%d %d %016llx\n \n",
                last_chunk.start,
                last_chunk.length,
                (long long unsigned int)last_chunk.cut_fingerprint);
    /*        if(chunks == 1) {
                printf("*********chunks**********");
                for(int i = 0; i < last_chunk.length; i++) {
                    printf("%c", buf[last_chunk.start + i]);
                }
            }
            */
            sha256_init(&ctx);
            sha256_update(&ctx, &buf[last_chunk.start], last_chunk.length); 
            sha256_final(&ctx, sha_buf);

            printf("SHA: ");
            for(int i = 0; i < SHA256_BLOCK_SIZE; i++)
                printf("%02x", sha_buf[i]);

            printf("\n");

            int shaIndex = indexForShaVal(sha_buf);
            if(sha_index == -1){
                int compress_size = lzwCompress(&buf[last_chunk.start], last_chunk.length, compress);
                
                printf("compress_size: %d\n", compress_size);
                fwrite(compress, sizeof(uint8_t), compress_size, fp2);
            }//if not found in table
            else{
                uint32_t dupPacket = shaIndex;
                dupPacket << 1;
                dupPacket |= 0x1;//bit 0 becomes a 1 to indicate a duplicate
                fwrite(&dupPacket, sizeof(uint32_t), 1, fp2);

            }//if found in table

            chunks++;
        }
    }

    if (rabin_finalize(hash) != NULL) {
        chunks++;
        printf("%d %016llx\n",
            last_chunk.length,
            (long long unsigned int)last_chunk.cut_fingerprint);

            sha256_init(&ctx);
            sha256_update(&ctx, &buf[last_chunk.start], last_chunk.length); 
            sha256_final(&ctx, sha_buf);
            printf("SHA: ");
            for(int i = 0; i < SHA256_BLOCK_SIZE; i++)
                printf("%x", sha_buf[i]);

            printf("\n");
            int shaIndex = indexForShaVal(sha_buf);
            if(sha_index == -1){
                int compress_size = lzwCompress(&buf[last_chunk.start], last_chunk.length, compress);
                
                printf("compress_size: %d\n", compress_size);
                fwrite(compress, sizeof(uint8_t), compress_size, fp2);
            }//if not found in table
            else{
                uint32_t dupPacket = shaIndex;
                dupPacket << 1;
                dupPacket |= 0x1;//bit 0 becomes a 1 to indicate a duplicate
                fwrite(&dupPacket, sizeof(uint32_t), 1, fp2);

            }//if found in table
    }

    unsigned int avg = 0;
    if (chunks > 0)
        avg = bytes / chunks;
    printf("%d chunks, average chunk size %d\n", chunks, avg);

    return 0;
}
