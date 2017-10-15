import usb.core
import usb.util
import datetime

VENDOR_ID = 0x4242
PRODUCT_ID = 0xe131

def connect():
    device = usb.core.find(find_all=False, idVendor=VENDOR_ID, idProduct=PRODUCT_ID)
    if device.is_kernel_driver_active(0):
        device.detach_kernel_driver(0)
    return device

def getTime():
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


d = getTime()
d2 = datetime.datetime.utcnow()
print(d.isoformat())
print(d2.isoformat())
print((d-d2).total_seconds())
#setTime()
