avr-otp: sha1.h sha1.cpp hmac_sha1.h hmac_sha1.cpp main.cpp usbdrv/usbdrv.c usbdrv/usbdrvasm.S usbdrv/oddebug.c usi_twi_master.c usi_twi_master.h usbconfig.h
	avr-gcc -I. -Wall -Os -DF_CPU=16500000 -mmcu=attiny85 -c usbdrv/usbdrv.c usbdrv/usbdrvasm.S usbdrv/oddebug.c usi_twi_master.c
	avr-g++ -I. -Wall -Os -DF_CPU=16500000 -mmcu=attiny85 -o avr-otp usbdrv.o usbdrvasm.o oddebug.o usi_twi_master.o sha1.cpp hmac_sha1.cpp main.cpp

avr-otp.hex: avr-otp
	avr-objcopy -j .text -j .data -O ihex avr-otp avr-otp.hex
	avr-size avr-otp

flash: avr-otp.hex
	avrdude -c usbtiny -P usb -p t85 -U flash:w:avr-otp.hex:i

fuse:
	avrdude -c usbtiny -P usb -p t85 -U hfuse:w:0xdd:m -U lfuse:w:0xe1:m

clean:
	rm -f avr-otp avr-otp.hex otp eeprom.hex eeprom.bin *.o

