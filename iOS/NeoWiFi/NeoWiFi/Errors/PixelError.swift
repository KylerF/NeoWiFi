//
//  PixelColorError.swift
//  NeoWiFi
//
//  Created by Kyler Freas on 6/3/20.
//  Copyright © 2020 Hugh Jorgan Industries. All rights reserved.
//

/// Errors associated with pixel operations
enum PixelError: Error {
    /// Error thrown when an RGB value is not within 0-255
    case RGBValueOutOfRange(color: (Int, Int, Int))
    
    /// Error thrown when attempting to access an out-of-range pixel
    case PixelOutOfRange(index: Int, count: Int)
    
    /// Error thrown when giving a pixel a negative index
    case NegativePixelIndex(index: Int)
    
    
    var errorDescription: String? {
        switch self {
        case let .RGBValueOutOfRange(color):
            return "\(color) -> RGB value out of range. Accepted values are (0-255)"
            
        case let .PixelOutOfRange(index, count):
            if count == 0 {
                return "\(index) -> Pixel out of range. Did you call getPixelCount()?"
            } else {
                return "\(index) -> Pixel out of range. Accepted values are (0-\(count))"
            }
            
        case let .NegativePixelIndex(index):
            return "\(index) -> Pixel index canot be negative"
        }
    }
}
