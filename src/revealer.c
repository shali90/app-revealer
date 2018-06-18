#include "revealer.h"
#include "ux_nanos.h"

//ux_revealer G_revealer;

uint8_t isNoise(char * string, uint8_t hashPos){
	uint8_t computedHash[32];

	cx_sha256_t context;
	cx_sha256_init(&context);
	cx_hash(&context,CX_LAST,string,hashPos,computedHash);

	if ((0x0F & computedHash[30]) == (0x0F & *(string+hashPos)) && \
        (computedHash[31]>>4)     == (0x0F & *(string+hashPos+1)) && \
        (0x0F & computedHash[31]) == (0x0F & *(string+hashPos+2))){
		return 1;
    }	
	return 0;
}

//TODO replace with G_revealer.noise_seed
static const char  seed[] = "0eca2c8cfa19be8a64a7d76772253ac07";
//prng G_prng;

//#define KEY_LEN 5

void noiseSeedToKey(void){
	uint8_t byte = 0;
	uint8_t offset = 32;
	uint8_t shift = 0;
	uint8_t i = 0;

	for (i = 0; i < KEY_LEN-1; i++){
        shift = 0;
		for (uint8_t j = offset-8*i; j > offset-8*i - 8; j--){
			//TODO replace with G_revealer.noise_seed
			byte = seed[j];
			if (byte >= '0' && byte <= '9') byte = byte - '0';
        	else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
        	//else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;
        	G_revealer.key[i] += (byte&0x0F) << shift*4;
        	shift++;
		}
	}
	byte = seed[0];
	byte == (byte >= '0' && byte <= '9')?byte - '0':byte - 'a' + 10;
	G_revealer.key[4] = (byte&0x0F);
}

void init_prng(uint32_t s){
    int mti;
    G_prng.mt[0] = s;    
    for (mti=1; mti<N; mti++) {
        G_prng.mt[mti] = (1812433253U * (G_prng.mt[mti-1] ^ (G_prng.mt[mti-1] >> 30)) + mti);
    }    
    G_prng.index = mti;    
}