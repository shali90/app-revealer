#include "os.h"
#include "bolos_ux_common.h"
#include "ux_nanos.h"

void noiseSeedToKey(void);
#define N  	624
#define M 397
#define UPPER_MASK 0x80000000U
#define LOWER_MASK 0x7fffffffU
#define MATRIX_A   0x9908b0dfU

typedef struct internalStorage_t {
// #define STORAGE_MAGIC 0xDEAD1337
//     uint32_t magic;
    uint32_t mt[N];
    uint32_t index;
} internalStorage_t;

extern WIDE internalStorage_t N_storage_real;
#define N_storage (*(WIDE internalStorage_t *)PIC(&N_storage_real))
