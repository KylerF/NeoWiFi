# A flickering rainbow
import requests
import random
import time

esp_url = 'http://192.168.1.60/write'
num_leds = 29

while 1:
    payload = []
    
    for x in range(29):
            r = random.randint(0,255)
            g = random.randint(0,255)
            b = random.randint(0,255)
            
            payload.append({"index": x, "r": r, "g": g, "b": b, "w": 0})
    
    r = requests.post(esp_url, json=payload)

    time.sleep(0.01)
