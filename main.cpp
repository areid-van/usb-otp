/*

An implementation of the RFC 6238 time based OTP protocol on an AtTiny85 microcontroller

Copyright (C) 2017 Adam Reid

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/
#include "sha1.h"
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/pgmspace.h>


const PROGMEM uint8_t ipad = '\x36';
const PROGMEM uint8_t opad = '\x5c';

int main(void)
{
    SHA1 sha1;

    {
    uint8_t m[10];
    m[0] = 0x48;
    m[1] = 0x65;
    m[2] = 0x6c;
    m[3] = 0x6c;
    m[4] = 0x6f;
    m[5] = 0x21;
    m[6] = 0xde;
    m[7] = 0xad;
    m[8] = 0xbe;
    m[9] = 0xef;

    for(uint8_t i=0; i<10; i++) m[i] ^= ipad;
    sha1.update(m, 10);
    m[0] = ipad;
    for(uint8_t i=0; i<54; i++) sha1.update(m, 1);

    m[0] = 0;
    m[1] = 0;
    m[2] = 0;
    m[3] = 0;
    m[4] = 2;
    m[5] = 0xfd;
    m[6] = 0xfa;
    m[7] = 0xd3;
    sha1.update(m, 8);
    }

    uint8_t d[20];
    sha1.digest(d);
    sha1.reset();

    {
    uint8_t m[10];
    m[0] = 0x48;
    m[1] = 0x65;
    m[2] = 0x6c;
    m[3] = 0x6c;
    m[4] = 0x6f;
    m[5] = 0x21;
    m[6] = 0xde;
    m[7] = 0xad;
    m[8] = 0xbe;
    m[9] = 0xef;

    for(uint8_t i=0; i<10; i++) m[i] ^= opad;
    sha1.update(m, 10);
    m[0] = opad;
    for(uint8_t i=0; i<54; i++) sha1.update(m, 1);
    }

    sha1.update(d, 20);
    sha1.digest(d);

    uint8_t o = d[19] & 0x0f;
    
    uint32_t p = 0;
    for(uint8_t i=0; i<4; i++){
        uint32_t x = d[o+i];
        p |= x << (3-i)*8;
    } 
    p = p % 1000000ul;

    for(uint8_t i=0; i<6; i++){
        d[5-i] = (p%10ul) + 48ul;
        p /= 10ul;
    }
    eeprom_write_block((uint8_t*)d, (uint8_t*)0, 6);

    while(1) _delay_ms(1000);
    return 0;
}
