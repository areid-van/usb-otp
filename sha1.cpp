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
#include <avr/pgmspace.h>

#define CircularShift(bits,word) (((word) << (bits)) | ((word) >> (32-(bits))))

SHA1::SHA1()
{
    reset();
}

void SHA1::reset()
{
    mBitCount = 0;
    mBlockIndex = 0;
    mHash[0]   = 0x67452301;
    mHash[1]   = 0xEFCDAB89;
    mHash[2]   = 0x98BADCFE;
    mHash[3]   = 0x10325476;
    mHash[4]   = 0xC3D2E1F0;
}

void SHA1::digest(uint8_t hash[20])
{
    if (mBlockIndex > 55)
    {
        mBlock[mBlockIndex++] = 0x80;
        while(mBlockIndex < 64) mBlock[mBlockIndex++] = 0;
        processBlock();
        while(mBlockIndex < 56) mBlock[mBlockIndex++] = 0;
    }
    else
    {
        mBlock[mBlockIndex++] = 0x80;
        while(mBlockIndex < 56) mBlock[mBlockIndex++] = 0;
    }
    
    mBlock[56] = 0;
    mBlock[57] = 0;
    mBlock[58] = 0;
    mBlock[59] = 0;
    mBlock[60] = 0;
    mBlock[61] = 0;
    mBlock[62] = mBitCount >> 8;
    mBlock[63] = mBitCount;
    
    processBlock();
    
    for(uint8_t i = 0; i < 20; ++i)
    {
        hash[i] = mHash[i>>2] >> 8 * ( 3 - ( i & 0x03 ) );
    }
}

void SHA1::update(const uint8_t* m, uint8_t length)
{
    while(length--)
    {
        mBlock[mBlockIndex++] = *m;
        mBitCount += 8;
        if(mBlockIndex == 64) processBlock();
        m++;
    }
}

const PROGMEM uint32_t K[4] = { 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };

inline uint32_t extendBlock(uint32_t W[], uint8_t t)
{
    return CircularShift(1,W[(t-3)%16] ^ W[(t-8)%16] ^ W[(t-14)%16] ^ W[(t-16)%16]);
}

void SHA1::processBlock()
{
    uint32_t* W = (uint32_t*) mBlock; 
    //Re-order bytes to little endian 32 bit words
    for(uint8_t t = 0; t < 16; t++)
    {
        uint32_t Wt = ((uint32_t)mBlock[t * 4]) << 24;
        Wt |= ((uint32_t)mBlock[t * 4 + 1]) << 16;
        Wt |= ((uint32_t)mBlock[t * 4 + 2]) << 8;
        Wt |= ((uint32_t)mBlock[t * 4 + 3]);
        W[t] = Wt;
    }
    
    uint32_t A = mHash[0];
    uint32_t B = mHash[1];
    uint32_t C = mHash[2];
    uint32_t D = mHash[3];
    uint32_t E = mHash[4];
    
    for(uint8_t t = 0; t < 16; t++)
    {
        uint32_t temp =  CircularShift(5,A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        E = D;
        D = C;
        C = CircularShift(30,B);
        B = A;
        A = temp;
    }
    
    for(uint8_t t = 16; t < 20; t++)
    {
        uint32_t Wt = extendBlock(W, t);
        W[t%16] = Wt;
        uint32_t temp =  CircularShift(5,A) + ((B & C) | ((~B) & D)) + E + Wt + K[0];
        E = D;
        D = C;
        C = CircularShift(30,B);
        B = A;
        A = temp;
    }
    
    for(uint8_t t = 20; t < 40; t++)
    {
        uint32_t Wt = extendBlock(W, t);
        W[t%16] = Wt;
        uint32_t temp = CircularShift(5,A) + (B ^ C ^ D) + E + Wt + K[1];
        E = D;
        D = C;
        C = CircularShift(30,B);
        B = A;
        A = temp;
    }
    
    for(uint8_t t = 40; t < 60; t++)
    {
        uint32_t Wt = extendBlock(W, t);
        W[t%16] = Wt;
        uint32_t temp = CircularShift(5,A) +
        ((B & C) | (B & D) | (C & D)) + E + Wt + K[2];
        E = D;
        D = C;
        C = CircularShift(30,B);
        B = A;
        A = temp;
    }
    
    for(uint8_t t = 60; t < 80; t++)
    {
        uint32_t Wt = extendBlock(W, t);
        W[t%16] = Wt;
        uint32_t temp = CircularShift(5,A) + (B ^ C ^ D) + E + Wt + K[3];
        E = D;
        D = C;
        C = CircularShift(30,B);
        B = A;
        A = temp;
    }
    
    mHash[0] += A;
    mHash[1] += B;
    mHash[2] += C;
    mHash[3] += D;
    mHash[4] += E;
    
    mBlockIndex = 0;
}

