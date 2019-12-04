

//#include <stdlib.h>
//#include<string.h>
//#include<stdio.h>
//#include <stdint.h>
#include "sha256_hw.h"

#define MAXCHUNKLENGTH (MAXSIZE)


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

//void sha256_hw_final(unsigned int datalen, WORD_SHA state[8], BYTE data[64], unsigned long long bitlen, BYTE hash[]);


/*********************** FUNCTION DEFINITIONS *********************** */

static int counter_hw = 0;

void sha256_hw_transform(hls::stream<ap_uint<9> > &data, unsigned int state[8])
{
	static const unsigned int k[64] = {
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

	unsigned int a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
//#pragma HLS array_partition variable=m
#pragma HLS RESOURCE variable=t1 core=AddSub_DSP
#pragma HLS RESOURCE variable=t2 core=AddSub_DSP
#pragma HLS RESOURCE variable=m core=AddSub_DSP
#pragma HLS RESOURCE variable=state core=AddSub_DSP


	loop_1:for (i = 0, j = 0; i < 16; ++i, j += 4)
	{
//#pragma HLS unroll
		m[i] = (data.read() << 24) | (data.read() << 16) | (data.read() << 8) | (data.read());
		counter_hw += 4;
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

	//printf("aggduasgduiak read %d bytes from subchunk\n", counter_hw);
}

int sha256_hw_compute(hls::stream<ap_uint<9> >& data, hls::stream< uint8_t >& hash)
{
//#pragma HLS allocation instances=sha256_hw_transform limit=1 function
	unsigned int state[8];
	int counter = 0, subchunk_counter = 0;
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

	ap_uint<9> byte;
	hls::stream<ap_uint<9> > subchunk;
	uint16_t itr;
#pragma HLS STREAM variable=subchunk depth=2048
	for(uint16_t i = 0; i < MAXCHUNKLENGTH + 5; i++)
	{
//#pragma HLS dataflow
#pragma HLS loop_tripcount min=0 avg=4000 max=8000

		byte = data.read();
		counter++;
		datalen++;


		if (byte > 255)	/** Either end of chunk or end of file */
		{
#if 1
			datalen--;

			itr = datalen;
			if (itr++ < 56) {
					subchunk.write(0x80);
					subchunk_counter++;
					while (itr < 56){
						itr++;
						subchunk.write(0x00);
						subchunk_counter++;
					}
				}
				else
				{
					//printf("len greater then 56\n");
					subchunk.write(0x80);
					subchunk_counter++;
					while_loop:while (itr++ < 64)
					{
			#pragma HLS loop_tripcount min=0 avg=4 max=8
						subchunk.write(0x00);
						subchunk_counter++;
					}

					//printf("dhasdujhandlka calling from 1\n");
					sha256_hw_transform(subchunk, state);
					for (itr = 0; itr < 56; itr++){
						subchunk.write(0);
						subchunk_counter++;
					}
				}

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
				subchunk_counter += 8;

				//printf("dhasdujhandlka calling from 2\n");
				sha256_hw_transform(subchunk, state);

				//#pragma HLS RESOURCE variable=hash core=AddSub_DSP
				// Since this implementation uses little endian byte ordering and SHA uses big endian,
				// reverse all the bytes when copying the final state to the output hash.
#if 1

				//printf("hash values: \n");
				for(int j = 0; j < 8; j++)
				{
						hash.write((state[j] >> (24 - 0 * 8)) & 0x000000ff);

						hash.write((state[j] >> (24 - 1 * 8)) & 0x000000ff);

						hash.write((state[j] >> (24 - 2 * 8)) & 0x000000ff);

						hash.write((state[j] >> (24 - 3 * 8)) & 0x000000ff);
				}
#endif

#endif
			ending_byte = byte;

			//printf("read %d bytes from input \n", counter);
			//printf("written %d bytes to subchunk\n", subchunk_counter);
			//printf("read %d bytes from subchunk \n", counter_hw);
			counter_hw = 0;
			return ending_byte;
		}


		subchunk.write(byte);
		subchunk_counter++;

		if (datalen == 64)
		{
			//printf("dhasdujhandlka calling from 3\n");
			sha256_hw_transform(subchunk, state);
			bitlen += 512;
			datalen = 0;
		}

	}

	//printf("OUTSIDE LOOP ===== read %d bytes from input \n", counter);

/*
	printf("state variables:\n");
	for(int i = 0; i < 8; i++)
		printf("%u ", state[i]);
	printf("\n");
*/
	return ENDOFFILE;//Shouldn't get here; will make the compiler happier in HLS though
}

void sha256_hw_wrapper(hls::stream<ap_uint<9> > &rabinToSHA, hls::stream< uint8_t > &shaToDeduplicate)
//void sha256_hw_wrapper(hls::stream<ap_uint<9>>& rabinToSHA, hls::stream< uint8_t >& shaToDeduplicate)
{
	/** Collect chunk and send it to SHA_HW unit to  process it */
	/** If MSB = 1, it indicates end of chunk */
	for (int i = 0; i < MAX_CHUNKS_IN_HW_BUFFER; i++){
	        //#pragma HLS pipeline
	        int endingByte = sha256_hw_compute(rabinToSHA, shaToDeduplicate);
	        if (endingByte == ENDOFFILE){
	            return;
	        }
	    }

}


