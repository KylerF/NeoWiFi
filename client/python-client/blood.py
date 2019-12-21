# Now I shall reign in blood
import requests
import random
import time
import sys

esp_url = 'http://192.168.1.27/write'

num_leds = 29
i = 0

while 1:
    payload = []

    if i == 0:
        brightness = random.randint(0, 255)
        speed = brightness / 10000 
 
    for x in range(num_leds):
        if x == i:
            r = brightness
            
            payload.append({"index": x, "r": r, "g": 0, "b": 0, "w": 0})
        else:
            payload.append({"index": x, "r": 0, "g": 0, "b": 0, "w": 0})
    
    try:
        r = requests.post(esp_url, json=payload, timeout=1)
        i = (i + 1) % num_leds
        time.sleep(speed)
    except requests.exceptions.ConnectionError as e:
        print("Error Connecting: ", e)
        
        if 'Max retries' in str(e):
            sys.exit(1)

