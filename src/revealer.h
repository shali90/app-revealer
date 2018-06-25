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

#define IMG_WIDTH  159
#define IMG_HEIGHT 97



typedef struct internalStorage_t {
// #define STORAGE_MAGIC 0xDEAD1337
//     uint32_t magic;
    uint32_t mt[N];	// mersene twister table
    uint32_t index; // mersene twister table index
    uint8_t  noise_img[IMG_SIZE]; // one byte for 8 pixels
	char screen_framebuffer[(IMG_WIDTH*IMG_HEIGHT)/8];
    //char screen_framebuffer[(WIDTH*HEIGHT)/8]; 
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

//#include "string.h"
//#include "bagl.h"

/*******************************************************************************
 * FONT REQUIRED FORMAT                                                        *
 * Data length: 8 bits                                                         *
 * Invert bits: No                                                             *
 * Data format: Little Endian, Row based, Row preferred, Packed                *
 *******************************************************************************/

//#include "bagl_font_rom.inc"

/*const bagl_font_t fontNONE = {
	-1UL, 
   0 , 
   0 , 
   0, 
   0x0000, 
   0x0000, 
   NULL,  
   NULL   
};*/
#define BAGL_FONT_FONT_11PX 0
//#include "font.h"


/*extern const bagl_font_t* const C_revealer_fonts[] = {

&fontFONT_11PX ,


};*/

//const unsigned int C_revealer_fonts_count = sizeof(C_revealer_fonts)/sizeof(C_revealer_fonts[0]);
