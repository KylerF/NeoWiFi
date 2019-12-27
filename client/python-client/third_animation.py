# A flickering rainbow
import requests
import random
import time

esp_url = 'http://192.168.1.110/write'
num_leds = 200

while 1:
    payload = []
    
    for x in range(num_leds):
            r = random.randint(0,255)
            g = random.randint(0,255)
            b = random.randint(0,255)
            
            r = requests.post(esp_url, json=[{"index": x, "r": r, "g": g, "b": b, "w": 0}])

    #time.sleep(0.01)
