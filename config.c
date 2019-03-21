/*
 * config.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <joystick-ng.h>

#include "config.h"
#include "psemu_plugin_defs.h"

jngp_pad_t jngp_pads[2];

// File di configurazione = solo numeri
// slot1 mode1 keys1 axis1
// slot2 mode2 keys2 axis2

void jngp_load_config(){
    fflush(stdout);
    
    FILE* fp = fopen(CONFIG_FILE, "r");
    if(fp){
        // Pad1
        if(fscanf(fp, "%u %u", &jngp_pads[0].slot, &jngp_pads[0].type) != 2) goto jngp_ld_conf_fallback;
        
        #define k(n) &jngp_pads[0].keys[n]
        if(fscanf(fp, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u", k(0), k(1), k(2), k(3), k(4), k(5), k(6), k(7), k(8), k(9), k(10), k(11), k(12), k(13), k(14), k(15), k(16)) != 17) goto jngp_ld_conf_fallback;
        #undef k
        
        #define a(n) &jngp_pads[0].axis[n]
        if(fscanf(fp, "%u %u %u %u", a(0), a(1), a(2), a(3)) != 4) goto jngp_ld_conf_fallback;
        #undef a
        
        if(fscanf(fp, "%u", &jngp_pads[0].mode_led) != 1) goto jngp_ld_conf_fallback;
        // Pad2
        if(fscanf(fp, "%u %u", &jngp_pads[1].slot, &jngp_pads[1].type) != 2) goto jngp_ld_conf_fallback;
        
        #define k(n) &jngp_pads[1].keys[n]
        if(fscanf(fp, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u", k(0), k(1), k(2), k(3), k(4), k(5), k(6), k(7), k(8), k(9), k(10), k(11), k(12), k(13), k(14), k(15), k(16)) != 17) goto jngp_ld_conf_fallback;
        #undef k
        
        #define a(n) &jngp_pads[1].axis[n]
        if(fscanf(fp, "%u %u %u %u", a(0), a(1), a(2), a(3)) != 4) goto jngp_ld_conf_fallback;
        #undef a
        
        if(fscanf(fp, "%u", &jngp_pads[1].mode_led) != 1) goto jngp_ld_conf_fallback;
        
        fclose(fp);
        return;
    }
  jngp_ld_conf_fallback:
    if(fp) fclose(fp);
    // Pad1
    jngp_pads[0].slot = 0;
    jngp_pads[0].type = PS_MODE_ANALOG;
    
    jngp_pads[0].keys[PS_KEY_SELECT]   = JNG_KEY_SELECT;
    jngp_pads[0].keys[PS_KEY_L3]       = JNG_KEY_L3;
    jngp_pads[0].keys[PS_KEY_R3]       = JNG_KEY_R3;
    jngp_pads[0].keys[PS_KEY_START]    = JNG_KEY_START;
    jngp_pads[0].keys[PS_KEY_UP]       = JNG_KEY_UP;
    jngp_pads[0].keys[PS_KEY_RIGHT]    = JNG_KEY_RIGHT;
    jngp_pads[0].keys[PS_KEY_DOWN]     = JNG_KEY_DOWN;
    jngp_pads[0].keys[PS_KEY_LEFT]     = JNG_KEY_LEFT;
    jngp_pads[0].keys[PS_KEY_L2]       = JNG_KEY_L2;
    jngp_pads[0].keys[PS_KEY_R2]       = JNG_KEY_R2;
    jngp_pads[0].keys[PS_KEY_L1]       = JNG_KEY_L1;
    jngp_pads[0].keys[PS_KEY_R1]       = JNG_KEY_R1;
    jngp_pads[0].keys[PS_KEY_TRIANGLE] = JNG_KEY_Y;
    jngp_pads[0].keys[PS_KEY_CIRCLE]   = JNG_KEY_B;
    jngp_pads[0].keys[PS_KEY_CROSS]    = JNG_KEY_A;
    jngp_pads[0].keys[PS_KEY_SQUARE]   = JNG_KEY_X;
    jngp_pads[0].keys[PS_KEY_ANALOG]   = JNG_KEY_OPTIONS1;
    
    jngp_pads[0].axis[PS_AXIS_RX] = JNG_AXIS_RX;
    jngp_pads[0].axis[PS_AXIS_RY] = JNG_AXIS_RY;
    jngp_pads[0].axis[PS_AXIS_LX] = JNG_AXIS_LX;
    jngp_pads[0].axis[PS_AXIS_LY] = JNG_AXIS_LY;
    
    
    jngp_pads[0].mode_led = JNG_FB_LED_4;
    
    // Pad2
    jngp_pads[1].slot = 1;
    jngp_pads[1].type = PS_MODE_ANALOG;
    
    jngp_pads[1].keys[PS_KEY_SELECT]   = JNG_KEY_SELECT;
    jngp_pads[1].keys[PS_KEY_L3]       = JNG_KEY_L3;
    jngp_pads[1].keys[PS_KEY_R3]       = JNG_KEY_R3;
    jngp_pads[1].keys[PS_KEY_START]    = JNG_KEY_START;
    jngp_pads[1].keys[PS_KEY_UP]       = JNG_KEY_UP;
    jngp_pads[1].keys[PS_KEY_RIGHT]    = JNG_KEY_RIGHT;
    jngp_pads[1].keys[PS_KEY_DOWN]     = JNG_KEY_DOWN;
    jngp_pads[1].keys[PS_KEY_LEFT]     = JNG_KEY_LEFT;
    jngp_pads[1].keys[PS_KEY_L2]       = JNG_KEY_L2;
    jngp_pads[1].keys[PS_KEY_R2]       = JNG_KEY_R2;
    jngp_pads[1].keys[PS_KEY_L1]       = JNG_KEY_L1;
    jngp_pads[1].keys[PS_KEY_R1]       = JNG_KEY_R1;
    jngp_pads[1].keys[PS_KEY_TRIANGLE] = JNG_KEY_Y;
    jngp_pads[1].keys[PS_KEY_CIRCLE]   = JNG_KEY_B;
    jngp_pads[1].keys[PS_KEY_CROSS]    = JNG_KEY_A;
    jngp_pads[1].keys[PS_KEY_SQUARE]   = JNG_KEY_X;
    jngp_pads[1].keys[PS_KEY_ANALOG]   = JNG_KEY_OPTIONS1;
    
    jngp_pads[1].axis[PS_AXIS_RX] = JNG_AXIS_RX;
    jngp_pads[1].axis[PS_AXIS_RY] = JNG_AXIS_RY;
    jngp_pads[1].axis[PS_AXIS_LX] = JNG_AXIS_LX;
    jngp_pads[1].axis[PS_AXIS_LY] = JNG_AXIS_LY;
    
    jngp_pads[1].mode_led = JNG_FB_LED_4;
}

int jngp_save_config(){
    FILE* fp = fopen(CONFIG_FILE, "w");
    if(!fp) return -1;
    
    int ret = -1;
    
    // Pad1
    if(fprintf(fp, "%u %u\n", jngp_pads[0].slot, jngp_pads[0].type) < 0) goto jngp_save_fail;
    
    #define k(n) jngp_pads[0].keys[n]
    if(fprintf(fp, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n", k(0), k(1), k(2), k(3), k(4), k(5), k(6), k(7), k(8), k(9), k(10), k(11), k(12),  k(13), k(14), k(15), k(16)) < 0) goto jngp_save_fail;
    #undef k
    
    #define a(n) jngp_pads[0].axis[n]
    if(fprintf(fp, "%u %u %u %u\n\n", a(0), a(1), a(2), a(3)) < 0) goto jngp_save_fail;
    #undef a
    
    if(fprintf(fp, "%u\n", jngp_pads[0].mode_led) < 0) goto jngp_save_fail;
    
    // Pad2
    if(fprintf(fp, "%u %u\n", jngp_pads[1].slot, jngp_pads[1].type) < 0) goto jngp_save_fail;
    
    #define k(n) jngp_pads[1].keys[n]
    if(fprintf(fp, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n", k(0), k(1), k(2), k(3), k(4), k(5), k(6), k(7), k(8), k(9), k(10), k(11), k(12),  k(13), k(14), k(15), k(16)) < 0) goto jngp_save_fail;
    #undef k
    
    #define a(n) jngp_pads[1].axis[n]
    if(fprintf(fp, "%u %u %u %u\n", a(0), a(1), a(2), a(3)) < 0) goto jngp_save_fail;
    #undef a
    
    if(fprintf(fp, "%u\n", jngp_pads[1].mode_led) < 0) goto jngp_save_fail;
    
    ret = 0;
  jngp_save_fail:
    fclose(fp);
    return ret;
    
}

