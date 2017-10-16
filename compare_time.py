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

import usbmfa
import datetime

d = usbmfa.getTime()
d2 = datetime.datetime.utcnow()
print(d.isoformat())
print(d2.isoformat())
print((d-d2).total_seconds())
