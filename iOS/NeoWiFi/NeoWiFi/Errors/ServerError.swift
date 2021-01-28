//
//  ServerError.swift
//  NeoWiFi
//
//  Created by Kyler Freas on 6/3/20.
//  Copyright © 2020 Hugh Jorgan Industries. All rights reserved.
//

import Foundation

enum ServerError: Error {
    case presetAnimationNotFound(presetID: UInt)
    case endpointNotFound(endpoint: String)
    case endpointWrongMethod(endpoint: String)
    case invalidResponse(responseText: String)
    case serverTimeout(url: String)
}
