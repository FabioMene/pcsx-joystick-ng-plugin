/*
 * psemu-joystick-ng-plugin.c
 *
 * Copyright 2016-2017 Fabio Meneghetti <fabiomene97@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 *
 */

// Questo è un plugin per emulatori ps1 compatibili con PSEmu

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <fcntl.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>

#include <joystick-ng.h>

#include "config.h"
#include "psemu_plugin_defs.h"

// Riconoscimento plugin
uint32_t PSEgetLibType(void) {
    return PSE_LT_PAD;
}

uint32_t PSEgetLibVersion(void) {
    return 0x010000;
}

char* PSEgetLibName(void) {
    return "Joystick-ng";
}

long  PADquery(void){
    return PSE_PAD_USE_PORT1 | PSE_PAD_USE_PORT2;
}

// Il plugin vero e proprio
long PADinit(long flags){
    jngp_load_config();
    return PSE_PAD_ERR_SUCCESS;
}

long PADshutdown(void){
    return PSE_PAD_ERR_SUCCESS;
}

enum {
    PS_CMD_READ               = 0x42,
    PS_CMD_CONFIG_MODE        = 0x43,
    PS_CMD_SET_MAIN_MODE      = 0x44,
    PS_CMD_QUERY_MODEL        = 0x45,
    PS_CMD_QUERY_ACT          = 0x46,
    PS_CMD_QUERY_COMB         = 0x47,
    PS_CMD_QUERY_MODE         = 0x4c,
    PS_CMD_SET_ACT_ALIGN      = 0x4d
};

// Molto basato su ps2hwadapt.ino (adattatore pad hardware ps2, software arduino)

static unsigned char curr_pad;
static unsigned char curr_cmd;
static unsigned char bi; // byte index

static jng_event_t fb_event = {
    .type = JNG_EV_FB_FORCE
};

struct {
    volatile unsigned char mode;
    // Se true il joystick è in modalità configurazione
    int inconf;
    // I dati del joystick
    // 0-1  stato digitale (b0: dal meno significativo SEL, L3, R3, START, U, R, D, L; b1: L2, R2, L1, R1, T, C, X, Q)
    // 2-5  levette (RX, RY, LX, LY)
    volatile unsigned char data[6];
    // Flags
    unsigned char flags;
    #define PS_FLAG_LOCK 0x01
    // Indica se nel comando precedente qualcosa è stato cambiato
    unsigned char mode_changed;
    // L'ultimo stato del tasto analog
    int aklast;
} joystick_arr[2] = {
    {
        .inconf       = 0,
        .data         = { 0xff, 0xff, 0x7f, 0x7f, 0x7f, 0x7f },
        .flags        = 0x00,
        .mode_changed = 0,
        .aklast       = 0
    }, {
        .inconf       = 0,
        .data         = { 0xff, 0xff, 0x7f, 0x7f, 0x7f, 0x7f },
        .flags        = 0x00,
        .mode_changed = 0,
        .aklast       = 0
    }
};

#define joystick joystick_arr[curr_pad]

static inline void jngp_set_mode_led(int pad){
    jng_event_t event = {
        .type  = JNG_EV_FB_LED,
        .what  = jngp_pads[pad].mode_led,
        .value = (joystick_arr[pad].mode == PS_MODE_ANALOG)?(0xffffffff):(0x00)
    };
    if(event.what) write(jngp_pads[pad].fd, &event, sizeof(jng_event_t));
}

static int opened = 0;

void* jngp_update_thread(void*);
static pthread_t    update_thread;
static volatile int update_thread_run = 1;

static Display* xdisplay;
static Window   xwindow;
static Atom     xwmprotocols;
static Atom     xwmdelwindow; 
static long     xlastkey = 0;

long PADopen(unsigned long* display){
    if(opened) return PSE_PAD_ERR_SUCCESS;
    xdisplay = (Display*)*display;
    
    stderr = fopen("/dev/stderr", "w");
    // stdout = stderr;
    
    // Inizializzazione joystick
    
    jngp_pads[0].fd = open("/dev/jng/device", O_RDWR);
    if(jngp_pads[0].fd < 0) return PSE_PAD_ERR_FAILURE;
    ioctl(jngp_pads[0].fd, JNGIOCSETSLOT, jngp_pads[0].slot);
    ioctl(jngp_pads[0].fd, JNGIOCSETMODE, JNG_RMODE_NORMAL | JNG_WMODE_EVENT);
    joystick_arr[0].mode = jngp_pads[0].type;
    jngp_set_mode_led(0);
    
    jngp_pads[1].fd = open("/dev/jng/device", O_RDWR);
    if(jngp_pads[1].fd < 0){
        close(jngp_pads[0].fd);
        return PSE_PAD_ERR_FAILURE;
    }
    ioctl(jngp_pads[1].fd, JNGIOCSETSLOT, jngp_pads[1].slot);
    ioctl(jngp_pads[1].fd, JNGIOCSETMODE, JNG_RMODE_NORMAL | JNG_WMODE_EVENT);
    joystick_arr[1].mode = jngp_pads[1].type;
    jngp_set_mode_led(1);
    
    // Inizializzazione tastiera
    
    int rto;
    xwmprotocols = XInternAtom(xdisplay, "WM_PROTOCOLS", 0);
    xwmdelwindow = XInternAtom(xdisplay, "WM_DELETE_WINDOW", 0);
    XkbSetDetectableAutoRepeat(xdisplay, 1, NULL);
    XGetInputFocus(xdisplay, &xwindow, &rto);
    
    // Inizializzazione thread
    
    update_thread_run = 1;
    if(pthread_create(&update_thread, NULL, jngp_update_thread, NULL) < 0){
        fprintf(stderr, "joystick-ng: Errore avvio pthread\n");
        return PSE_PAD_ERR_FAILURE;
    }
    
    opened = 1;
    return 0;
}

long PADclose(void){
    if(opened == 0) return 0;
    opened = 0;
    update_thread_run = 0;
    pthread_join(update_thread, NULL);
    close(jngp_pads[0].fd);
    close(jngp_pads[1].fd);
	XkbSetDetectableAutoRepeat(xdisplay, 0, NULL);
    return 0;
}

unsigned char PADstartPoll(int pad){ // Linea ATT attivata, reset comando
	curr_pad = pad - 1;
	bi       = 0;
	return 0xff;
}

unsigned char PADpoll(unsigned char value){
    static unsigned char cmdb, tmp; // command byte, temp
    static unsigned char ivi, ovi; // output vector index, input vector index. I vector index tolgono l'offset dell'header in ingresso (3) e in uscita (2)
    static unsigned char cb = 0; // Command Bytes (byte totali del comando)
    static unsigned char send;
    
    ivi = bi - 2;
    ovi = bi - 2;
    
    if(bi == 0){ // La ps invia il comando come secondo byte. Qui si decide anche la lunghezza del comando
        curr_cmd = value;
        if     (joystick.inconf)                  cb = 6;
        else if(joystick.mode == PS_MODE_DIGITAL) cb = 2;
        else if(joystick.mode == PS_MODE_ANALOG)  cb = 6;
        else                                      cb = 0; // boh
    } else cmdb = value;

    // Nessuno dei comandi ha qualche dato nell'ultimo byte, quindi se abbiamo raggiunto la fine return;
    // In teoria non è necessario
    // if(bi > 1 && ivi >= cb - 1) return 0x00;

    if(bi >= 2) switch(curr_cmd){
        case PS_CMD_READ:
            // Nessun controllo, in mod digitale potrebbe inviare anche falsi dati di joystick (in teoria la ps non li considera)
            send = joystick.data[ovi];
            if(ivi == 1){
                fb_event.what  = JNG_FB_FORCE_SMALLMOTOR;
                fb_event.value = (cmdb)?65535:0;
                write(jngp_pads[curr_pad].fd, &fb_event, sizeof(jng_event_t));
            } else if(ivi == 0){
                fb_event.what  = JNG_FB_FORCE_BIGMOTOR;
                fb_event.value = cmdb << 8;
                write(jngp_pads[curr_pad].fd, &fb_event, sizeof(jng_event_t));
            }
            break;
        
        case PS_CMD_CONFIG_MODE: 
            // I dati vanno inviati solo se non si è già in mod config
            if(joystick.inconf == 0) send = joystick.data[ovi];
            else                     send = 0x00;
            // Vediamo se entrare in mod config. I cambiamenti vengono fatti dopo byte inviato
            if     (ivi == 0)      tmp = cmdb;
            else if(ovi == cb - 2) joystick.inconf = (tmp)?1:0;
            break;

        case PS_CMD_QUERY_MODEL:
            if(!joystick.inconf){
                cb = 6; // A volte i driver si dimenticano di mettere in config
                //return; // solo mod config
            }
            if     (ovi == 0) send = 0x01; // model: DualShock
            else if(ovi == 1) send = 0x02; // numModes: 2 (digitale e dualshock)
            else if(ovi == 2) send = ((joystick.mode == PS_MODE_ANALOG)?0x01:0x00); // modeCurOffs
            else if(ovi == 3) send = 0x02; // numActuators: 2
            else if(ovi == 4) send = 0x01; // numActComb: 1
            else              send = 0x00; // non utilizzato
            break;

        case PS_CMD_QUERY_ACT:
            if(!joystick.inconf){
                cb = 6;
                //return; // solo mod config
            }
            if(ivi == 0) tmp = cmdb;
            if     (ovi == 3) send = (tmp)?0x00:0x02; // boh
            else if(ovi == 4) send = (tmp)?0x01:0x00; // boh
            else if(ovi == 5) send = (tmp)?0x14:0x0a; // boh
            else              send = 0x00;
            break;

        case PS_CMD_QUERY_COMB:
            if(!joystick.inconf){
                cb = 6;
                //return; // solo mod config
            }
            if(ovi == 2) send = 0x02; // boh
            else         send = 0x00;
            break;

        case PS_CMD_QUERY_MODE:
            if(!joystick.inconf){
                cb = 6;
                //return; // solo mod config
            }
            // In PS_CMD_QUERY_MODEL abbiamo detto che supportiamo 2 configurazioni, ovvero digitale e analogico
            if(ivi == 0) tmp = cmdb;
            if(ovi == 3) send = (tmp)?PS_MODE_ANALOG:PS_MODE_DIGITAL;
            else         send = 0x00;
            break;

        case PS_CMD_SET_MAIN_MODE:
            if(!joystick.inconf){
                cb = 6;
                //return; // solo mod config
            }
            if(ivi == 0){
                tmp = (cmdb == 0x01)?PS_MODE_ANALOG:PS_MODE_DIGITAL;
                if(tmp != joystick.mode) joystick.mode_changed = 1;
                joystick.mode = tmp;
            } else if(ivi == 1){
                tmp = joystick.flags;
                if(cmdb == 0x03) tmp |=  PS_FLAG_LOCK;
                else             tmp &= ~PS_FLAG_LOCK;
                if(tmp != joystick.flags) joystick.mode_changed = 1;
                joystick.flags = tmp;
            }
            send = 0x00;
            break;

        case PS_CMD_SET_ACT_ALIGN:
            if(!joystick.inconf){
                cb = 6;
                //return; // solo mod config
            }
            if     (ovi == 0) send = 0x00;
            else if(ovi == 1) send = 0x01;
            else              send = 0xff;
            break;

        // boh
        default:
            send = 0x00;
    } else if(bi == 0) { // Dobbiamo spedire la configurazione del joystick
        if     (joystick.inconf){
            send = 0xf3; // 0x0f (mod config), 6 byte
        } else if(joystick.mode == PS_MODE_DIGITAL){
            send = 0x41; // 0x04 (digitale),   2 byte
        } else if(joystick.mode == PS_MODE_ANALOG){
            send = 0x53; // 0x05 (analogico), 6 byte
        }
    } else { // bi = 1
        if(joystick.mode_changed) send = 0x00;
        else                      send = 0x5a;
        joystick.mode_changed = 0;
    }
    bi++;
    return send;
}

static inline void jngp_pad_read(int pn, PadDataS* pad){
    pad->controllerType = PSE_PAD_TYPE_ANALOGPAD;
    pad->buttonStatus   = (joystick_arr[pn].data[0] << 8) | joystick_arr[pn].data[1];
    pad->rightJoyX      = joystick_arr[pn].data[2];
    pad->rightJoyY      = joystick_arr[pn].data[3];
    pad->leftJoyX       = joystick_arr[pn].data[4];
    pad->leftJoyY       = joystick_arr[pn].data[5];
}

long PADreadPort1(PadDataS *pad){
    jngp_pad_read(0, pad);
	return 0;
}

long PADreadPort2(PadDataS *pad){
    jngp_pad_read(1, pad);
	return 0;
}

long PADkeypressed(void){
    static int frame = 1;
    
    // Aggiornamento X
    
    XEvent event;
    XClientMessageEvent* cmevent;
    if(frame) while(XPending(xdisplay)){ // Controlla gli eventi X11 solo ogni due frame
        XNextEvent(xdisplay, &event);
        switch(event.type){
            case KeyPress:
                xlastkey = XLookupKeysym((XKeyEvent*)&event, 0);
                break;
            case KeyRelease:
                xlastkey = XLookupKeysym((XKeyEvent*)&event, 0) | 0x40000000;
                break;
            case ClientMessage:
                cmevent = (XClientMessageEvent*)&event;
                if(cmevent->message_type == xwmprotocols && (Atom)cmevent->data.l[0] == xwmdelwindow){
                    xlastkey = XK_Escape;
                }
                break;
        }
    }
    frame ^= 1;
    
    long key = xlastkey;
    xlastkey = 0;
	return key;
}

void* jngp_update_thread(void* unused){
    int i;
    jng_state_t state;
    while(update_thread_run){
        // Aggiornamento jng
        
        for(i = 0;i < 2;i++){
            read(jngp_pads[i].fd, &state, sizeof(jng_state_t));
            
            int akstate = state.keys & jngp_pads[i].keys[PS_KEY_ANALOG];
            
            if(akstate == 0 && joystick_arr[i].aklast != 0){
                if(joystick_arr[i].mode == PS_MODE_DIGITAL) joystick_arr[i].mode = PS_MODE_ANALOG;
                else                                        joystick_arr[i].mode = PS_MODE_DIGITAL;
                
                jngp_set_mode_led(i);
            }
            
            joystick_arr[i].aklast = akstate;
            
            // Genera i dati del joystick
            // (b0: dal meno significativo SEL, L3, R3, START, U, R, D, L; b1: L2, R2, L1, R1, T, C, X, Q)
            #define jngp_js_k_bit(psbit, off) ((state.keys & jngp_pads[i].keys[(psbit)])?(0):(1 << (off)))
            joystick_arr[i].data[0] = jngp_js_k_bit(PS_KEY_SELECT,   0)
                                    | jngp_js_k_bit(PS_KEY_L3,       1)
                                    | jngp_js_k_bit(PS_KEY_R3,       2)
                                    | jngp_js_k_bit(PS_KEY_START,    3)
                                    | jngp_js_k_bit(PS_KEY_UP,       4)
                                    | jngp_js_k_bit(PS_KEY_RIGHT,    5)
                                    | jngp_js_k_bit(PS_KEY_DOWN,     6)
                                    | jngp_js_k_bit(PS_KEY_LEFT,     7);
            if(joystick_arr[i].mode == PS_MODE_ANALOG){
                joystick_arr[i].data[1] = jngp_js_k_bit(PS_KEY_L2,       0)
                                        | jngp_js_k_bit(PS_KEY_R2,       1)
                                        | jngp_js_k_bit(PS_KEY_SQUARE,   2)
                                        | jngp_js_k_bit(PS_KEY_TRIANGLE, 3)
                                        | jngp_js_k_bit(PS_KEY_R1,       4)
                                        | jngp_js_k_bit(PS_KEY_CIRCLE,   5)
                                        | jngp_js_k_bit(PS_KEY_CROSS,    6)
                                        | jngp_js_k_bit(PS_KEY_L1,       7);
            } else {
                joystick_arr[i].data[1] = jngp_js_k_bit(PS_KEY_L2,       0)
                                        | jngp_js_k_bit(PS_KEY_R2,       1)
                                        | jngp_js_k_bit(PS_KEY_L1,       2)
                                        | jngp_js_k_bit(PS_KEY_R1,       3)
                                        | jngp_js_k_bit(PS_KEY_TRIANGLE, 4)
                                        | jngp_js_k_bit(PS_KEY_CIRCLE,   5)
                                        | jngp_js_k_bit(PS_KEY_CROSS,    6)
                                        | jngp_js_k_bit(PS_KEY_SQUARE,   7);
            }
            #undef jngp_js_k_bit
            // levette (RX, RY, LX, LY)
            #define jngp_js_a_byte(psbit, boff) joystick_arr[i].data[2 + (boff)] = (JNG_AXIS(state, jngp_pads[i].axis[(psbit)]) >> 8) - 128;
            jngp_js_a_byte(PS_AXIS_RX, 0);
            jngp_js_a_byte(PS_AXIS_RY, 1);
            jngp_js_a_byte(PS_AXIS_LX, 2);
            jngp_js_a_byte(PS_AXIS_LY, 3);
            #undef jngp_js_a_byte
        }
        usleep(5000);
    }
    return NULL;
}

void PADregisterVibration(void (*callback)(uint32_t, uint32_t)){}


long  PADconfigure(void){
    // launch config
    return 0;
}

void  PADabout(void){
    // launch about
}

long  PADtest(void){
    return PSE_PAD_ERR_SUCCESS;
}






