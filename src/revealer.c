#include "revealer.h"

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
