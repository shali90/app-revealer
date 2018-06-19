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

    /*G_io_apdu_buffer[0] =  N_storage.mt[10]&0x000000FF;
    G_io_apdu_buffer[1] = (N_storage.mt[10]&0x0000FF00)>>8;
    G_io_apdu_buffer[2] = (N_storage.mt[10]&0x00FF0000)>>16;
    G_io_apdu_buffer[3] = (N_storage.mt[10]&0xFF000000)>>24;*/
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


    /*G_io_apdu_buffer[0] =  N_storage.mt[623]&0x000000FF;
    G_io_apdu_buffer[1] = (N_storage.mt[623]&0x0000FF00)>>8;
    G_io_apdu_buffer[2] = (N_storage.mt[623]&0x00FF0000)>>16;
    G_io_apdu_buffer[3] = (N_storage.mt[623]&0xFF000000)>>24;*/
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

	/*G_io_apdu_buffer[0] =  N_storage.mt[1]&0x000000FF;
    G_io_apdu_buffer[1] = (N_storage.mt[1]&0x0000FF00)>>8;
    G_io_apdu_buffer[2] = (N_storage.mt[1]&0x00FF0000)>>16;
    G_io_apdu_buffer[3] = (N_storage.mt[1]&0xFF000000)>>24;*/


	val = N_storage.index;
	y = N_storage.mt[val++];
	nvm_write(&N_storage.index, (uint32_t *)&val, sizeof(uint32_t));
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680U;
    y ^= (y << 15) & 0xefc60000U;
    y ^= (y >> 18);
	/*G_io_apdu_buffer[0] =  y&0x000000FF;
    G_io_apdu_buffer[1] = (y&0x0000FF00)>>8;
    G_io_apdu_buffer[2] = (y&0x00FF0000)>>16;
    G_io_apdu_buffer[3] = (y&0xFF000000)>>24;*/
    return y;
}

uint8_t random_getrandbits(uint8_t k){
	uint8_t ret;
	do {
		ret = (uint8_t)(genrand_int32()>>32-k);		
	}while (ret >1);
	return ret;
}

uint8_t get_words(void){
	int offset = 0;
    int i = 0;
    switch (G_bolos_ux_context.onboarding_kind){
      case 24:
        offset = 128;
        break;
      case 18:
        offset = 64;
        break;
      case 12:
        offset = 64;
        break;
      default:
        offset = 0;
        break;
    }
    while(G_bolos_ux_context.string_buffer[offset] != 0x00){
        G_io_apdu_buffer[i] = G_bolos_ux_context.string_buffer[offset];
        offset++;
        i++;
    }
    return i;
}

const uint8_t a[9][9] = {
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},
{0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,},
{0x00,0x00,0x00,0x01,0x01,0x01,0x00,0x00,0x00,},
{0x00,0x00,0x01,0x01,0x00,0x01,0x01,0x00,0x00,},
{0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,},
{0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x01,0x01,},
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,},
};

/*int write_words(void){
	uint8_t val = 0x00;
	uint8_t i, j, shift;
	shift = 0;
	for (int i=0; i<1928; i++){
		for (uint8_t j=0; j<8; j++){
			val |= a[i%9][j] << j;
		}
		nvm_write(&N_storage.words_img[i], (uint8_t *)&val, sizeof(uint8_t));	
		val = 0;
	}
	return 159*97;
}*/

int write_noise(void){
	uint8_t val;
	val = 0;
	for (int i=0; i<IMG_SIZE; i++){
		for(uint8_t j=0; j<8; j++){
			val |= random_getrandbits(2)<<j;
		}
		nvm_write(&N_storage.noise_img[i], (uint8_t *)&val, sizeof(uint8_t));	
		val = 0;
	}
	return IMG_SIZE;
}

int send_img_chunk(int chunk_nb){
	int idx = chunk_nb * CHUNK_SIZE;
	//os_memcpy(G_io_apdu_buffer, &N_storage.words_img[idx], 250);
	os_memcpy(G_io_apdu_buffer, &N_storage.noise_img[idx], CHUNK_SIZE);
	return CHUNK_SIZE;
}

#if 0
uint8_t display_icon(bagl_component_t* icon_component, bagl_icon_details_t* icon_details) {
  bagl_component_t icon_component_mod;
  uint8_t size;
  // ensure not being out of bounds in the icon component agianst the declared icon real size
  os_memmove(&icon_component_mod, icon_component, sizeof(bagl_component_t));
  icon_component_mod.width = icon_details->width;
  icon_component_mod.height = icon_details->height;
  icon_component = &icon_component_mod;


  // component type = ICON, provided bitmap
  // => bitmap transmitted


  // color index size
  unsigned int h = (1<<(icon_details->bpp))*sizeof(unsigned int); 
  // bitmap size
  unsigned int w = ((icon_component->width*icon_component->height*icon_details->bpp)/8)+((icon_component->width*icon_component->height*icon_details->bpp)%8?1:0);
  unsigned short length = sizeof(bagl_component_t)
                          +1 /* bpp */
                          +h /* color index */
                          +w; /* image bitmap size */
  G_io_apdu_buffer[0] = SEPROXYHAL_TAG_SCREEN_DISPLAY_STATUS;
  G_io_apdu_buffer[1] = length>>8;
  G_io_apdu_buffer[2] = length;
  /*io_seproxyhal_spi_send(G_io_seproxyhal_spi_buffer, 3);
  io_seproxyhal_spi_send((unsigned char*)icon_component, sizeof(bagl_component_t));*/
  G_io_apdu_buffer[3] = icon_details->bpp;
  os_memcpy(&G_io_apdu_buffer[4],   (unsigned char*)PIC(icon_details->colors), h);
  os_memcpy(&G_io_apdu_buffer[4+h], (unsigned char*)PIC(icon_details->bitmap), w);
  size += 3+length;

  return size;
}

uint8_t display_bitmap(int x, int y, unsigned int w, unsigned int h, unsigned int* color_index, unsigned int bit_per_pixel, unsigned char* bitmap) {
  // component type = ICON
  // component icon id = 0
  // => bitmap transmitted
  
  bagl_component_t c;
  bagl_icon_details_t d;
  os_memset(&c, 0, sizeof(c));
  c.type = BAGL_ICON;
  c.x = x;
  c.y = y;
  c.width = w;
  c.height = h;
  // done by memset // c.icon_id = 0;
  d.width = w;
  d.height = h;
  d.bpp = bit_per_pixel;
  d.colors = color_index;
  d.bitmap = bitmap;

  return display_icon(&c, &d);
  /*
  // color index size
  h = ((1<<bit_per_pixel)*sizeof(unsigned int)); 
  // bitmap size
  w = ((w*c.height*bit_per_pixel)/8)+((w*c.height*bit_per_pixel)%8?1:0);
  unsigned short length = sizeof(bagl_component_t)
                          +1 // bpp 
                          +h // color index
                          +w; // image bitmap
  G_io_seproxyhal_spi_buffer[0] = SEPROXYHAL_TAG_SCREEN_DISPLAY_STATUS;
  G_io_seproxyhal_spi_buffer[1] = length>>8;
  G_io_seproxyhal_spi_buffer[2] = length;
  io_seproxyhal_spi_send(G_io_seproxyhal_spi_buffer, 3);
  io_seproxyhal_spi_send((unsigned char*)&c, sizeof(bagl_component_t));
  G_io_seproxyhal_spi_buffer[0] = bit_per_pixel;
  io_seproxyhal_spi_send(G_io_seproxyhal_spi_buffer, 1);
  io_seproxyhal_spi_send((unsigned char*)color_index, h);
  io_seproxyhal_spi_send(bitmap, w);
  */
}


uint8_t display_default(const bagl_element_t * element) {
  // process automagically address from rom and from ram
  unsigned int type = (element->component.type & ~(BAGL_FLAG_TOUCHABLE));
  int size = 0;
  // avoid sending another status :), fixes a lot of bugs in the end
  /*if (io_seproxyhal_spi_is_status_sent()) {
    return;
  }*/

  if (type != BAGL_NONE) {
    if (element->text != NULL) {
      unsigned int text_adr = PIC((unsigned int)element->text);
      // consider an icon details descriptor is pointed by the context
      if (type == BAGL_ICON && element->component.icon_id == 0) {
        size += display_icon(&element->component, (bagl_icon_details_t*)text_adr);
      }
      else {
        unsigned short length = sizeof(bagl_component_t)+strlen((const char*)text_adr);
        /*G_io_seproxyhal_spi_buffer[0] = SEPROXYHAL_TAG_SCREEN_DISPLAY_STATUS;
        G_io_seproxyhal_spi_buffer[1] = length>>8;
        G_io_seproxyhal_spi_buffer[2] = length;
        io_seproxyhal_spi_send(G_io_seproxyhal_spi_buffer, 3);
        io_seproxyhal_spi_send((unsigned char*)&element->component, sizeof(bagl_component_t));
        io_seproxyhal_spi_send((unsigned char*)text_adr, length-sizeof(bagl_component_t));*/

        G_io_apdu_buffer[0] = SEPROXYHAL_TAG_SCREEN_DISPLAY_STATUS;
        G_io_apdu_buffer[1] = length>>8;
        G_io_apdu_buffer[2] = length;
        os_memcpy(&G_io_apdu_buffer[3], (unsigned char*)&element->component, sizeof(bagl_component_t));
        os_memcpy(&G_io_apdu_buffer[3+sizeof(bagl_component_t)], (unsigned char*)text_adr, length-sizeof(bagl_component_t));
        size += 3 + length;
      }
    }
    else {
      unsigned short length = sizeof(bagl_component_t);
      /*G_io_seproxyhal_spi_buffer[0] = SEPROXYHAL_TAG_SCREEN_DISPLAY_STATUS;
      G_io_seproxyhal_spi_buffer[1] = length>>8;
      G_io_seproxyhal_spi_buffer[2] = length;
      io_seproxyhal_spi_send(G_io_seproxyhal_spi_buffer, 3);
      io_seproxyhal_spi_send((unsigned char*)&element->component, sizeof(bagl_component_t));*/

      G_io_apdu_buffer[0] = SEPROXYHAL_TAG_SCREEN_DISPLAY_STATUS;
      G_io_apdu_buffer[1] = length>>8;
      G_io_apdu_buffer[2] = length;
      os_memcpy(&G_io_apdu_buffer[3],(unsigned char*)&element->component, sizeof(bagl_component_t));
      size += 3+length;
    }
  }
  return size;
}
#endif
