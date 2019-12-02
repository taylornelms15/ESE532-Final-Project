#ifndef _RABIN_H
#define _RABIN_H

//#include <stdint.h>
#include "common.h"
#include <hls_stream.h>

#define POLYNOMIAL 0x3DA3358B4DC173LL
#define POLYNOMIAL_DEGREE 53
#define WINSIZE 64
#define AVERAGE_BITS 10


struct rabin_t {
    uint8_t window[WINSIZE];
    unsigned int wpos;
    unsigned int count;
    unsigned int pos;
    unsigned int start;
    uint64_t digest;
};
typedef struct rabin_t rabin_t;

struct chunk_t {
    unsigned int start;
    unsigned int length;
    uint64_t cut_fingerprint;
    uint8_t byte[MAXSIZE];
};

extern struct chunk_t last_chunk;

struct rabin_t *rabin_init(void);
void rabin_reset(struct rabin_t *h);
uint8_t rabin_slide(struct rabin_t *h, uint8_t b, uint8_t wpos);
uint64_t rabin_append(uint64_t digest, uint8_t c);

void rabin_next_chunk_HW(hls::stream<ap_uint<9> > &readerToRabin, hls::stream<ap_uint<9> > &rabinToSHA, hls::stream<ap_uint<9> > &rabinToLZW, unsigned long long out_table[256], unsigned long long mod_table[256], uint32_t len);

int rabin_next_chunk_SW(struct rabin_t *h, uint8_t buf[MAXSIZE], uint8_t chunk[MAXSIZE], unsigned long long out_table[256], unsigned long long mod_table[256], unsigned int len);

/*
int rabin_next_chunk_HW(uint8_t buf[MAXINPUTFILESIZE], uint8_t chunk[MAXSIZE], unsigned long long out_table[256], unsigned long long mod_table[256], unsigned int len);
*/

struct chunk_t *rabin_finalize(struct rabin_t *h);

#endif
