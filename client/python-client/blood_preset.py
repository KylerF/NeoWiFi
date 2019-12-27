# Now I shall reign in blood
import requests

esp_url = "http://192.168.1.110"

def start_bleeding(retries = 0):
    url = esp_url + "/preset"
    json = {
        "id": 1
    }

    try:
        r = requests.post(url, json = json, timeout = 1)
    except requests.exceptions.ConnectionError as e:
        print("Error Connecting: ", e)

        if "Max retries" in str(e):
            return

        start_bleeding(retries + 1)

def stop_bleeding(retries = 0):
    url = esp_url + "/stop"
    try:
        r = requests.get(url, timeout = 1)
    except requests.exceptions.ConnectionError as e:
        print("Error Connecting: ", e)
        
        if "Max retries" in str(e):
            return

        stop_bleeding(retries + 1)

def bleed_off(retries = 0):
    url = esp_url + "/off"
    
    try:
        r = requests.get(url, timeout = 1)
    except requests.exceptions.ConnectionError as e:
        print("Error Connecting: ", e)
        
        if "Max retries" in str(e):
            return

        stop_bleeding(retries + 1)

if __name__ == "__main__":
    import time

    start_bleeding()
    time.sleep(10)
    bleed_off()

