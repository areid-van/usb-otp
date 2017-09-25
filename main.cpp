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



struct KeyboardReport{
        KeyboardReport():modifier(0), reserved(0)
        {
            for(uint8_t i=0; i<6; i++) keycode[i] = 0;
        }
        uint8_t& operator[](uint8_t i){return keycode[i];}

        uint8_t modifier;
        uint8_t reserved;
        uint8_t keycode[6];
};

KeyboardReport report;
uint8_t idleRate = 0xff;
uint8_t ledState = 0xff;

#define INIT 0
#define WAIT 1
#define SEND 2
#define RELEASE 3
uint8_t state = INIT;
uint8_t holdCounter = 0;
uint8_t password[6];
uint8_t charIndex = 0;
uint8_t counter = 0;


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
    eeprom_read_block(time, (uint8_t*)0 + 16 + counter*16, 8);

    otp(password, secret, 10, time);
    eeprom_write_block(password, (uint8_t*)0 + 26 + counter*16, 6);

    for(uint8_t i=7; i<8; i--)
    {
        time[i]++;
        if(time[i]) break;
    }
    counter++;
    eeprom_write_block(time, (uint8_t*)0 + 16 + counter*16, 8);
}

PROGMEM const char usbHidReportDescriptor [USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
    '\x05', '\x01',                    // USAGE_PAGE (Generic Desktop)
    '\x09', '\x06',                    // USAGE (Keyboard)
    '\xa1', '\x01',                    // COLLECTION (Application)
    '\x75', '\x01',                    //   REPORT_SIZE (1)
    '\x95', '\x08',                    //   REPORT_COUNT (8)
    '\x05', '\x07',                    //   USAGE_PAGE (Keyboard)(Key Codes)
    '\x19', '\xe0',                    //   USAGE_MINIMUM (Keyboard LeftControl)(224)
    '\x29', '\xe7',                    //   USAGE_MAXIMUM (Keyboard Right GUI)(231)
    '\x15', '\x00',                    //   LOGICAL_MINIMUM (0)
    '\x25', '\x01',                    //   LOGICAL_MAXIMUM (1)
    '\x81', '\x02',                    //   INPUT (Data,Var,Abs) ; Modifier byte
    '\x95', '\x01',                    //   REPORT_COUNT (1)
    '\x75', '\x08',                    //   REPORT_SIZE (8)
    '\x81', '\x03',                    //   INPUT (Cnst,Var,Abs) ; Reserved byte
    '\x95', '\x05',                    //   REPORT_COUNT (5)
    '\x75', '\x01',                    //   REPORT_SIZE (1)
    '\x05', '\x08',                    //   USAGE_PAGE (LEDs)
    '\x19', '\x01',                    //   USAGE_MINIMUM (Num Lock)
    '\x29', '\x05',                    //   USAGE_MAXIMUM (Kana)
    '\x91', '\x02',                    //   OUTPUT (Data,Var,Abs) ; LED report
    '\x95', '\x01',                    //   REPORT_COUNT (1)
    '\x75', '\x03',                    //   REPORT_SIZE (3)
    '\x91', '\x03',                    //   OUTPUT (Cnst,Var,Abs) ; LED report padding
    '\x95', '\x06',                    //   REPORT_COUNT (6)
    '\x75', '\x08',                    //   REPORT_SIZE (8)
    '\x15', '\x00',                    //   LOGICAL_MINIMUM (0)
    '\x25', '\x65',                    //   LOGICAL_MAXIMUM (101)
    '\x05', '\x07',                    //   USAGE_PAGE (Keyboard)(Key Codes)
    '\x19', '\x00',                    //   USAGE_MINIMUM (Reserved (no event indicated))(0)
    '\x29', '\x65',                    //   USAGE_MAXIMUM (Keyboard Application)(101)
    '\x81', '\x00',                    //   INPUT (Data,Ary,Abs)
    '\xc0'                           // END_COLLECTION
};

extern "C" usbMsgLen_t usbFunctionSetup(uint8_t data[8])
{
    usbRequest_t *rq = reinterpret_cast<usbRequest_t*>(data);

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS)
    {
        switch(rq->bRequest)
        {
            case USBRQ_HID_GET_REPORT:
                usbMsgPtr = reinterpret_cast<usbMsgPtr_t>(&report);
                report[0] = 0;
                return sizeof(report);
            case USBRQ_HID_SET_REPORT: 
                return (rq->wLength.word == 1) ? USB_NO_MSG : 0;
            case USBRQ_HID_GET_IDLE: 
                usbMsgPtr = reinterpret_cast<usbMsgPtr_t>(&idleRate);
                return 1;
            case USBRQ_HID_SET_IDLE:
                idleRate = rq->wValue.bytes[1];
                return 0;
        }
    }

    return 0;
}

extern "C" usbMsgLen_t usbFunctionWrite(uint8_t * data, uchar len)
{
    if(state == INIT)
    {
        state = WAIT;
        PORTB |= 1;
    }

    if(data[0] == ledState) return 1;
    ledState = data[0];
    return 1;
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
    DDRB |= 1 << PB0;
    DDRB &= ~(1 << PB1);
    PORTB |= 1 << PB1; //pullup input

    usbInit();
    usbDeviceDisconnect();
    for(int i = 0; i<250; i++) {
            wdt_reset();
            _delay_ms(2);
    }
    usbDeviceConnect();

    sei();

    for ( ;; )
    {
        wdt_reset();
        usbPoll();

        if(!(PINB & (1<<PB1)))
        {
            if(state == WAIT && holdCounter == 0)
            {
                getPassword();
                state = SEND;
                charIndex = 0;
            }
            holdCounter = 255;
        }

        if(holdCounter > 0) holdCounter--;

        if(usbInterruptIsReady())
        {
            switch(state)
            {
                case SEND:
                    report[0] = 30 + (password[charIndex]-39)%10;
                    charIndex++;
                    state = RELEASE;
                    break;
                case RELEASE:
                    report[0] = 0;
                    if(charIndex<6) state = SEND;
                    else state = WAIT; 
                    break;
                default:
                    continue;
            }

            usbSetInterrupt(reinterpret_cast<unsigned char*>(&report), sizeof(report));
        }

    }

    return 0;

}
