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
#include <avr/wdt.h>
#include <avr/interrupt.h>

extern "C" {
    #include "usbdrv/usbdrv.h"
}



static uint8_t report_out [3];

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

PROGMEM const char usbHidReportDescriptor [USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
    '\x05', '\x01',     //USAGE_PAGE (Generic Desktop)
    '\x09', '\x05',     //USAGE (Game Pad)
    '\xa1', '\x01',     //COLLECTION (Application)
    '\xa1', '\x00',     //  COLLECTION (Physical)
    '\x05', '\x09',     //    USAGE_PAGE (Button)
    '\x19', '\x01',     //    USAGE_MINIMUM (Button 1)
    '\x29', '\x08',     //    USAGE_MAXIMUM (Button 8)
    '\x15', '\x00',     //    LOGICAL_MINIMUM (0)
    '\x25', '\x01',     //    LOGICAL_MAXIMUM (1)
    '\x95', '\x08',     //    REPORT_COUNT (8)
    '\x75', '\x01',     //    REPORT_SIZE (1)
    '\x81', '\x02',     //    INPUT (Data,Var,Abs)
    '\x05', '\x01',     //    USAGE_PAGE (Generic Desktop)
    '\x09', '\x30',     //    USAGE (X)
    '\x09', '\x31',     //    USAGE (Y)
    '\x15', '\x81',     //    LOGICAL_MINIMUM (-127)
    '\x25', '\x7f',     //    LOGICAL_MAXIMUM (127)
    '\x75', '\x08',     //    REPORT_SIZE (8)
    '\x95', '\x02',     //    REPORT_COUNT (2)
    '\x81', '\x02',     //    INPUT (Data,Var,Abs)
    '\xc0',           //  END COLLECTION
    '\xc0'            //END COLLECTION
};

extern "C" uint8_t usbFunctionSetup( uint8_t data [8] )
{
        usbRequest_t const* rq = (usbRequest_t const*) data;

        if ( (rq->bmRequestType & USBRQ_TYPE_MASK) != USBRQ_TYPE_CLASS )
                return 0;

        switch ( rq->bRequest )
        {
        case USBRQ_HID_GET_REPORT:
                usbMsgPtr = (usbMsgPtr_t) report_out;
                return sizeof report_out;

        default:
                return 0;
        }
}

static void calibrateOscillator(void)
{
    uchar       step = 128;
    uchar       trialValue = 0, optimumValue;
    int         x, optimumDev, targetValue = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);

    /* do a binary search: */
    do{
        OSCCAL = trialValue + step;
        x = usbMeasureFrameLength();    // proportional to current real frequency
        if(x < targetValue)             // frequency still too low
            trialValue += step;
        step >>= 1;
    }while(step > 0);
    /* We have a precision of +/- 1 for optimum OSCCAL here */
    /* now do a neighborhood search for optimum value */
    optimumValue = trialValue;
    optimumDev = x; // this is certainly far away from optimum
    for(OSCCAL = trialValue - 1; OSCCAL <= trialValue + 1; OSCCAL++){
        x = usbMeasureFrameLength() - targetValue;
        if(x < 0)
            x = -x;
        if(x < optimumDev){
            optimumDev = x;
            optimumValue = OSCCAL;
        }
    }
    OSCCAL = optimumValue;
}


extern "C" void usbEventResetReady(void)
{
    cli();  // usbMeasureFrameLength() counts CPU cycles, so disable interrupts.
    calibrateOscillator();
    sei();
}


int main(void)
{
    wdt_enable(WDTO_1S);
    usbInit();

    usbDeviceDisconnect();
    for(int i = 0; i<250; i++) {
            wdt_reset();
            _delay_ms(2);
    }
    usbDeviceConnect();

    sei();

    getPassword();

    for ( ;; )
    {
        wdt_reset();
        usbPoll();
    }

    return 0;
}
