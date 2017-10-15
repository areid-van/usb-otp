import usbmfa
import datetime

usbmfa.setTime();
print("Device time set to current time")
d = usbmfa.getTime()
d2 = datetime.datetime.utcnow()
print("Current time: ", d.isoformat())
print("Device time: ", d2.isoformat())
