// main.rs
// Rust client for the NeoWiFi project
use reqwest::{StatusCode};
use std::collections::HashMap;
use std::time::Duration;
use std::{thread, time};
use rand::Rng;

// A single pixel in the strip
#[derive(Copy, Clone)]
struct Pixel {
    r: u32,
    g: u32,
    b: u32,
    w: u32,
}

impl Pixel {
    pub fn new() -> Pixel {
        Pixel {
            // Default to 'off'
            r: 0,
            g: 0,
            b: 0,
            w: 0,
        }
    }

    // Set the color of the pixel
    pub fn set_color(&mut self, r: u32, g: u32, b: u32, w: u32) {
        self.r = r;
        self.g = g;
        self.b = b;
        self.w = w;
    }

    // Convert to a HashMap
    pub fn to_json(&self) -> HashMap<&str, u32> {
        let mut json = HashMap::new();
        json.insert("r", self.r);
        json.insert("g", self.g);
        json.insert("b", self.b);
        json.insert("w", self.w);

        return json;
    }
}

// The NeoWiFi controller
struct NeoWiFi {
    pixel_count: usize, // number of pixels in the strip
    pixels: Vec<Pixel>, // vector of pixels
    host: String, // IP address or hostname of NeoWiFi server
}

impl NeoWiFi {
    pub fn new(pixel_count: usize, host: String) -> NeoWiFi {
        NeoWiFi {
            pixel_count,
            pixels: vec!(Pixel::new(); pixel_count),
            host,
        }
    }

    // Set the color of a single pixel
    pub fn set_pixel_color(&mut self, index: usize, r: u32, g: u32, b: u32, w: u32) {
        // Return if invalid index
        if index > self.pixel_count {
            return;
        }

        // Find and set the pixel
        self.pixels[index].set_color(r, g, b, w);
    }

    // Set the color of the entire strip
    pub fn set_color(&mut self, r: u32, g: u32, b: u32, w: u32) {
        for i in 0 .. self.pixel_count {
            self.set_pixel_color(i, r, g, b, w);
        }
    }

    // Convert all pixels in the strip to a JSON consumable format
    pub fn to_json(&self) -> Vec<HashMap<&str, u32>> {
        let mut json = Vec::new();

        for i in 0 .. self.pixel_count {
            let mut this_pixel = self.pixels[i].to_json();

            // Pixels don't know where they are in the strip. Need to insert index.
            this_pixel.insert("index", i as u32);

            json.push(this_pixel);
        }

        return json;
    }

    // Send updated state to server
    pub fn send(&self) -> Result<(StatusCode), reqwest::Error> {
        // Construct body from pixel states
        let body = self.to_json();

        // Construct the request and send
        let url = format!("http://{}/write", self.host);

        // Construct a client
        let client = reqwest::Client::builder()
            .timeout(Duration::from_secs(1))
            .build().unwrap();

        // Send the request
        let resp = client.post(&url)
            .json(&body)
            .send()?;

        return Ok(resp.status());
    }
}

// An animation fo run on a NeoWiFi strip
struct Animation {
    strip: *mut NeoWiFi,
    state: Vec<Pixel>,
}

// Called asynchronously to progress an animation forward one step
trait AnimationHandler {
    fn handler(&self) -> Result<(), reqwest::Error>;
}

// Run a simple animation to show off the client
fn main() {
    let pixel_count = 29;
    let host = "192.168.1.27".to_owned();

    let mut strip = NeoWiFi::new(pixel_count, host);
    let mut rng = rand::thread_rng();

    let mut i: usize = 0;
    let mut r;
    let mut g;
    let mut b;

    loop {
        for j in 0 .. pixel_count {
            if i == j {
                r = rng.gen_range(0, 255);
                g = rng.gen_range(0, 255);
                b = rng.gen_range(0, 255);
            } else {
                r = 0;
                g = 0;
                b = 0;
            }

            strip.set_pixel_color(j, r, g, b, 0);
        }

        // Send update request and show any error responses
        let status = strip.send();
        match status {
            Ok(_code) => {},
            Err(e) => println!("{}", e)
        }

        // Keep within bounds
        i = (i + 1) % pixel_count;
    }
}
