#include "os.h"
#include "bolos_ux_common.h"
#include "ux_nanos.h"

// Mersene twister algo defines
#define N  		   	624
#define M 		   	397
#define UPPER_MASK 	0x80000000U
#define LOWER_MASK 	0x7fffffffU
#define MATRIX_A   	0x9908b0dfU
#define KEY_LEN		5

// image defines
#define CHUNK_SIZE 	250
#define IMG_WIDTH  	159
#define IMG_HEIGHT 	97
#define IMG_YX 	    1+(IMG_WIDTH*IMG_HEIGHT)/8

// Nvm struct to store prng (mersene twister) data and revealer image
typedef struct internalStorage_t {
    uint32_t mt[N];					// mersene twister table
    uint32_t index; 				// mersene twister table index
    uint32_t key[KEY_LEN];			// mersene twister key
	uint8_t key_len;				// mersene twister key len
	uint8_t revealer_image[IMG_YX];
	// Every byte of revealer image encodes 8 pixels
	// BYTE 0	BYTE 1 	...
	// bit7		bit7	...		
	// bit6		bit6	...
	// bit5		bit5	...
	// bit4		bit4	...
	// bit3		bit3	...
	// bit2		bit2	...
	// bit1		bit1	...
	// bit0		bit0	...
	// ...      ...		...
	// BYTE N   BYTE N+1
} internalStorage_t;

extern WIDE internalStorage_t N_storage_real;
#define N_storage (*(WIDE internalStorage_t *)PIC(&N_storage_real))

#define BAGL_FONT_FONT_11PX 0x00
#define BAGL_FONT_FONT_16PX 0x01

// Checks wether user entered noise seed is valid, last 3 chars of noise seed are last 3 char of hash(noise_seed)
uint8_t isNoise(char * string, uint8_t hashPos);
// Converts user entered noise seed to mersene twister key
void noiseSeedToKey(void);
// Initialize mersene twister table with mersene twister key
void init_by_array(uint8_t key_length);
void init_prng(uint32_t s);
// Generate random int32 using mersene twister table
uint32_t genrand_int32(void);
// Generate random bit using genrand_int32
uint8_t random_getrandbits(uint8_t k);
// Writes pseudo random noise in revealer_image
void write_noise(void);
