from NeoWifiClient import NeoWifiStrip
from time import sleep

strip = NeoWifiStrip("192.168.1.111")
pixel_count = strip.get_pixel_count()

strip.clear()

for x in range(pixel_count):
    strip.write(x, 255, 0, 0)
    strip.update()

for x in reversed(range(pixel_count)):
    strip.write(x, 0, 0, 0)
    strip.update()


for x in range(pixel_count):
    strip.write(x, 0, 255, 0)
    strip.update()

for x in reversed(range(pixel_count)):
    strip.write(x, 0, 0, 0)
    strip.update()


for x in range(pixel_count):
    strip.write(x, 0, 0, 255)
    strip.update()

for x in reversed(range(pixel_count)):
    strip.write(x, 0, 0, 0)
    strip.update()

