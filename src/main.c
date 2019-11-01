#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "rabin.h"
#include "sha_256.h"
#include "lzw_sw.h"

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
    uint16_t *compress;
    unordered_map<string, int> umap;

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
                printf("%x", sha_buf[i]);

            printf("\n");


            int compress_size = lzwCompress(&buf[last_chunk.start], last_chunk.length, compress);
            
            printf("compress_size: %d\n", compress_size);
            fwrite(compress, compress_size, 1, fp2);
/*
           if shaResult in chunkDictionary:
             send(shaResult)
           else:
             send(LZW(rawChunk))
          **/

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
            int compress_size = lzwCompress(&buf[last_chunk.start], last_chunk.length, compress);
            
            printf("compress_size: %d\n", compress_size);
            fwrite(compress, compress_size, 1, fp2);
          /** sha256_hash(buf[last_chunk.start + i], last_chunk.length); 
           if shaResult in chunkDictionary:
             send(shaResult)
           else:
             send(LZW(rawChunk))

             **/
    }

    unsigned int avg = 0;
    if (chunks > 0)
        avg = bytes / chunks;
    printf("%d chunks, average chunk size %d\n", chunks, avg);

    return 0;
}
