#include "revealer.h"
#include "ux_nanos.h"
#include "font.h"
#include "cx.h"
//#include "stdlib.h"

WIDE internalStorage_t N_storage_real;
#define N_storage (*(WIDE internalStorage_t *)PIC(&N_storage_real)) 


uint8_t isNoise(char * string, uint8_t hashPos){
	uint8_t computedHash[32];
	uint8_t hashIdCMP[4];
	int hashId = 0;
	uint8_t digit;
	
	cx_sha256_t context;
	cx_sha256_init(&context);
	cx_hash(&context,CX_LAST,string,hashPos,computedHash, 32);

	for (uint8_t n=0; n<3; n++){
		hashId += (string[n+hashPos] >= 0x30)&&(string[n+hashPos] <= 0x39)?(string[n+hashPos] - 0x30)<<4*(2-n):(string[n+hashPos] - 0x57)<<4*(2-n);
	}
	hashIdCMP[0] = computedHash[30]&0x0F;
	hashIdCMP[1] = computedHash[31];
	hashIdCMP[2] = (hashId>>8)&0x000F;
	hashIdCMP[3] = hashId&0x00FF;
	// Comparison fails if not using intermediary hashIdCMP
	// Invalid cast ?? TODO remove hashIdCMP and replace by valid compare
	if ((hashIdCMP[0] == hashIdCMP[2])&&(hashIdCMP[1] == hashIdCMP[3])){
		// Noise seed is Valid
		return 1;
	}
	return 0;
}

// test noise seed
// static const char  seed[] = "0eca2c8cfa19be8a64a7d76772253ac07";

void noiseSeedToKey(void){
	uint8_t byte = 0;
	uint8_t offset = 32;
	uint8_t shift = 0;
	uint8_t i = 0;
	uint32_t val = 0x00000000;

	nvm_write(N_storage.key, (uint32_t *)&val, KEY_LEN * sizeof(uint32_t));
	for (i = 0; i < KEY_LEN-1; i++){
		//nvm_write(&N_storage.mt[0], (uint32_t *)&s, sizeof(uint32_t));
		val = 0x00000000;
        shift = 0;
		for (uint8_t j = offset-8*i; j > offset-8*i - 8; j--){
			byte = G_bolos_ux_context.noise_seed[j];
			if (byte >= '0' && byte <= '9') byte = byte - '0';
        	else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
        	val += (byte&0x0F) << shift*4;
        	shift++;
		}
		nvm_write(&N_storage.key[i], (uint32_t *)&val, sizeof(uint32_t));
	}
	byte = G_bolos_ux_context.noise_seed[0];
	byte == (byte >= '0' && byte <= '9')?byte - '0':byte - 'a' + 10;
	val = (byte&0x0F);
	nvm_write(&N_storage.key[4], (uint32_t *)&val, sizeof(uint32_t));

	val = 5;
	for (i=4; i>=0; i--){
		if (N_storage.key[i]==0x00000000){
			val--;
		}
		else {
			break;
		}
	}
	nvm_write(&N_storage.key_len, (uint32_t *)&val, sizeof(uint32_t));
}

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
        val = (N_storage.mt[i] ^ ((N_storage.mt[i-1] ^ (N_storage.mt[i-1] >> 30)) * 1664525U)) + N_storage.key[j] + (uint32_t)j;
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
    #ifdef WORDS_IMG_DBG //Debug macro to write words only in image (no noise)
        if (colors[((ch>>i) & pixel_mask)] != 0) {
          val |= Ybitmask;
        }
        else {
          val &= ~Ybitmask;
        }        
    #else
      	if (colors[((ch>>i) & pixel_mask)] != 0) {
          val ^= Ybitmask; 
        }        
    #endif
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

int draw_string(bagl_font_t font_id, unsigned int fgcolor, unsigned int bgcolor, int x, int y, unsigned int width, unsigned int height, const void* text, unsigned int text_length, unsigned char text_encoding) {
  unsigned int xx;
  unsigned int colors[16];
  colors[0] = bgcolor;
  colors[1] = fgcolor;

  bagl_font_t *font = &font_id;

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
        /*if (ch != ' '){
        	ch-=0x20; //uppercase
        }*/
        text = (void*)(((unsigned char*)text)+1);
        break;
    }

    unsigned char ch_height = 0;
    unsigned char ch_kerning = 0;
    unsigned char ch_width = 0;
    const unsigned char * ch_bitmap = NULL;
    int ch_y = y;

    /*if (ch < font->first_char || ch > font->last_char) {
      //printf("invalid char");
      // can't proceed
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
    else {*/
      ch -= font->first_char;
      ch_bitmap = &font->bitmap[font->characters[ch].bitmap_offset];
      ch_width = font->characters[ch].char_width;
      ch_kerning = font->char_kerning;
      ch_height = font->char_height;
    //}

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

#define LINE_H_11		12
#define NUM_LINE_11		8

#define LINE_H_16		17
#define NUM_LINE_16		5

uint8_t max_char_l;
uint8_t charRemaining;

// Display helpers function, computes the next number of chars that fit one line without cutting a word, depending on the used font
// and returns the corresponding number of pixels for horizontal alignment
uint8_t getNextLinePixelWidth(const char* text, bagl_font_t font_id, uint8_t* numChar){
	uint8_t linePixelWidth, linePixelWidthInt, numCharInt;
	linePixelWidth = 0;
	numCharInt = 0;
	*numChar = 0;
	char ch;
	linePixelWidthInt = 0;

	bagl_font_t *font = &font_id;

	while (charRemaining-numCharInt>0){
		ch = *((unsigned char*)text);
		if ((ch==' ')||(ch==NULL)){
			linePixelWidth = linePixelWidthInt;
			*numChar = numCharInt;
		}
		// -18 t0 add 9*2 pixels on left and right
		if (linePixelWidthInt > IMG_WIDTH-18){
			return linePixelWidth;
		}
		ch -= font->first_char;
		linePixelWidthInt += font->characters[ch].char_width;
		numCharInt++;
		*text++;
	}
	return linePixelWidthInt;
}


void write_words(void){
	int bgcolor = 0x000000;
	int fgcolor = 0xFFFFFF;
	//uint8_t charRemainingDBG;
	uint8_t nextLineIdx, linePixCnt, rowPixCnt, numLine, line;
	uint8_t line_h;
	bagl_font_t font;
	charRemaining = G_bolos_ux_context.words_length; 
	line = 0;
	numLine = 0;
	//First check if words can fit in image with 16px font
	charRemaining += 1; //include '\0'
	strcpy(G_bolos_ux_context.string_buffer, G_bolos_ux_context.words);
	while ((charRemaining>0)&&(line+LINE_H_16 < IMG_HEIGHT)){
		linePixCnt = getNextLinePixelWidth(G_bolos_ux_context.string_buffer, fontFONT_16PX, &nextLineIdx);
		strcpy(G_bolos_ux_context.string_buffer, &G_bolos_ux_context.string_buffer[nextLineIdx+1]);
		charRemaining -= (nextLineIdx+1);
		numLine++;
		line+=LINE_H_16;
	}
	line = 0;
	//Chars remaining, words don't fit in image with 16px font > use 11px font
	if (charRemaining !=0){
		charRemaining = G_bolos_ux_context.words_length+1; // include \0 
		numLine = 0;
		line_h = LINE_H_11;
		font = fontFONT_11PX;
		strcpy(G_bolos_ux_context.string_buffer, G_bolos_ux_context.words);
		while ((charRemaining>0)&&(line+LINE_H_11 < IMG_HEIGHT)){
			linePixCnt = getNextLinePixelWidth(G_bolos_ux_context.string_buffer, fontFONT_11PX, &nextLineIdx);
			strcpy(G_bolos_ux_context.string_buffer, &G_bolos_ux_context.string_buffer[nextLineIdx+1]);
			charRemaining -= (nextLineIdx+1);
			numLine++;
			line+=LINE_H_11;
		}
		line = 0;
	}
	//No chars remaining, words are fitting with 16px font
	else {					
		line_h = LINE_H_16;
		font = fontFONT_16PX;
	}
	// Write words in choosen font 
	line+= (IMG_HEIGHT - numLine*line_h)/2;
	charRemaining = G_bolos_ux_context.words_length+1; // include \0
	while ((charRemaining>0)&&(line+line_h < IMG_HEIGHT)){
		linePixCnt = getNextLinePixelWidth(G_bolos_ux_context.words, font, &nextLineIdx);
		draw_string(font, fgcolor, bgcolor, (IMG_WIDTH-linePixCnt)/2,  line, IMG_WIDTH, IMG_HEIGHT, G_bolos_ux_context.words, nextLineIdx, BAGL_ENCODING_LATIN1);
		strcpy(G_bolos_ux_context.words, &G_bolos_ux_context.words[nextLineIdx+1]); // Dump space char
		charRemaining -= (nextLineIdx+1);
		// SPRINTF(G_bolos_ux_context.string_buffer, "%d %d %d %d\0", linePixCnt, numLine, nextLineIdx, charRemaining);
		// draw_string(font, fgcolor, bgcolor, 0,  line, IMG_WIDTH, IMG_HEIGHT, G_bolos_ux_context.string_buffer, strlen(G_bolos_ux_context.string_buffer), BAGL_ENCODING_LATIN1);	
		line += line_h;
	}
}

void write_noise(void){
	int YX;
	uint8_t x, y, val, cpt;
	cpt = 0;
	val = 0;
	for (x=0; x<IMG_WIDTH; x++){
		for (y=0; y<IMG_HEIGHT/8; y++){
			YX = y*IMG_WIDTH + x;
			val = 0;
			for (uint8_t j=0; j<8; j++){
				val += (random_getrandbits(2)&1)<<j;
			}
			nvm_write(&N_storage.revealer_image[YX], (uint8_t *)&val, sizeof(uint8_t));
		}
		//IMG_HEIGHT = 97 = 12*8+1 => dump last pixel 
		random_getrandbits(2);
	}
}

int send_row(int x){
	int YX;
	uint8_t idx;
	idx = 0;
	uint8_t val;
	for (int i=0; i<IMG_HEIGHT/8; i++){
		YX = i*IMG_WIDTH+x;
		val = N_storage.revealer_image[YX];
		for (uint8_t j=0; j<8; j++){
			G_io_apdu_buffer[idx] = (val>>j)&1;
			idx++;
			if (idx == IMG_HEIGHT-1){
				break;
			}
		}
	}
	G_io_apdu_buffer[IMG_HEIGHT-1] = 0;
	return IMG_HEIGHT;
}

