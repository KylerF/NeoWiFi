# Now I shall reign in blood
import requests
import random
import time
import sys

esp_url = 'http://192.168.1.3/write'

num_leds1 = 144
num_leds2 = 103

i = num_leds1
j = 0

while 1:
    payload = []

    if i == num_leds1:
        brightness = 255
        speed = brightness / 10000 
 
    for x in range(num_leds1):
        if x == i:
            r = brightness
            
            payload.append({"index": x, "r": r, "g": 0, "b": 0})
        elif x == i + 1:
            payload.append({"index": x, "r": 5, "g": 0, "b": 0})
            
    for x in range(num_leds2):
        if x == j:
            r = brightness
            payload.append({"index": x + num_leds1, "r": r, "g": 0, "b": 0})
        elif x == j - 1:
            payload.append({"index": x + num_leds1, "r": 5, "g": 0, "b": 0})

    try:
        r = requests.post(esp_url, json=payload, timeout=1)
        i = (i - 1) % num_leds1
        j = (j + 1) % num_leds2
        time.sleep(speed)
    
    except requests.exceptions.ConnectionError as e:
        print("Error Connecting: ", e)
        
        if "Max retries" in str(e):
            sys.exit(1)


