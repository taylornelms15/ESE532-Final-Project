/* Reference : https://github.com/fd0/rabin-cdc */

//#include <err.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "rabin.h"

#define MASK ((1<<AVERAGE_BITS)-1)
#define POL_SHIFT (POLYNOMIAL_DEGREE-8)

struct chunk_t last_chunk;

static bool tables_initialized = false;
/*unsigned long long mod_table[256];
unsigned long long out_table[256];
*/
extern unsigned long long* out_table;
//std::cout << "Allocated out_table at " << out_table << std::endl;
extern unsigned long long* mod_table;
//std::cout <<"Allocated mod_table at " << mod_table << std::endl;


static int deg(uint64_t p) {
    uint64_t mask = 0x8000000000000000LL;

    for (int i = 0; i < 64; i++) {
        if ((mask & p) > 0) {
            return 63 - i;
        }

        mask >>= 1;
    }

    return -1;
}

// Mod calculates the remainder of x divided by p.
static uint64_t mod(uint64_t x, uint64_t p) {
    while (deg(x) >= deg(p)) {
        unsigned int shift = deg(x) - deg(p);

        x = x ^ (p << shift);
    }

    return x;
}

static uint64_t append_byte(uint64_t hash, uint8_t b, uint64_t pol) {
    hash <<= 8;
    hash |= (uint64_t)b;

    return mod(hash, pol);
}

static void calc_tables(void) {
    // calculate table for sliding out bytes. The byte to slide out is used as
    // the index for the table, the value contains the following:
    // out_table[b] = Hash(b || 0 ||        ...        || 0)
    //                          \ windowsize-1 zero bytes /
    // To slide out byte b_0 for window size w with known hash
    // H := H(b_0 || ... || b_w), it is sufficient to add out_table[b_0]:
    //    H(b_0 || ... || b_w) + H(b_0 || 0 || ... || 0)
    //  = H(b_0 + b_0 || b_1 + 0 || ... || b_w + 0)
    //  = H(    0     || b_1 || ...     || b_w)
    //
    // Afterwards a new byte can be shifted in.
	int b;
    for (b = 0; b < 256; b++) {
        uint64_t hash = 0;

        hash = append_byte(hash, (uint8_t)b, POLYNOMIAL);
        for (int i = 0; i < WINSIZE-1; i++) {
            hash = append_byte(hash, 0, POLYNOMIAL);
        }
        out_table[b] = hash;
    }

    // calculate table for reduction mod Polynomial
    int k = deg(POLYNOMIAL);
    for (b = 0; b < 256; b++) {
        // mod_table[b] = A | B, where A = (b(x) * x^k mod pol) and  B = b(x) * x^k
        //
        // The 8 bits above deg(Polynomial) determine what happens next and so
        // these bits are used as a lookup to this table. The value is split in
        // two parts: Part A contains the result of the modulus operation, part
        // B is used to cancel out the 8 top bits so that one XOR operation is
        // enough to reduce modulo Polynomial
        mod_table[b] = mod(((uint64_t)b) << k, POLYNOMIAL) | ((uint64_t)b) << k;
    }
}

uint64_t rabin_append(uint64_t digest, uint8_t c) {
	uint8_t d = c;
    uint8_t index = (uint8_t)(digest >> POL_SHIFT);
    digest <<= 8;
    digest |= (uint64_t)d;
    digest ^= mod_table[index];
    return digest;
}


uint8_t rabin_slide(struct rabin_t *h, uint8_t b, uint8_t wpos) {
    uint64_t digest = h->digest;
    uint8_t c = b;
	uint8_t out = h->window[wpos];
    h->window[wpos] = c;
    digest = (digest ^ out_table[out]);
    wpos = (wpos +1 ) % WINSIZE;
    h->digest = rabin_append(digest, c);
    return wpos;
}


void rabin_reset(struct rabin_t *h) {
    for (int i = 0; i < WINSIZE; i++)
        h->window[i] = 0;
    h->digest = 0;
    h->wpos = 0;
    h->count = 0;
    h->digest = 0;
    h->start = 0;
    h->pos = 0;

    rabin_slide(h, 1, 0);
}



void rabin_next_chunk_HW(hls::stream<ap_uint<9> > &readerToRabin, hls::stream<ap_uint<9> > &rabinToSHA, hls::stream<ap_uint<9> > &rabinToLZW, unsigned long long out_table[256], unsigned long long mod_table[256], uint32_t len) {

	int counter = 0;
    uint8_t wpos = 0;
    uint64_t digest = 0;
    signed int count = 0;
    int last_chunk_length = -1;
    uint8_t window[WINSIZE];
    uint8_t copy_to_window = 0;
 #pragma HLS ARRAY_PARTITION variable=window dim=1 complete

    for (int j = 0; j < WINSIZE; j++) {
	#pragma HLS UNROLL
      window[j] = 0;
    }

    uint8_t out = window[0];
    window[0] = 1;
    wpos = (wpos +1 ) % WINSIZE;

    digest = (digest ^ out_table[out]);
    uint8_t index = (uint8_t)(digest >> POL_SHIFT);

    digest = (digest << 8 | (uint64_t)1) ^ mod_table[index];


	chunk_loop:for (unsigned int i = 0; i < INBUFFER_SIZE + MAX_CHUNKS_IN_HW_BUFFER + 1; i++) {
		#pragma HLS loop_tripcount min=1024 max=8192
		#pragma HLS pipeline II=1
         ap_uint<9> val = readerToRabin.read();
         uint8_t b = val & 0xff;
        out = window[wpos];
        if(val == ENDOFFILE)
           copy_to_window = 0;
        else
           copy_to_window = 1;

        if(copy_to_window == 1) {
        	window[wpos] = b;
        	wpos = (wpos +1 ) % WINSIZE;

        	digest = (digest ^ out_table[out]);
        	index = (uint8_t)(digest >> POL_SHIFT);

        	digest = (digest << 8 | (uint64_t)b) ^ mod_table[index];
        	count++;
        }
        rabinToSHA.write(val);
        counter++;
        rabinToLZW.write(val);
        if(val == ENDOFFILE) {
        	//printf("wrote %d bytes to SHA - EOF\n", counter);
           break;
        }
        //chunk[i] = b;

        if (count >= MINSIZE) {
        	if((digest & MASK) == 0 || count >= MAXSIZE) {
        		rabinToSHA.write(ENDOFCHUNK);
        		counter++;
        		//printf("wrote %d bytes to SHA - EOC\n", counter);
        		counter = 0;
        		rabinToLZW.write(ENDOFCHUNK);

       //     last_chunk_length = count;
        		count = 0;

        	}
        }

    }

}


int rabin_next_chunk_SW(struct rabin_t *h, uint8_t buf[MAXSIZE], uint8_t chunk[MAXSIZE], unsigned long long out_table[256], unsigned long long mod_table[256], unsigned int len) {
	unsigned int count = h->count;
	unsigned int pos = h->pos;
	uint64_t digest = h->digest;
    uint8_t wpos = 0;
   // uint8_t is_stop = 0;



	for (unsigned int i = 0; i < len; i++) {

        uint8_t b = buf[i];

     //   if (is_stop == 0) {
        uint8_t out = h->window[wpos];
        h->window[wpos] = b;
        wpos = (wpos +1 ) % WINSIZE;

        digest = (digest ^ out_table[out]);
        uint8_t index = (uint8_t)(digest >> POL_SHIFT);

        digest = (digest << 8 | (uint64_t)b) ^ mod_table[index];

        count++;
        pos++;
        chunk[i] = b;
       // }

      //  if (((count >= MINSIZE) && (digest & MASK) == 0) || count >= MAXSIZE) {
        if (count >= MINSIZE) {
        	if((digest & MASK) == 0) {

            last_chunk.length = count;
            last_chunk.cut_fingerprint =  digest;
            last_chunk.byte[count] = '\0';
            // keep position
            //unsigned int pos = h.pos;
            for (int i = 0; i < WINSIZE; i++)
                    h->window[i] = 0;

                h->digest = 0;
                h->count = 0;
                digest = 0;
                count = 0;

            h->pos = pos;
           // is_stop = 1;
            return last_chunk.length;
            //return i+1;
        }
        if(count >= MAXSIZE) {
        	            last_chunk.length = count;
        	            last_chunk.cut_fingerprint =  digest;
        	            last_chunk.byte[count] = '\0';
        	            // keep position
        	            //unsigned int pos = h.pos;
        	#pragma HLS ARRAY_PARTITION variable=h->window dim=0 complete
        	            for (int i = 0; i < WINSIZE; i++)
        	                    h->window[i] = 0;
        	                h->digest = 0;
        	                h->count = 0;
        	                digest = 0;
        	                count = 0;

        	            h->pos = pos;
        	           // is_stop = 1;
        	            return last_chunk.length;
        	            //return i+1;

        	}
        }
    }
	h->count = count;
	h->pos = pos;

	return -1;
	//last_chunk.length;

}




struct rabin_t *rabin_init(void) {
    if (!tables_initialized) {
        calc_tables();
        tables_initialized = true;
    }

    struct rabin_t *h;

    if ((h = (rabin_t*)malloc(sizeof(struct rabin_t))) == NULL) {
    	printf("Error on rabin_init malloc()\n");
    	exit(1);
        //errx(1, "malloc()");
    }

    rabin_reset(h);

    return h;
}


struct chunk_t *rabin_finalize(struct rabin_t *h) {
    if (h->count == 0) {
        last_chunk.length = 0;
        return NULL;
    }

    last_chunk.length = h->count;

    return &last_chunk;
}
