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

#ifndef _HMAC_SHA1_H_
#define _HMAC_SHA1_H_

#include <stdint.h>
#include "sha1.h"

/*
An implementation of HMAC-SHA1 message digest algorithm that uses a minimal amount of memory.
Runs on AtTiny microcontrollers with 512 bytes of RAM. Might run with 256 bytes of RAM.

To save memory the object does not store the a copy of the key, so you must 
provide it twice!

Does not support keys longer than 64 bytes since that requires an extra SHA1 hash

*/

class HMAC_SHA1
{
    public:
    HMAC_SHA1(const uint8_t* key, uint8_t length);
    void reset(const uint8_t* key, uint8_t length);
    void update(const uint8_t* m, uint8_t length);
    void digest(const uint8_t* key, uint8_t length, uint8_t hash[20]);

    private:
    SHA1 mSHA1;
};

#endif
