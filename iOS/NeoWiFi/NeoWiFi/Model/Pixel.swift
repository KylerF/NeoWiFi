//
//  Pixel.swift
//  NeoWiFi
//
//  Created by Kyler Freas on 6/1/20.
//  Copyright © 2020 Hugh Jorgan Industries. All rights reserved.
//

/// Represents a single NeoPixel within a strip
class Pixel {
    var index: Int
    var color: (Int, Int, Int) = (0, 0, 0)
    var needsUpdate: Bool = false

    /// Set pixel's index within a strip
    init(index: Int) {
        self.index = index
    }
    
    /// Set the RGB color of the pixel
    public func setColor(r: Int, g: Int, b: Int) throws {
        let color = (r, g, b)
        
        let rValid = 0...255 ~= r
        let gValid = 0...255 ~= g
        let bValid = 0...255 ~= b
        
        if !(rValid && gValid && bValid) {
            throw PixelError.RGBValueOutOfRange(color: color)
        }
        
        self.color = color
        self.needsUpdate = true
    }
    
    /// Convert the pixel to a dictionary, which is sent to the server as JSON
    public func toJSON() -> Dictionary<String, Int> {
        return [
            "index": self.index,
            "r": self.color.0,
            "g": self.color.1,
            "b": self.color.2
        ]
    }
}
