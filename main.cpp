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
    #include "usi_twi_master.h"
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
uint8_t time[10];
uint8_t charIndex = 0;
//uint8_t timeSet = 0;

//uint8_t fastCounter = 0;
//uint16_t slowCounter = 0;

void otp(uint8_t password[6], uint8_t secret[], uint8_t length, uint8_t time[8])
{
    HMAC_SHA1 hmac(secret, length);
    hmac.update(time, 8);

    uint8_t digest[20];
    hmac.digest(secret, length, digest);

    uint8_t o = digest[19] & 0x0f;
    
    digest[o] &= 0x7f;
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

void setTime(void)
{
    time[0] = (0x68<<TWI_ADR_BITS) | (FALSE<<TWI_READ_BIT);
    time[1] = 0;
    eeprom_read_block(time+2, (uint8_t*)0+64, 8);
    do
    {
        //wdt_reset();
        USI_TWI_Start_Transceiver_With_Data( time, 10 );
    }
    while (USI_TWI_Get_State_Info() == USI_TWI_NO_ACK_ON_ADDRESS);
}

void getTime(void)
{
    for(uint8_t i=0; i<10; i++) time[i] = 0xaa;
    
    time[0] = (0x68<<TWI_ADR_BITS) | (FALSE<<TWI_READ_BIT);
    time[1] = 0;

    USI_TWI_Start_Transceiver_With_Data( time, 2 );
    _delay_ms(10);
    time[0] = (0x68<<TWI_ADR_BITS) | (TRUE<<TWI_READ_BIT);
    //do
        USI_TWI_Start_Transceiver_With_Data( time, 8 );
    //while (USI_TWI_Get_State_Info() == USI_TWI_NO_ACK_ON_ADDRESS);

    eeprom_write_block(time, (uint8_t*)0+81, 10);

    //year
    uint8_t y = ((time[7]>>4)&0x0f)*10 + (time[7]&0x0f);
    uint32_t u = (y+31)/4;
    u += ((uint32_t) (y+30))*365;

    //month
    uint8_t b = ((time[6]>>4)&0x01)*10 + (time[6]&0x0f);
    switch(b){
        case 12: u+=30;
        case 11: u+=31;
        case 10: u+=30;
        case 9: u+=31;
        case 8: u+=31;
        case 7: u+=30;
        case 6: u+=31;
        case 5: u+=30;
        case 4: u+=31;
        case 3: u+=28;
        case 2: u+=31;
    }
    if(!(y%4) && b>2) u++;

    //day
    b = ((time[5]>>4)&0x03)*10 + (time[5]&0x0f) - 1;
    u += b;

    //hour
    u *= 24;
    b = ((time[3]>>4)&0x03)*10 + (time[3]&0x0f);
    u += b;

    //minute
    u *= 60;
    b = ((time[2]>>4)&0x07)*10 + (time[2]&0x0f);
    u += b;

    //second
    u *= 60;
    b = ((time[1]>>4)&0x07)*10 + (time[1]&0x0f);
    u += b;

    //30 second intervals
    u /= 30;
    for(uint8_t i=0; i<4; i++)
    {
        time[i] = 0;
        time[i+4] = (uint8_t)((u >> (24 - i*8)) & 0x000000ff);
    }
    eeprom_write_block(time, (uint8_t*)0 + 96, 8);
}

void getPassword(void)
{
    uint8_t secret[40];
    uint8_t secretLen;
    eeprom_read_block(&secretLen, (uint8_t*)0, 1);
    if(secretLen > 40) secretLen = 40;
    eeprom_read_block(secret, (uint8_t*)0+1, secretLen);

    otp(password, secret, secretLen, time);

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

/*void initTimer()
{
    OCR1A = 164;
    OCR1C = 164;
    TCCR1 = 0x8f; //CTC1 and CK/16384
    TCNT1 = 0;
    TIMSK |= 1 << OCIE1A;
}

ISR(TIMER1_COMPA_vect)
{
    fastCounter = (fastCounter + 1)%25;
    if(!fastCounter) slowCounter++;
}*/


int main(void)
{
    //wdt_enable(WDTO_1S);
    DDRB &= ~(1 << PB1);
    PORTB |= 1 << PB1; //pullup input

    usbInit();
    usbDeviceDisconnect();
    for(int i = 0; i<250; i++) {
            //wdt_reset();
            _delay_ms(2);
    }
    usbDeviceConnect();

    USI_TWI_Master_Initialise();

    sei();

    for ( ;; )
    {
        //wdt_reset();
        usbPoll();

        if(!(PINB & (1<<PB1)))
        {
            if(state == WAIT && holdCounter == 0)
            {
                //if(timeSet==5) setTime();
                //if(timeSet<6) timeSet++;
                getTime();
                getPassword();
                state = SEND;
                charIndex = 0;
            }
            holdCounter = 200;
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
