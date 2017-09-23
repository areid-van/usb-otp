/*
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
#include <avr/pgmspace.h>


const PROGMEM uint8_t ipad = '\x36';
const PROGMEM uint8_t opad = '\x5c';

HMAC_SHA1::HMAC_SHA1(const uint8_t* key, uint8_t length)
{
    reset(key, length);
}

void HMAC_SHA1::reset(const uint8_t* key, uint8_t length)
{
    for(uint8_t i=0; i<length; i++)
    {
        uint8_t ki = key[i] ^ ipad;
        mSHA1.update(&ki, 1);
    }
    uint8_t ki = ipad;
    for(uint8_t i=0; i<64-length; i++) mSHA1.update(&ki, 1);
}

void HMAC_SHA1::update(const uint8_t* m, uint8_t length)
{
    mSHA1.update(m, length);
}

void HMAC_SHA1::digest(const uint8_t* key, uint8_t length, uint8_t hash[20])
{
    mSHA1.digest(hash);
    mSHA1.reset();

    for(uint8_t i=0; i<length; i++)
    {
        uint8_t ki = key[i] ^ opad;
        mSHA1.update(&ki, 1);
    }
    uint8_t ki = opad;
    for(uint8_t i=0; i<64-length; i++) mSHA1.update(&ki, 1);

    mSHA1.update(hash, 20);
    mSHA1.digest(hash);
}

