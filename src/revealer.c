#include "revealer.h"
#include "ux_nanos.h"
#include "font.h"



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

// test noise seed
//static const char  seed[] = "0eca2c8cfa19be8a64a7d76772253ac07";

void noiseSeedToKey(void){
	uint8_t byte = 0;
	uint8_t offset = 32;
	uint8_t shift = 0;
	uint8_t i = 0;

	for (i = 0; i < KEY_LEN-1; i++){
        shift = 0;
		for (uint8_t j = offset-8*i; j > offset-8*i - 8; j--){
			byte = G_revealer.noise_seed[j];
			if (byte >= '0' && byte <= '9') byte = byte - '0';
        	else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
        	G_revealer.key[i] += (byte&0x0F) << shift*4;
        	shift++;
		}
	}
	byte = G_revealer.noise_seed[0];
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
}

void init_by_array(uint8_t key_length){
	uint32_t i, j, k, val;
	init_prng(19650218U);
	i = 1;
	j = 0;
	k = (N>key_length ? N : key_length);
	for (; k; k--) {
        val = (N_storage.mt[i] ^ ((N_storage.mt[i-1] ^ (N_storage.mt[i-1] >> 30)) * 1664525U)) + G_revealer.key[j] + (uint32_t)j;
        nvm_write(&N_storage.mt[i], (uint32_t *)&val, sizeof(uint32_t));
        i++; 
        j++;
        if (i>=N) {
        	val = N_storage.mt[N-1]; 
        	nvm_write(&N_storage.mt[0], (uint32_t *)&val, sizeof(uint32_t));
        	i=1;
        }
        if (j>=key_length) j=0;
    }
    for (k=N-1; k; k--) {
        val = (N_storage.mt[i] ^ ((N_storage.mt[i-1] ^ (N_storage.mt[i-1] >> 30)) * 1566083941U)) - (uint32_t)i;
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
}

uint32_t genrand_int32(void){
	uint32_t y, val;
	static const uint32_t mag01[2] = {0x0U, MATRIX_A};

	if (N_storage.index >=N){
		int kk;
		for (kk=0;kk<N-M;kk++) {
            y = (N_storage.mt[kk]&UPPER_MASK)|(N_storage.mt[kk+1]&LOWER_MASK);
            val = N_storage.mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1U];
            nvm_write(&N_storage.mt[kk], (uint32_t *)&val, sizeof(uint32_t));
        }
        for (;kk<N-1;kk++) {
            y = (N_storage.mt[kk]&UPPER_MASK)|(N_storage.mt[kk+1]&LOWER_MASK);
            val = N_storage.mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1U];
            nvm_write(&N_storage.mt[kk], (uint32_t *)&val, sizeof(uint32_t));
        }
        
        y = (N_storage.mt[N-1]&UPPER_MASK)|(N_storage.mt[0]&LOWER_MASK);
        val = N_storage.mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1U];
        nvm_write(&N_storage.mt[N-1], (uint32_t *)&val, sizeof(uint32_t));
        val = 0;
        nvm_write(&N_storage.index, (uint32_t *)&val, sizeof(uint32_t));
	}
	val = N_storage.index;
	y = N_storage.mt[val++];
	nvm_write(&N_storage.index, (uint32_t *)&val, sizeof(uint32_t));
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680U;
    y ^= (y << 15) & 0xefc60000U;
    y ^= (y >> 18);
	return y;
}

uint8_t random_getrandbits(uint8_t k){
	uint8_t ret;
	do {
		ret = (uint8_t)(genrand_int32()>>32-k);		
	}while (ret >1);
	return ret;
}

// Code below is adapted from Nano S MCU screen HAL driver, screen is replaced by an image matrix stored in nvram cf revealer.h
unsigned int screen_changed; // to avoid screen update for nothing

int screen_draw_x;
int screen_draw_y;
unsigned int screen_draw_width;
unsigned int screen_draw_height;
int screen_draw_YX;
int screen_draw_YXlinemax;
int screen_draw_Ybitmask;
unsigned int* screen_draw_colors;

void screen_clear(void) {
  char val;
  val = 0x00;
  //memset(revealer_image, 0, sizeof(revealer_image)); 
  for (int i=0; i<(IMG_WIDTH*IMG_HEIGHT)/8; i++){
  	nvm_write(&N_storage.revealer_image[i], (char *)&val, sizeof(char));  	
  }
  screen_changed = 1;
}


void draw_bitmap_within_rect_internal(unsigned int bit_per_pixel, const unsigned char* bitmap, unsigned int bitmap_length_bits) {
  unsigned int i;
  int xx;  
  //unsigned int pixel_mask = (1<<bit_per_pixel)-1;
  #define pixel_mask 1 // 2 colors only

  int x = screen_draw_x;
  xx = x;
  int y = screen_draw_y;
  unsigned int width = screen_draw_width;
  unsigned int height = screen_draw_height;
  int YX = screen_draw_YX;
  int YXlinemax = screen_draw_YXlinemax;
  int Ybitmask = screen_draw_Ybitmask;
  unsigned int* colors = screen_draw_colors;
  char val, val2;
  screen_changed=1;
  val2 = 0xFFFFFFFF;
  while(bitmap_length_bits) {
    // horizontal scan transformed into vertical
    unsigned int ch = *bitmap++;
    // draw each pixel (at most 256 index color bitmap support)
    
    for (i = 0; i < 8 && bitmap_length_bits; bitmap_length_bits -= bit_per_pixel, i += bit_per_pixel) {
    //for (i = 0; i < 8 ; i++) {
      // grab LSB to MSB bits
      // 2 colors only
      // bit    colorsarray     painted
      // 0      [0]=0               0
      // 1      [1]=0               0
      // 0      [0]=1               1
      // 1      [1]=1               1
      // 
      if (y>=0 && xx>=0) { // else we're out of screen
        val = N_storage.revealer_image[YX];
    	if (colors[((ch>>i) & pixel_mask)] != 0) {
          val |= Ybitmask;
        }
        else {
          val &= ~Ybitmask;
        }
        nvm_write(&N_storage.revealer_image[YX], (char*)&val, sizeof(char));
      }


      xx++;
      YX++;
      if (YX >= YXlinemax) {
      	y++;
        height--;
        // update fast bit operation variables
        YX = (y/8)*IMG_WIDTH + x;
        xx = x;
        YXlinemax = YX + width;
        Ybitmask = 1<<(y%8);
      }
      if (height == 0) {
      	goto end;
      }
    }
  }
  // save for continue
end:
  screen_draw_x = x;
  screen_draw_y = y;
  screen_draw_width = width;
  screen_draw_height = height;
  screen_draw_YX = YX;
  screen_draw_YXlinemax = YXlinemax;
  screen_draw_Ybitmask = Ybitmask;
  return;
}

void draw_bitmap_within_rect(int x, int y, unsigned int width, unsigned int height, unsigned int color_count, const unsigned int *colors, unsigned int bit_per_pixel, const unsigned char* bitmap, unsigned int bitmap_length_bits) {
  // horizontal scan
  if (x>= IMG_WIDTH || y >= IMG_HEIGHT) {
    return;
  }
  if (x+width > IMG_WIDTH) {
    width = IMG_WIDTH-x;
  }

  if (y+height > IMG_HEIGHT) {
    height = IMG_HEIGHT-y;
  }

  int YX = (y/8)*IMG_WIDTH + x;
  int Ybitmask = 1<<(y%8); 
  int YXlinemax = YX + width;

  // run bitmap draw
  screen_draw_x = x;
  screen_draw_y = y;
  screen_draw_width = width;
  screen_draw_height = height;
  screen_draw_YX = YX;
  screen_draw_YXlinemax = YXlinemax;
  screen_draw_Ybitmask = Ybitmask;
  screen_draw_colors = colors;

  draw_bitmap_within_rect_internal(bit_per_pixel, bitmap, bitmap_length_bits);
}

/*void draw_bitmap_continue(unsigned int bit_per_pixel, const unsigned char* bitmap, unsigned int bitmap_length_bits) {
  draw_bitmap_within_rect_internal(bit_per_pixel, bitmap, bitmap_length_bits);
}*/

/*void draw_rect(unsigned int color, int x, int y, unsigned int width, unsigned int height) {
  unsigned int i;

  if (x+width > IMG_WIDTH || x < 0) {
    return;
  }
  if (y+height > IMG_HEIGHT || y < 0) {
    return;
  }

  unsigned int YX = (y/8)*IMG_WIDTH + x;
  unsigned int Ybitmask = 1<<(y%8); 
  unsigned int YXlinemax = YX + width;
  char val;

  screen_changed=1;

  i = width*height;
  while(i--) {
    // 2 colors only
  	val = N_storage.revealer_image[YX];
    if (color) {
      val |= Ybitmask;
    }
    else {
      val &= ~Ybitmask;
    }
    nvm_write(&N_storage.revealer_image[YX], (char*)&val, sizeof(char));
    YX++;
    if (YX >= YXlinemax) {
      y++;
      height--;
      // update fast bit operation variables
      YX = (y/8)*IMG_WIDTH + x;
      YXlinemax = YX + width;
      Ybitmask = 1<<(y%8);
    }

    if (height == 0) {
      break;
    }
  }
}*/

int draw_string(unsigned short font_id, unsigned int fgcolor, unsigned int bgcolor, int x, int y, unsigned int width, unsigned int height, const void* text, unsigned int text_length, unsigned char text_encoding) {
  unsigned int xx;
  unsigned int colors[16];
  colors[0] = bgcolor;
  colors[1] = fgcolor;

  const bagl_font_t *font = &fontFONT_11PX;
  if (font == NULL) {
    return 0;
  }

  // always comparing this way, very optimized etc
  width += x;
  height += y;

  // initialize first index
  xx = x;

  //printf("display text: %s\n", text);

  // depending on encoding
  while (text_length--) {
    unsigned int ch = 0;
    // TODO support other encoding than ascii ISO8859 Latin
    switch(text_encoding) {
      default:
      case BAGL_ENCODING_LATIN1:
        ch = *((unsigned char*)text); 
        if (ch != ' '){
        	ch-=0x20; //uppercase
        }
        text = (void*)(((unsigned char*)text)+1);
        break;
    }

    unsigned char ch_height = 0;
    unsigned char ch_kerning = 0;
    unsigned char ch_width = 0;
    const unsigned char * ch_bitmap = NULL;
    int ch_y = y;

    if (ch < font->first_char || ch > font->last_char) {
      //printf("invalid char");
      // can't proceed
      //THROW(0x6FFF);
      if (ch == '\n' || ch == '\r') {
        y += ch_height; // no interleave

        // IGNORED for first line
        if (y + ch_height > height) {
          // we're writing half height of the last line ... probably better to put some dashes
          return (y<<16)|(xx&0xFFFF);
        }

        // newline starts back at first x offset
        xx = x;
        continue;
      }

      // SKIP THE CHAR // return (y<<16)|(xx&0xFFFF);
      ch_width = 0;
      if (ch >= 0xC0) {
        ch_width = ch & 0x3F;
        ch_height = font->char_height;
      }
      else if (ch >= 0x80) {
        // open the glyph font
        //const bagl_font_t *font_symbols = bagl_get_font((ch&0x20)?BAGL_FONT_SYMBOLS_1:BAGL_FONT_SYMBOLS_0);
      	const bagl_font_t *font_symbols = BAGL_FONT_SYMBOLS_1;
        if (font_symbols != NULL) {
          ch_bitmap = &font_symbols->bitmap[font_symbols->characters[ch & 0x1F].bitmap_offset];
          ch_width = font_symbols->characters[ch & 0x1F].char_width;
          ch_height = font_symbols->char_height;
          // align baselines
          ch_y = y + font->baseline_height - font_symbols->baseline_height;
        }
      }
    }
    else {
      ch -= font->first_char;
      ch_bitmap = &font->bitmap[font->characters[ch].bitmap_offset];
      ch_width = font->characters[ch].char_width;
      ch_kerning = font->char_kerning;
      ch_height = font->char_height;
    }

    // retrieve the char bitmap

    // go to next line if needed
    if (xx + ch_width > width) {
      y += ch_height; // no interleave

      // IGNORED for first line
      if (y + ch_height > height) {
        // we're writing half height of the last line ... probably better to put some dashes
        return (y<<16)|(xx&0xFFFF);
      }

      // newline starts back at first x offset
      xx = x;
    }

    /* IGNORED for first line
    if (y + ch_height > height) {
        // we're writing half height of the last line ... probably better to put some dashes
        return;
    }
    */

    // chars are storred LSB to MSB in each char, packed chars. horizontal scan
    if (ch_bitmap) {
      draw_bitmap_within_rect(xx, ch_y, ch_width, ch_height, (1<<font->bpp), colors, font->bpp, ch_bitmap, font->bpp*ch_width*ch_height); // note, last parameter is computable could be avoided
    }
    else {
      //draw_rect(bgcolor, xx, ch_y, ch_width, ch_height);
    }
    // prepare for next char
    xx += ch_width + ch_kerning;
  }

  // return newest position, for upcoming printf
  return (y<<16)|(xx&0xFFFF);
}
#define MAX_CHAR	21

uint8_t getNextLineIdx(char *words){
	uint8_t numChar, numCharInt, offset;
	numCharInt = 0;
	while (*words++!='\0'){
		numCharInt++;
		if (*words==' '){
			numChar = numCharInt;
		}
		if (numCharInt > MAX_CHAR){
			return numChar + 1;
		}
	}
	return numCharInt;
}

#define SEED_SIZE 160

void write_words(void){
	char  text[SEED_SIZE];
	char  text_display[MAX_CHAR];
	//test words
	//SPRINTF(text, "TRAFFIC POWDER RURAL WISH BLESS BEGIN TEXT PYRAMID SECOND FEED ANOTHER PANEL WRECK WOMAN DUTCH CHAIR REMOVE ERUPT PROPERTY BURGER AUTHOR FANTASY TWIST RANDOM");
	strcpy(text,G_bolos_ux_context.words_buffer);
	int bgcolor = 0x000000;
	int fgcolor = 0xFFFFFF;

	uint8_t nextLineIdx, curLineIdx;

	screen_clear();

	//THROW(0x6FFF);
	curLineIdx = 0;
	nextLineIdx = getNextLineIdx(text);
	memcpy(text_display, &text[curLineIdx], nextLineIdx);
	draw_string(BAGL_FONT_FONT_11PX, fgcolor, bgcolor, 0,  0, IMG_WIDTH, IMG_HEIGHT, text_display, nextLineIdx, BAGL_ENCODING_LATIN1);
	strcpy(text, &text[nextLineIdx]);

	nextLineIdx = getNextLineIdx(text);
	memcpy(text_display, text, nextLineIdx);
	draw_string(BAGL_FONT_FONT_11PX, fgcolor, bgcolor, 0,  12, IMG_WIDTH, IMG_HEIGHT, text_display, nextLineIdx, BAGL_ENCODING_LATIN1);
	strcpy(text, &text[nextLineIdx]);

	nextLineIdx = getNextLineIdx(text);
	memcpy(text_display, text, nextLineIdx);
	draw_string(BAGL_FONT_FONT_11PX, fgcolor, bgcolor, 0,  24, IMG_WIDTH, IMG_HEIGHT, text_display, nextLineIdx, BAGL_ENCODING_LATIN1);
	strcpy(text, &text[nextLineIdx]);

	nextLineIdx = getNextLineIdx(text);
	memcpy(text_display, text, nextLineIdx);
	draw_string(BAGL_FONT_FONT_11PX, fgcolor, bgcolor, 0,  36, IMG_WIDTH, IMG_HEIGHT, text_display, nextLineIdx, BAGL_ENCODING_LATIN1);
	strcpy(text, &text[nextLineIdx]);

	nextLineIdx = getNextLineIdx(text);
	memcpy(text_display, text, nextLineIdx);
	draw_string(BAGL_FONT_FONT_11PX, fgcolor, bgcolor, 0,  48, IMG_WIDTH, IMG_HEIGHT, text_display, nextLineIdx, BAGL_ENCODING_LATIN1);
	strcpy(text, &text[nextLineIdx]);

	nextLineIdx = getNextLineIdx(text);
	memcpy(text_display, text, nextLineIdx);
	draw_string(BAGL_FONT_FONT_11PX, fgcolor, bgcolor, 0,  60, IMG_WIDTH, IMG_HEIGHT, text_display, nextLineIdx, BAGL_ENCODING_LATIN1);
	strcpy(text, &text[nextLineIdx]);

	nextLineIdx = getNextLineIdx(text);
	memcpy(text_display, text, nextLineIdx);
	draw_string(BAGL_FONT_FONT_11PX, fgcolor, bgcolor, 0,  72, IMG_WIDTH, IMG_HEIGHT, text_display, nextLineIdx, BAGL_ENCODING_LATIN1);
	strcpy(text, &text[nextLineIdx]);

	nextLineIdx = getNextLineIdx(text);
	memcpy(text_display, text, nextLineIdx);
	draw_string(BAGL_FONT_FONT_11PX, fgcolor, bgcolor, 0,  84, IMG_WIDTH, IMG_HEIGHT, text_display, nextLineIdx, BAGL_ENCODING_LATIN1);
	strcpy(text, &text[nextLineIdx]);
}

int send_column(int x){
	int YX;
	uint8_t idx;
	idx = 0;
	uint8_t val;
	for (int i=0; i<IMG_HEIGHT/8; i++){
		YX = i*IMG_WIDTH+x;
		val = N_storage.revealer_image[YX];	
		for (uint8_t j=0; j<8; j++){
			G_io_apdu_buffer[idx] = (val>>j)&1;
			G_io_apdu_buffer[idx] ^= random_getrandbits(2);
			idx++;
			if (idx == IMG_HEIGHT-1){
				break;
			}
		}
	}
	G_io_apdu_buffer[IMG_HEIGHT-1] = 0;
	G_io_apdu_buffer[IMG_HEIGHT-1] ^= random_getrandbits(2);
	return IMG_HEIGHT;
}