#include "os.h"
#include "bolos_ux_common.h"
#include "ux_nanos.h"

void noiseSeedToKey(void);
#define N  	624
#define M 397
#define UPPER_MASK 0x80000000U
#define LOWER_MASK 0x7fffffffU
#define MATRIX_A   0x9908b0dfU

#define CHUNK_SIZE 250

#define IMG_WIDTH  159
#define IMG_HEIGHT 97



#define IMG_YX 	   1+(IMG_WIDTH*IMG_HEIGHT)/8

typedef struct internalStorage_t {
    uint32_t mt[N];	// mersene twister table
    uint32_t index; // mersene twister table index
	char revealer_image[IMG_YX];	
} internalStorage_t;

extern WIDE internalStorage_t N_storage_real;
#define N_storage (*(WIDE internalStorage_t *)PIC(&N_storage_real))

#define BAGL_FONT_FONT_11PX 0
