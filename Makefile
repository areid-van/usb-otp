avr-otp: sha1.cpp main.cpp
	avr-g++ -Wall -Os -DF_CPU=16000000 -mmcu=attiny85 -o avr-otp sha1.cpp main.cpp

avr-otp.hex: avr-otp
	avr-objcopy -j .text -j .data -O ihex avr-otp avr-otp.hex
	avr-size avr-otp

flash: avr-otp.hex
	avrdude -c usbtiny -P usb -p t85 -U flash:w:avr-otp.hex:i

clean:
	rm -f avr-otp avr-otp.hex otp

test: sha1.cpp main.cpp
	g++ -o otp sha1.cpp main.cpp

