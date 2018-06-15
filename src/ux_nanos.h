#include "glyphs.h"
#include "os.h"
#include "bolos_ux_common.h"

#define NOISE_SEED_LEN   36

typedef struct {
	char noise_seed[36];
	char noise_seed_display[17];
	char string_buffer[20];
	uint8_t first_display;
	uint8_t typedDigitLen;
	uint8_t offset;
	uint8_t noise_seed_valid;
	uint8_t words_seed_valid;
}ux_revealer;

extern ux_revealer G_revealer;

void ui_idle_init(void);
void ui_idle_reinit(void);
void ui_type_noise_seed_nanos_init(void);
void revealer_struct_init(void);

