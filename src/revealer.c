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

	G_revealer.key_len = 5;
	for (i=4; i>=0; i--){
		if (G_revealer.key[i]==0x00000000){
			G_revealer.key_len--;
		}
		else {
			break;
		}
	}

	/*G_io_apdu_buffer[0] =  G_revealer.key[0]&0x000000FF;
	G_io_apdu_buffer[1] = (G_revealer.key[0]&0x0000FF00)>>8;
	G_io_apdu_buffer[2] = (G_revealer.key[0]&0x00FF0000)>>16;
	G_io_apdu_buffer[3] = (G_revealer.key[0]&0xFF000000)>>24;*/

	//G_io_apdu_buffer[0]=G_revealer.key_len;

}

WIDE internalStorage_t N_storage_real;
#define N_storage (*(WIDE internalStorage_t *)PIC(&N_storage_real)) 


void init_prng(uint32_t s){
    uint32_t mti;
    uint32_t val;
    nvm_write(&N_storage.mt[0], (uint32_t *)&s, sizeof(uint32_t));
    for (mti=1; mti<N; mti++) {
        val = (1812433253U * (N_storage.mt[mti-1] ^ (N_storage.mt[mti-1] >> 30)) + mti);
        nvm_write(&N_storage.mt[mti], (uint32_t *)&val, sizeof(uint32_t));
    }
    nvm_write(&N_storage.index, (uint32_t *)&mti, sizeof(uint32_t));

    G_io_apdu_buffer[0] =  N_storage.mt[10]&0x000000FF;
    G_io_apdu_buffer[1] = (N_storage.mt[10]&0x0000FF00)>>8;
    G_io_apdu_buffer[2] = (N_storage.mt[10]&0x00FF0000)>>16;
    G_io_apdu_buffer[3] = (N_storage.mt[10]&0xFF000000)>>24;
}

void init_by_array(uint8_t key_length){
	uint32_t i, j, k, val;
	init_prng(19650218U);
	i = 1;
	j = 0;
	k = (N>key_length ? N : key_length);
	//k = N;
	for (; k; k--){
		val = (N_storage.mt[i] ^ ((N_storage.mt[i-1] ^ (N_storage.mt[i-1] >> 30)) * 1664525U))+ G_revealer.key[j] + (uint32_t)j;
		i++;
		j++;
		nvm_write(&N_storage.mt[i], (uint32_t *)&val, sizeof(uint32_t));
		if (i>=N) {
			val = N_storage.mt[N-1];
			nvm_write(&N_storage.mt[0], (uint32_t *)&val, sizeof(uint32_t)); 
			i=1; 
		}
        if (j>=key_length) j=0;
	}
	for (k=N-1; k; k--) {
        val = (N_storage.mt[i] ^ ((N_storage.mt[i-1] ^ (N_storage.mt[i-1] >> 30)) * 1566083941U))- (uint32_t)i;
        nvm_write(&N_storage.mt[i], (uint32_t *)&val, sizeof(uint32_t));
        i++;
        if (i>=N) {
        	val = N_storage.mt[N-1];
        	nvm_write(&N_storage.mt[0], (uint32_t *)&val, sizeof(uint32_t));
        	i=1;
        }
    }
    val = 0x80000000U;
    nvm_write(&N_storage.mt[0], (uint32_t *)&val, sizeof(uint32_t));

    G_io_apdu_buffer[0] =  N_storage.mt[10]&0x000000FF;
    G_io_apdu_buffer[1] = (N_storage.mt[10]&0x0000FF00)>>8;
    G_io_apdu_buffer[2] = (N_storage.mt[10]&0x00FF0000)>>16;
    G_io_apdu_buffer[3] = (N_storage.mt[10]&0xFF000000)>>24;
}