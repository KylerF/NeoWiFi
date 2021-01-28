from NeoWifiClient import NeoWifiStrip
import random

strip = NeoWifiStrip("192.168.1.111")
pixel_count = strip.get_pixel_count()

strip.clear()

while 1:
    for x in range(pixel_count):
        r = random.randint(0, 255)
        g = random.randint(0, 255)
        b = random.randint(0, 255)

        strip.write(x, r, g, b)

    strip.update()
