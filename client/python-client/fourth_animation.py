# Supposed to look like snow or something
import requests
import random
import time

esp_url = 'http://192.168.1.60/write'
num_leds = 29

while 1:
    payload = []
    
    for x in range(29):
            w = random.randint(50,255)
            
            payload.append({"index": x, "r": 0, "g": 0, "b": 0, "w": w})
    
    r = requests.post(esp_url, json=payload)

    time.sleep(0.05)
