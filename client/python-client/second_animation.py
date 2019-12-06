# Basically a laptop light
from itertools import cycle, chain
import requests
import random
import time

esp_url = 'http://192.168.1.60/write'
num_leds = 29

for i in cycle(chain(range(0, 100, 1), range(100, 0, -1))):
    payload = []
    
    for j in range(num_leds):
            payload.append({
                "index": j, 
                "r": i, 
                "g": 0, 
                "b": 0, 
                "w": 0
            })
    
    resp = requests.post(esp_url, json=payload)
    #time.sleep(0.001)
