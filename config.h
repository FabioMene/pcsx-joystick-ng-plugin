/*
 * config.h
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

#ifndef JNGP_CONFIG_H
#define JNGP_CONFIG_H 1

typedef struct {
    unsigned int slot; // slot joystick-ng
    
    unsigned int type; // pad analogico (1) / digitale (0) (non la modalità ma il tipo)
    unsigned int keys[17]; // I vari tasti. i valori sono JNG_KEY_*
    unsigned int axis[4]; // JNG_AXIS_*
    
    unsigned int mode_led; // Il led JNG_FB_LED_* per indicare se è attiva la modalità analogica (acceso) o digitale (spento). 0 per disattivare
    
    int fd;
} jngp_pad_t;

extern jngp_pad_t jngp_pads[2];

enum {
	PS_KEY_SELECT = 0,
	PS_KEY_L3,
	PS_KEY_R3,
	PS_KEY_START,
	PS_KEY_UP,
	PS_KEY_RIGHT,
	PS_KEY_DOWN,
	PS_KEY_LEFT,
	PS_KEY_L2,
	PS_KEY_R2,
	PS_KEY_L1,
	PS_KEY_R1,
	PS_KEY_TRIANGLE,
	PS_KEY_CIRCLE,
	PS_KEY_CROSS,
	PS_KEY_SQUARE,
	PS_KEY_ANALOG
};

enum {
    PS_AXIS_RX = 0,
    PS_AXIS_RY,
    PS_AXIS_LX,
    PS_AXIS_LY
};

#define PS_MODE_DIGITAL 0x04
#define PS_MODE_ANALOG  0x07

#define CONFIG_FILE "jngp.conf"

// Carica CONFIG_FILE o la configurazione di default
extern void jngp_load_config();

// Salva configurazione su CONFIG_FILE
extern int jngp_save_config();


#endif

