/*******************************************************************************
*   Ledger Blue
*   (c) 2016 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include "os.h"
#include "cx.h"
#include "bolos_ux_common.h"

#include "os_io_seproxyhal.h"

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];
extern unsigned char seed_buffer[300];

//bolos_ux_context_t G_bolos_ux_context;

static unsigned int current_text_pos; // parsing cursor in the text to display
static unsigned int text_y;           // current location of the displayed text

// UI currently displayed
extern enum UI_STATE { UI_IDLE, UI_TEXT, UI_APPROVAL };

extern enum UI_STATE uiState;

ux_state_t ux;

static unsigned char display_text_part(void);

#define MAX_CHARS_PER_LINE 49
#define SW_OK 0x9000
#define REVEALER_CODE_LEN   18

static const uint8_t NoiseSeedReference[]={0x0E,0xCA,0x2C,0x8C,0xFA,0x19,0xBE,0x8A,0x64,0xA7,0xD7,0x67,0x72,0x25,0x3A,0xC0,0x72,0x78};


static char lineBuffer[50];

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
    case CHANNEL_KEYBOARD:
        break;

    // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
    case CHANNEL_SPI:
        if (tx_len) {
            io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

            if (channel & IO_RESET_AFTER_REPLIED) {
                reset();
            }
            return 0; // nothing received from the master so far (it's a tx
                      // transaction)
        } else {
            return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                                          sizeof(G_io_apdu_buffer), 0);
        }

    default:
        THROW(INVALID_PARAMETER);
    }
    return 0;
}

static void sample_main(void) {

    // next timer callback in 500 ms
    //UX_CALLBACK_SET_INTERVAL(500);
    volatile unsigned int rx = 0;
    volatile unsigned int tx = 0;
    volatile uint8_t NoiseSeed[REVEALER_CODE_LEN];
    uint8_t flags;

    for (;;) {
    volatile unsigned short sw = 0;

    BEGIN_TRY {
      TRY {
        rx = tx;
        tx = 0; // ensure no race in catch_other if io_exchange throws an error
        rx = io_exchange(CHANNEL_APDU|flags, rx);
        flags = 0;

        // no apdu received, well, reset the session, and reset the bootloader configuration
        if (rx == 0) {
          THROW(0x6982);
        }

        if (G_io_apdu_buffer[0] != 0x80) {
          THROW(0x6E00);
        }

        switch (G_io_apdu_buffer[1]) {
          case 0xCA: //Recieve revealer code
            memcpy(NoiseSeed,&G_io_apdu_buffer[5],REVEALER_CODE_LEN);
            uint8_t idx;
            for (idx=0; idx<REVEALER_CODE_LEN; idx++){
                if (NoiseSeed[idx] != NoiseSeedReference[idx]){
                    THROW(0x6dFF);
                }
            }
            memset(G_io_apdu_buffer, 0x00, sizeof(G_io_apdu_buffer));
            flags |= IO_ASYNCH_REPLY;
            ui_type_seed_words_init();            
            break;
          default:
            THROW(0x6D00);
            break;
        }
      }
      CATCH_OTHER(e) {
        switch(e & 0xF000) {
          case 0x6000:            
          case SW_OK:
            sw = e;
            break;
          default:
            sw = 0x6800 | (e&0x7FF);
            break;
        }
        // Unexpected exception => report 
        G_io_apdu_buffer[tx] = sw>>8;
        G_io_apdu_buffer[tx+1] = sw;
        tx += 2;
      }
      FINALLY {
        
      }
    }
    END_TRY;
  }

return_to_dashboard:
    return;
}


// Pick the text elements to display
static unsigned char display_text_part() {
    unsigned int i;
    WIDE char *text = (char*) G_io_apdu_buffer + 5;
    if (text[current_text_pos] == '\0') {
        return 0;
    }
    i = 0;
    while ((text[current_text_pos] != 0) && (text[current_text_pos] != '\n') &&
           (i < MAX_CHARS_PER_LINE)) {
        lineBuffer[i++] = text[current_text_pos];
        current_text_pos++;
    }
    if (text[current_text_pos] == '\n') {
        current_text_pos++;
    }
    lineBuffer[i] = '\0';

    return 1;
}

#include "bolos_ux.h"

unsigned char io_event(unsigned char channel) {
    // nothing done with the event, throw an error on the transport layer if
    // needed

    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
    case SEPROXYHAL_TAG_FINGER_EVENT:
        UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
        break;

    case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT: // for Nano S
        UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
        break;

    case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
        if ((uiState == UI_TEXT) &&
            (os_seph_features() & SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_SCREEN_BIG)) {
                UX_REDISPLAY();
            }
        else {
            if(G_bolos_ux_context.processing == 1)
            {
                UX_DISPLAYED_EVENT(foo(););
            }
            else
            {
                UX_DISPLAYED_EVENT();
            }
        }
        break;

    case SEPROXYHAL_TAG_TICKER_EVENT:
        #ifdef TARGET_NANOS
            UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
                // defaulty retrig very soon (will be overriden during
                // stepper_prepro)
                UX_CALLBACK_SET_INTERVAL(500);
                UX_REDISPLAY();
            });
        #endif 
        break;

    // unknown events are acknowledged
    default:
        UX_DEFAULT_EVENT();
        break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    // command has been processed, DO NOT reset the current APDU transport
    return 1;
}

__attribute__((section(".boot"))) int main(void) {
    // exit critical section
    __asm volatile("cpsie i");

    current_text_pos = 0;
    text_y = 60;
    uiState = UI_IDLE;

    // ensure exception will work as planned
    os_boot();

    UX_INIT();

    BEGIN_TRY {
        TRY {
            io_seproxyhal_init();

            
#ifdef LISTEN_BLE
            if (os_seph_features() &
                SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_BLE) {
                BLE_power(0, NULL);
                // restart IOs
                BLE_power(1, NULL);
            }
#endif

            USB_power(0);
            USB_power(1);

            //ui_idle();
            ui_idle_init();
            sample_main();
        }
        CATCH_OTHER(e) {
        }
        FINALLY {
        }
    }
    END_TRY;
}
