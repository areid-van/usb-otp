import datetime as dt
import time
import sys

t = dt.datetime.utcnow() + dt.timedelta(minutes=5)
sys.stderr.write(str(t))

h = '0000030: %016x0000000000000000' % int(time.time())

h += '\n0000040: '
h += '%02x' % ((int(t.second/10) << 4) | (t.second % 10))
h += '%02x' % ((int(t.minute/10) << 4) | (t.minute % 10))
h += '%02x' % ((int(t.hour/10) << 4) | (t.hour % 10))
h += '%02x' % (t.weekday() + 1)
h += '%02x' % ((int(t.day/10) << 4) | (t.day % 10))
h += '%02x' % ((int(t.month/10) << 4) | (t.month % 10))
y = t.year - 2000
h += '%02x' % ((int(y/10) << 4) | (y % 10))
h += '030000000000000000'

print(h)
