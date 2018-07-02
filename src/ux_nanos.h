#ifndef UX_NANOS_H
#define UX_NANOS_H

#include "glyphs.h"
#include "os.h"
#include "bolos_ux_common.h"



#define SEED_SIZE 200

extern char  text[SEED_SIZE];

void ui_idle_init(void);
void ui_idle_reinit(void);
void ui_type_noise_seed_nanos_init(void);
void revealer_struct_init(void);

#endif //UX_NANOS_H