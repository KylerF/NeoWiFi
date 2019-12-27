# A spinny random animation
import requests
import random
import time

esp_url = 'http://192.168.1.3/write'
num_leds = 256
i = 0

while 1:
    payload = []
    
    for x in range(29):
        if x == i:
            r = random.randint(0,255)
            g = random.randint(0,255)
            b = random.randint(0,255)
            
            payload.append({"index": x, "r": r, "g": g, "b": b})
        else:
            payload.append({"index": x, "r": 0, "g": 0, "b": 0})
    
    r = requests.post(esp_url, json=payload)
    i = (i + 1) % num_leds

    #time.sleep(0.01)
