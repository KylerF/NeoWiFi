"""
Client API for the NeoWiFi server.
Example usage can be found in example_client.py.
"""
import requests


class NeoWifiStrip:
    """
    Main client API class with methods to interface with the NeoWiFi server
    """
    def __init__(self, server_ip, count=0, timeout=1):
        """
        Constructor method

        :param server_ip: IP address of ESP8266 running the NeoPixels
        :param count: Optional parameter - sets a static number of pixels before
                      querying it from the server
        :param timeout: Connection timeout. Default is 1 second.
        """
        self.server_ip = server_ip
        self.pixel_count = count
        self.timeout = timeout

        self.base_url = "http://{0}".format(server_ip)
        self.pixels = []

        # Populate the strip if a count was provided
        if count > 0:
            for i in range(count):
                self.pixels.append(Pixel(i))

        # Maximum size of each request
        self.max_request_size = 40

    def get_pixel_count(self):
        """
        Request the number of pixels in the strip

        :return: Number of pixels in the strip
        """
        url = "{0}/count".format(self.base_url)
        response = self.__send_get_request(url, self.timeout)

        try:
            count = int(response.text)
        except ValueError:
            raise InvalidResponseException(response.text)

        # Populate the strip with blank pixels
        for i in range(count):
            self.pixels.append(Pixel(i))

        self.pixel_count = count

        return count

    def on(self):
        """
        Turn the strip on
        Sets all pixels to their last saved state
        """
        url = "{0}/on".format(self.base_url)
        self.__send_get_request(url, self.timeout)

    def off(self):
        """
        Turn the strip off
        """
        url = "{0}/off".format(self.base_url)
        self.__send_get_request(url, self.timeout)

    def write(self, index, r, g, b):
        """
        Set a single pixel to a given color

        :param index: index of the pixel
        :param r: red value (0 - 255)
        :param g: green value (0 - 255)
        :param b: blue value (0 - 255)
        """
        try:
            pixel = self.pixels[index]
        except IndexError:
            raise PixelNotInRangeException(self.pixel_count)

        pixel.set_color(r, g, b)

    def fill(self, r, g, b):
        """
        Fill the strip with a single color

        :param r: Red value
        :param g: Green value
        :param b: Blue value
        """
        for i in range(self.pixel_count):
            self.write(i, r, g, b)

    def clear(self):
        """
        Clear all pixels in the strip
        """
        for i in range(self.pixel_count):
            self.write(i, 0, 0, 0)

    def update(self):
        """
        Send the current pixel colors to the server. Updates are sent
        in batches if greater than the max JSON size for HTTP requests.
        """
        url = "{0}/write".format(self.base_url)
        payload = []

        for pixel in self.pixels:
            # Only send the pixels that were changed
            if pixel.needs_update:
                payload.append(pixel.to_json())
                pixel.needs_update = False

        # Split up the payload if needed. If the payload size is less than the maximum body size,
        # chunkify will return the original payload.
        for chunk in list(self.__chunkify(payload, self.max_request_size)):
            self.__send_post_request(url, chunk, self.timeout)

    def get_animations(self):
        """
        Request a list of all available preset animations

        :return: A dictionary containing animations ({key=id, value=name})
        """
        pass

    def start_animation(self, animation_id):
        """
        Start playing a preset animation

        :param animation_id: ID for the preset animation
        """
        url = "{0}/preset".format(self.base_url)
        payload = {
            "id": animation_id
        }

        response = self.__send_post_request(url, payload)

        if response.status_code == 400:
            # Invalid animation ID
            raise PresetAnimationNotFoundException(animation_id)

    def stop_animation(self):
        """
        Stop the currently playing animation
        """
        url = "{0}/stop".format(self.base_url)
        self.__send_get_request(url, self.timeout)

    def __chunkify(self, payload, length):
        """
        Split a payload into successive n-sized chunks

        :param payload: List to split up
        :param length: Size of each chunk

        :return: The payload, split into chunks
        """
        for i in range(0, len(payload), length):
            yield payload[i:i + length]

    def __send_get_request(self, url, retries=0):
        """
        Helper function to send a GET request

        :param url: URL to send request

        :return: The response sent back from the server
        """
        try:
            response = requests.get(url, timeout=self.timeout)
        except requests.exceptions.ConnectionError as e:
            print("Error Connecting: ", e)

            if "Max retries" in str(e):
                raise ServerTimeoutException(url)

            return self.__send_get_request(url, retries + 1)

        # Catch 404 errors
        if "404" in response.text:
            raise EndpointNotFoundException(url)

        # Catch 405 errors
        if "405" in response.text:
            raise EndpointWrongMethodException(url)

        return response

    def __send_post_request(self, url, json, retries=0):
        """
        Helper function to send POST request

        :param url: URL to send request
        :param json: JSON data to send

        :return: The response sent back from the server
        """
        try:
            response = requests.post(url, json=json, timeout=self.timeout)
        except requests.exceptions.ConnectionError as e:
            print("Error Connecting: ", e)

            if "Max retries" in str(e):
                raise ServerTimeoutException(url)

            return self.__send_post_request(url, json, retries + 1)

        # Catch 404 errors
        if "404" in response.text:
            raise EndpointNotFoundException(url)

        return response


class Pixel:
    """
    A single NeoPixel
    """
    def __init__(self, index, r=0, g=0, b=0):
        """
        Initialize color to 0 (off), and set index

        :param index: Index within the strip
        :param r: Red value - defaults to 0
        :param g: Green value - defaults to 0
        :param b: Blue value - defaults to 0
        """
        self.index = index
        self.color = (r, g, b)

        # Whether this pixel has been changed since the last update.
        # Will only be sent to the server if this is true.
        self.needs_update = False

    def set_color(self, r, g, b):
        """
        Set the RGB color of the pixel

        :param r: red value (0 - 255)
        :param g: green value (0 - 255)
        :param b: blue value (0 - 255)
        """
        color = (r, g, b)

        # Check that the values are in range
        r_valid = (0 <= r <= 255)
        g_valid = (0 <= g <= 255)
        b_valid = (0 <= b <= 255)

        if not (r_valid and g_valid and b_valid):
            raise RGBValueOutOfRangeException(color)

        self.color = color
        self.needs_update = True

    def to_json(self):
        """
        Convert the pixel to a dictionary, which is sent to the server as JSON

        :return: Dictionary containing the pixel data
        """
        return {"index": self.index, "r": self.color[0], "g": self.color[1], "b": self.color[2]}


class PixelOutOfRangeException(Exception):
    """
    Exception raised for attempting to access an out-of-range pixel
    """
    def __init__(self, count):
        self.count = count

        # Check for 0 count. May just need to call get_pixel_count.
        if self.count == 0:
            self.message = "{0} -> Pixel out of range. Did you call get_pixel_count()?".format(self.count)
        else:
            self.message = "{0} -> Pixel out of range".format(self.count)

        super().__init__(self.message)


class RGBValueOutOfRangeException(Exception):
    """
    Exception raised when an RGB value is not within 0-255
    """
    def __init__(self, color_value):
        self.color_value = color_value
        self.message = "{0} -> RGB value out of range. Accepted values are (0-255).".format(self.color_value)

        super().__init__(self.message)


class PresetAnimationNotFoundException(Exception):
    """
    Exception raised when attempting to start a preset animation with an invalid ID
    """
    def __init__(self, preset_id):
        self.preset_id = preset_id
        self.message = "{0} -> Preset animation ID not found".format(self.preset_id)

        super().__init__(self.message)


class EndpointNotFoundException(Exception):
    """
    Exception raised when a requested endpoint cannot be found (404)
    """
    def __init__(self, endpoint):
        self.message = "{0} -> 404: The requested endpoint does not exist. The NeoWiFi server may be running an " \
                       "older firmware version that does not support this.".format(endpoint)

        super().__init__(self.message)


class EndpointWrongMethodException(Exception):
    """
    Exception raised when the wrong request method was used (405)
    """
    def __init__(self, endpoint):
        self.message = "{0} -> 405: The wrong request method was used for this endpoint. The NeoWiFi server may be " \
                       "running an older firmware version that does not support this.".format(endpoint)

        super().__init__(self.message)


class InvalidResponseException(Exception):
    """
    Exception raised when an invalid response is received from the server
    """
    def __init__(self, response_text):
        self.response_text = response_text
        self.message = "{0} -> Invalid response received from the NeoWiFi server".format(response_text)

        super().__init__(self.message)


class ServerTimeoutException(Exception):
    """
    Exception raised when connection to the server times out, and max retries exceeded
    """
    def __init__(self, url):
        self.url = url
        self.message = "Connection timed out. Max retries exceeded with url: {0}".format(url)

        super().__init__(self.message)
