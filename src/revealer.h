#include "os.h"
#include "bolos_ux_common.h"
#include "ux_nanos.h"

void noiseSeedToKey(void);
uint8_t get_words(void);
#define N  	624
#define M 397
#define UPPER_MASK 0x80000000U
#define LOWER_MASK 0x7fffffffU
#define MATRIX_A   0x9908b0dfU

#define WIDTH 	   159
#define HEIGHT	   97

#define CHUNK_SIZE 250
#define IMG_SIZE   1928

typedef struct internalStorage_t {
// #define STORAGE_MAGIC 0xDEAD1337
//     uint32_t magic;
    uint32_t mt[N];	// mersene twister table
    uint32_t index; // mersene twister table index
    uint8_t  noise_img[IMG_SIZE]; // one byte for 8 pixels 
    //uint8_t  words_img[IMG_SIZE]; // one byte for 8 pixels

	// image is 159*97 = 15423 pixels
	// one img octet represents 8 pixel
	// 15423/8 = 1927,875 ~ 1928
	// one _img elem is 4 image octet
	// _img[] size is 1928/4 = 482
    //uint32_t noise_img[482];
    //uint32_t words_img[482];
} internalStorage_t;

extern WIDE internalStorage_t N_storage_real;
#define N_storage (*(WIDE internalStorage_t *)PIC(&N_storage_real))
