#
#  An implementation of the RFC 6238 time based OTP protocol on an AtTiny85 microcontroller
#
#  Copyright (C) 2017 Adam Reid
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

import usb.core
import usb.util
import datetime
import base64

VENDOR_ID = 0x4242
PRODUCT_ID = 0xe131

def connect():
    """Connect to the USB device

    Internal utility function used inside the module

    Returns:
        A device handle
    """
    device = usb.core.find(find_all=False, idVendor=VENDOR_ID, idProduct=PRODUCT_ID)
    if device.is_kernel_driver_active(0):
        device.detach_kernel_driver(0)
    return device

def getTime():
    """Get the current time on the device

    Returns: device time as a datetime object
    """
    device = connect()
    ba = device.ctrl_transfer(0x80 | 0x20, 0x1, 0x2, 0, 9)

    sec = (ba[1] & 0xf) + (ba[1]>>4 & 0x7)*10
    minu = (ba[2] & 0xf) + (ba[2]>>4 & 0x7)*10
    hr = (ba[3] & 0xf) + (ba[3]>>4 & 0x3)*10
    day = (ba[5] & 0xf) + (ba[5]>>4 & 0x3)*10
    mnth = (ba[6] & 0xf) + (ba[6]>>4 & 0x1)*10
    yr = (ba[7] & 0xf) + (ba[7]>>4 & 0xf)*10 + 2000

    return datetime.datetime(yr, mnth, day, hr, minu, sec)

def setTime():
    """Set the time on the device to the current time

    Current time is read from the system clock
    """
    t = datetime.datetime.utcnow()

    d = [2]
    sec = t.second + 1
    d.append((int(sec/10) << 4) | (sec % 10))
    d.append((int(t.minute/10) << 4) | (t.minute % 10))
    d.append((int(t.hour/10) << 4) | (t.hour % 10))
    d.append(t.weekday() + 1)
    d.append((int(t.day/10) << 4) | (t.day % 10))
    d.append((int(t.month/10) << 4) | (t.month % 10))
    y = t.year - 2000
    d.append((int(y/10) << 4) | (y % 10))
    d.append(3)
    b = bytes(bytearray(d))

    device = connect()
    device.ctrl_transfer(0x20, 0x9, 0x2, 0, b)

def setSecret(secret):
    """Set the secret on the device
    
    Args:
        secret (str): a base32 encoded string containing the secret.
           Both upper and lower case are allowed. Spaces will be ignored.
           The decode secret must be no longer than 40 bytes.
    """
    k = list(base64.b32decode(secret.upper().replace(' ','')))
    if len(k) > 40: raise ValueError
    b = bytes(bytearray([3, len(k)] + k + ([0] * (40-len(k)))))
    device = connect()
    device.ctrl_transfer(0x20, 0x9, 0x3, 0, b)

