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
#include "hmac_sha1.h"
#include <avr/eeprom.h>
#include <util/delay.h>


void otp(uint8_t password[6], uint8_t secret[], uint8_t length, uint8_t time[8])
{
    HMAC_SHA1 hmac(secret, length);
    hmac.update(time, 8);

    uint8_t digest[20];
    hmac.digest(secret, length, digest);

    uint8_t o = digest[19] & 0x0f;
    
    uint32_t p = 0;
    for(uint8_t i=0; i<4; i++){
        uint32_t x = digest[o+i];
        p |= x << (3-i)*8;
    } 
    p = p % 1000000ul;

    for(uint8_t i=0; i<6; i++){
        password[5-i] = (p%10ul) + 48ul;
        p /= 10ul;
    }
}

void getPassword(void)
{

    uint8_t secret[10];
    eeprom_read_block(secret, (uint8_t*)0, 10);

    uint8_t time[8];
    eeprom_read_block(time, (uint8_t*)0 + 16, 8);

    uint8_t password[6];
    otp(password, secret, 10, time);
    eeprom_write_block(password, (uint8_t*)0 + 32, 6);
}

int main(void)
{
    getPassword();
    while(1) _delay_ms(1000);
    return 0;
}
