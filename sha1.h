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

#ifndef _SHA1_H_
#define _SHA1_H_

#include <stdint.h>

/*
An implementation of the SHA1 hash algorithm that uses a minimal amount of memory.
Runs on AtTiny microcontrollers with 512 bytes of RAM. Might run with 256 bytes of RAM.

As this code is intended for an MCU it only supports adding 255 message bytes at a time
and a maximum message length of 2^16 bits


Usage: Follows the Python hashlib interface

SHA1 sha1;
uint8_t message[10] = {...};
sha1.update(message, 10); //can call message multiple times
uint8_t digest[20];
sha1.digest(digest);
//call SHA1::reset() if you want to start a new hash

*/

class SHA1
{
    public:
    SHA1();
    void reset();
    void update(const uint8_t* m, uint8_t length);
    void digest(uint8_t hash[20]);

    private:
    uint32_t mHash[5]; 
    uint16_t mBitCount; 
    uint8_t mBlockIndex;
    uint8_t mBlock[64];

    void processBlock();
};

#endif
