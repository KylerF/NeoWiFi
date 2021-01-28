//
//  Strip.swift
//  NeoWiFi
//
//  Created by Kyler Freas on 6/1/20.
//  Copyright © 2020 Hugh Jorgan Industries. All rights reserved.
//

import Foundation

class NeoWifiStrip {
    var serverIP: String
    var pixelCount: UInt
    var timeout: UInt

    init(serverIP: String, pixelCount: UInt = 0, timeout: UInt = 1) {
        self.serverIP = serverIP
        self.pixelCount = pixelCount
        self.timeout = timeout
    }
}
