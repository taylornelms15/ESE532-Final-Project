#ifndef _RABIN_H
#define _RABIN_H

#include <stdint.h>
#include "common.h"

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
    signed int length;
    uint64_t cut_fingerprint;
    uint8_t byte[MAXSIZE];
};

extern struct chunk_t last_chunk;

struct rabin_t *rabin_init(void);
void rabin_reset(struct rabin_t *h);
uint8_t rabin_slide(struct rabin_t *h, uint8_t b, uint8_t wpos);
uint64_t rabin_append(uint64_t digest, uint8_t c);
int rabin_next_chunk(struct rabin_t *h,uint8_t buf[MAXINPUTFILESIZE], uint8_t chunk[MAXSIZE], uint64_t out_table[256], uint64_t mod_table[256], unsigned int len);
struct chunk_t *rabin_finalize(struct rabin_t *h);

#endif
