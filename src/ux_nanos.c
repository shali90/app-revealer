#include "ux_nanos.h"

enum UI_STATE { UI_IDLE, UI_TEXT, UI_APPROVAL };

enum UI_STATE uiState;

//TODO move in common .h
#define DENIED_BY_USER 0x6985
#define SW_OK 0x9000

extern const ux_menu_entry_t menu_about_nanos[];
extern const ux_menu_entry_t ui_idle_mainmenu_nanos[];
extern const ux_menu_entry_t ui_type_seed_words_menu_nanos[];
extern const bagl_element_t ui_confirm_seed_display_nanos[];

const ux_menu_entry_t menu_about_nanos[] = {
    {NULL, NULL, 0, NULL, "Version", APPVERSION, 0, 0},
    {ui_idle_mainmenu_nanos, NULL, 1, &C_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END}; 

const ux_menu_entry_t ui_idle_mainmenu_nanos[] = {
  {NULL, /*screen_onboarding_3_restore_init*/NULL, 0, /*&icon_hack*/NULL, "Waiting for", "noise seed", 32, 10},
  //{/*menu_settings_nanos*/NULL, NULL, 0, NULL, "Settings", NULL, 0, 0},
  {menu_about_nanos, NULL, 0, NULL, "About", NULL, 0, 0},
  {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
  UX_MENU_END
};

const ux_menu_entry_t ui_type_seed_words_menu_nanos[] = {
  {NULL, screen_onboarding_3_restore_init, 0, /*&icon_hack*/NULL, "Type your", "seed words", 32, 10},
  //{/*menu_settings_nanos*/NULL, NULL, 0, NULL, "Settings", NULL, 0, 0},
  {ui_idle_mainmenu_nanos, NULL, 0, &C_icon_dashboard, "Abort", NULL, 50, 29},
  UX_MENU_END
}; 

const bagl_element_t ui_confirm_seed_display_nanos[] = {
    // type                               userid    x    y   w    h  str rad
    // fill      fg        bg      fid iid  txt   touchparams...       ]
    {{BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF,
      0, 0},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_LABELINE, 0x01, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER , 0},
     "Confirm seed",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x01, 1, 26, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER , 0},
     "display",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
      BAGL_GLYPH_ICON_CROSS},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
      BAGL_GLYPH_ICON_CHECK},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

};

void seed_display_cancel(void){
  G_io_apdu_buffer[0] = DENIED_BY_USER >> 8;
  G_io_apdu_buffer[1] = DENIED_BY_USER & 0xff;
  io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
  ui_idle_init();
}

void seed_display_confirm(void){
  int replysize = 250;
  int offset = 0;
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
  memcpy(G_io_apdu_buffer, &G_bolos_ux_context.string_buffer[offset], replysize);
  G_io_apdu_buffer[replysize] = SW_OK >> 8;
  G_io_apdu_buffer[replysize+1] = SW_OK & 0xff;
  io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, replysize+2);
  ui_idle_init();
}

unsigned int ui_confirm_seed_display_nanos_button(unsigned int button_mask,unsigned int button_mask_counter) {
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_RIGHT:       
        //confirm
        seed_display_confirm();
        break;

    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        // deny
        seed_display_cancel();
        break;

    default:
        break;
    }
}



void ui_idle_init(void) {
  uiState = UI_IDLE;

  UX_MENU_DISPLAY(0, ui_idle_mainmenu_nanos, NULL);
  // setup the first screen changing
  UX_CALLBACK_SET_INTERVAL(1000);
}

void ui_type_seed_words_init(void){
  uiState = UI_TEXT;

  UX_MENU_DISPLAY(0, ui_type_seed_words_menu_nanos, NULL);
  // setup the first screen changing
  UX_CALLBACK_SET_INTERVAL(1000);
}

void ui_confirm_seed_display_init(void){
  UX_DISPLAY(ui_confirm_seed_display_nanos,NULL);
}

