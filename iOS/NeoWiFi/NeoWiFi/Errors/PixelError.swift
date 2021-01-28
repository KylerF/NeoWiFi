//
//  PixelColorError.swift
//  NeoWiFi
//
//  Created by Kyler Freas on 6/3/20.
//  Copyright © 2020 Hugh Jorgan Industries. All rights reserved.
//

import Foundation

enum PixelError: Error {
    case rgbValueOutOfRange(color: (UInt, UInt, UInt))
}
