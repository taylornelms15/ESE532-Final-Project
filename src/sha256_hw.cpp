

#include <stdlib.h>
#include<string.h>
#include<stdio.h>
#include <stdint.h>
//#include "sha_256.h"
#include "sha256_hw.h"
#include "common.h"

#define MAXCHUNKLENGTH (MAXSIZE)

#define SHA_STREAM

/** Reference : https://github.com/B-Con/crypto-algorithms */

/****************************** MACROS ******************************/
#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

void sha256_hw_final(unsigned int datalen, WORD_SHA state[8], BYTE data[64], unsigned long long bitlen, BYTE hash[]);


/*********************** FUNCTION DEFINITIONS *********************** */

#ifdef SHA_STREAM
void sha256_hw_transform(hls::stream<ap_uint<9>>& data, WORD_SHA state[])
{
	static const WORD_SHA k[64] = {
				0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
				0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
				0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
				0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
				0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
				0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
				0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
				0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
			};
#pragma HLS array_partition variable=k

	WORD_SHA a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
//#pragma HLS array_partition variable=m
#pragma HLS RESOURCE variable=t1 core=AddSub_DSP
#pragma HLS RESOURCE variable=t2 core=AddSub_DSP
#pragma HLS RESOURCE variable=m core=AddSub_DSP
#pragma HLS RESOURCE variable=state core=AddSub_DSP

	loop_1:for (i = 0, j = 0; i < 16; ++i, j += 4)
	{
//#pragma HLS unroll
		m[i] = (data.read() << 24) | (data.read() << 16) | (data.read() << 8) | (data.read());
	}

	loop_2:for ( ; i < 64; ++i)
	{
//#pragma HLS unroll factor=2
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
	}

	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];
	e = state[4];
	f = state[5];
	g = state[6];
	h = state[7];

	update_loop:for (i = 0; i < 64; ++i)
	{
//#pragma HLS pipeline
		t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
		t2 = EP0(a) + MAJ(a,b,c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
	state[5] += f;
	state[6] += g;
	state[7] += h;
}
#else
void sha256_hw_transform(BYTE data[], WORD_SHA state[])
{
	static const WORD_SHA k[64] = {
				0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
				0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
				0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
				0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
				0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
				0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
				0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
				0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
			};
#pragma HLS array_partition variable=k

	WORD_SHA a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
//#pragma HLS array_partition variable=m
#pragma HLS RESOURCE variable=t1 core=AddSub_DSP
#pragma HLS RESOURCE variable=t2 core=AddSub_DSP
#pragma HLS RESOURCE variable=m core=AddSub_DSP
#pragma HLS RESOURCE variable=state core=AddSub_DSP

	loop_1:for (i = 0, j = 0; i < 16; ++i, j += 4)
	{
//#pragma HLS unroll
		m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
	}

	loop_2:for ( ; i < 64; ++i)
	{
//#pragma HLS unroll factor=2
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
	}

	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];
	e = state[4];
	f = state[5];
	g = state[6];
	h = state[7];

	update_loop:for (i = 0; i < 64; ++i)
	{
//#pragma HLS pipeline
		t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
		t2 = EP0(a) + MAJ(a,b,c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
	state[5] += f;
	state[6] += g;
	state[7] += h;
}
#endif

#ifdef SHA_STREAM
int sha256_hw_compute(hls::stream<ap_uint<9>>& data, hls::stream< uint8_t >& hash)
{
//#pragma HLS allocation instances=sha256_hw_transform limit=1 function
	WORD_SHA state[8];
#pragma HLS array_partition variable=state

	state[0] = 0x6a09e667;
	state[1] = 0xbb67ae85;
	state[2] = 0x3c6ef372;
	state[3] = 0xa54ff53a;
	state[4] = 0x510e527f;
	state[5] = 0x9b05688c;
	state[6] = 0x1f83d9ab;
	state[7] = 0x5be0cd19;

	unsigned int datalen = 0;
	unsigned long long bitlen = 0;
	unsigned int i;
	int ending_byte = ENDOFCHUNK;

#pragma HLS RESOURCE variable=bitlen core=AddSub_DSP
#pragma HLS RESOURCE variable=datalen core=AddSub_DSP
#pragma HLS RESOURCE variable=i core=AddSub_DSP

	for(uint16_t i = 1; i <= MAXCHUNKLENGTH; i++)
	{
#pragma HLS loop_tripcount min=0 avg=4000 max=8000
		hls::stream<ap_uint<9>> subchunk, last_subchunk;
		ap_uint<9> byte = data.read();

		datalen++;
		subchunk.write(byte);
#if 1
		if (byte > 255 && datalen < 64)	/** Either end of chunk or end of file */
		{
			if (datalen < 56) {
					subchunk.write(0x80);
					while (i < 56)
						subchunk.write(0x00);
				}
				else
				{
					//printf("len greater then 56\n");
					subchunk.write(0x80);
					while_loop:while (i < 64)
					{
			#pragma HLS loop_tripcount min=0 avg=4 max=8
						subchunk.write(0x00);
					}

					sha256_hw_transform(subchunk, state);
					for (i = 0; i < 56; i++)
						subchunk.write(0);
				}

				/*
				printf("internediate state variables:\n");
						for(int i = 0; i < 8; i++)
							printf("%u ", state[i]);
						printf("\n");

			*/
				// Append to the padding the total message's length in bits and transform.
				bitlen += datalen * 8;
				subchunk.write(bitlen >> 56);
				subchunk.write(bitlen >> 48);
				subchunk.write(bitlen >> 40);
				subchunk.write(bitlen >> 32);
				subchunk.write(bitlen >> 24);
				subchunk.write(bitlen >> 16);
				subchunk.write(bitlen >> 8);
				subchunk.write(bitlen);

				sha256_hw_transform(subchunk, state);

				// TODO: Stream SHA values ?
				#pragma HLS RESOURCE variable=hash core=AddSub_DSP
				// Since this implementation uses little endian byte ordering and SHA uses big endian,
				// reverse all the bytes when copying the final state to the output hash.

				printf("hash values: \n");
				for(int j = 0; j < 8; j++)
				{
					for (i = 0; i < 4; ++i)
					{
						hash.write((state[j] >> (24 - i * 8)) & 0x000000ff);
						printf("%x ", (state[j] >> (24 - i * 8)) & 0x000000ff);
						hash.write((state[j] >> (24 - i * 8)) & 0x000000ff);
						printf("%x ", (state[j] >> (24 - i * 8)) & 0x000000ff);
						hash.write((state[j] >> (24 - i * 8)) & 0x000000ff);
						printf("%x ", (state[j] >> (24 - i * 8)) & 0x000000ff);
						hash.write((state[j] >> (24 - i * 8)) & 0x000000ff);
						printf("%x ", (state[j] >> (24 - i * 8)) & 0x000000ff);
					}
				}

			ending_byte = byte;
			return ending_byte;
		}

		if (datalen == 64)
		{
			sha256_hw_transform(subchunk, state);
			bitlen += 512;
			datalen = 0;
		}
#endif
	}

/*
	printf("state variables:\n");
	for(int i = 0; i < 8; i++)
		printf("%u ", state[i]);
	printf("\n");
*/

}
#else
#pragma SDS data access_pattern(data:SEQUENTIAL)
#pragma SDS data copy(data[0:len])
/** Top-level function */
void sha256_hw_compute(const BYTE data[MAXSIZE], int len, BYTE hash[SHA256_BLOCK_SIZE])
{
//#pragma HLS allocation instances=sha256_hw_transform limit=1 function
	WORD_SHA state[8];
#pragma HLS array_partition variable=state

	state[0] = 0x6a09e667;
	state[1] = 0xbb67ae85;
	state[2] = 0x3c6ef372;
	state[3] = 0xa54ff53a;
	state[4] = 0x510e527f;
	state[5] = 0x9b05688c;
	state[6] = 0x1f83d9ab;
	state[7] = 0x5be0cd19;

	BYTE subchunk[64];
#pragma HLS array_partition variable=subchunk

	unsigned int datalen = 0;
	unsigned long long bitlen = 0;
	unsigned int i;

#pragma HLS RESOURCE variable=bitlen core=AddSub_DSP
#pragma HLS RESOURCE variable=datalen core=AddSub_DSP
#pragma HLS RESOURCE variable=i core=AddSub_DSP

	main_loop:for (i = 0; i < len; ++i)
	{
#pragma HLS loop_tripcount min=0 avg=4000 max=8000
#pragma HLS pipeline
		//subchunk.write(data[i]);
		subchunk[datalen] = data[i];
		datalen++;
		if (datalen == 64)
		{
			sha256_hw_transform(subchunk, state);
			bitlen += 512;
			datalen = 0;
		}
	}
/*
	printf("state variables:\n");
	for(int i = 0; i < 8; i++)
		printf("%u ", state[i]);
	printf("\n");
*/

	sha256_hw_final(datalen, state, subchunk, bitlen, hash);
}


#if 1
void sha256_hw_final(unsigned int datalen, WORD_SHA state[8], hls::stream<ap_uint<9>> data, unsigned long long bitlen, BYTE hash[])
{
#pragma HLS INLINE
	WORD_SHA i;

	i = datalen;

	// Pad whatever data is left in the buffer.
	if (datalen < 56) {
		data[i++] = 0x80;
		while (i < 56)
			data[i++] = 0x00;
	}
	else
	{
		//printf("len greater then 56\n");
		data[i++] = 0x80;
		while_loop:while (i < 64)
		{
#pragma HLS loop_tripcount min=0 avg=4 max=8
			data[i++] = 0x00;
		}

		sha256_hw_transform(data, state);
		// TODO: need to memset
		for (i = 0; i < 56; i++)
			data[i] = 0;
	}

	/*
	printf("internediate state variables:\n");
			for(int i = 0; i < 8; i++)
				printf("%u ", state[i]);
			printf("\n");

*/
	// Append to the padding the total message's length in bits and transform.
	bitlen += datalen * 8;
	data[63] = bitlen;
	data[62] = bitlen >> 8;
	data[61] = bitlen >> 16;
	data[60] = bitlen >> 24;
	data[59] = bitlen >> 32;
	data[58] = bitlen >> 40;
	data[57] = bitlen >> 48;
	data[56] = bitlen >> 56;
	sha256_hw_transform(data, state);

	/*
	printf("final state variables:\n");
		for(int i = 0; i < 8; i++)
			printf("%u ", state[i]);
		printf("\n");

*/

#pragma HLS RESOURCE variable=hash core=AddSub_DSP
	// Since this implementation uses little endian byte ordering and SHA uses big endian,
	// reverse all the bytes when copying the final state to the output hash.
	for (i = 0; i < 4; ++i) {
		hash[i]      = (state[0] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 4]  = (state[1] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 8]  = (state[2] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 12] = (state[3] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 16] = (state[4] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 20] = (state[5] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 24] = (state[6] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 28] = (state[7] >> (24 - i * 8)) & 0x000000ff;
	}
/*
	printf("hash values: \n");
	for(int i = 0; i < SHA256_BLOCK_SIZE; i++)
		printf("%x ", hash[i]);
	printf("\n");
*/
}
#endif
#endif

void sha256_hw_wrapper(hls::stream<ap_uint<9>>& rabinToSHA, hls::stream< uint8_t >& shaToDeduplicate)
{
	/** Collect chunk and send it to SHA_HW unit to  process it */
	/** If MSB = 1, it indicates end of chunk */
	for (int i = 0; i < MAX_CHUNKS_IN_HW_BUFFER; i++){
	        //#pragma HLS pipeline
	        int endingByte = sha256_hw_compute(rabinToSHA, shaToDeduplicate);
	        shaToDeduplicate.write((ap_uint<9>) endingByte);
	        if (endingByte == ENDOFFILE){
	            return;
	        }
	    }
}


