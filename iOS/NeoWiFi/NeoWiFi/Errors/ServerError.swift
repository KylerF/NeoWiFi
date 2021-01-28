//
//  ServerError.swift
//  NeoWiFi
//
//  Created by Kyler Freas on 6/3/20.
//  Copyright © 2020 Hugh Jorgan Industries. All rights reserved.
//

/// Errors associated with NeoWiFi Server communication
enum ServerError: Error {
    /// Error thrown when a preset animation ID does not exist
    case PresetAnimationNotFound(presetID: Int)
    
    /// Error thrown when attempting to request a non-existent API endpoint
    case EndpointNotFound(endpoint: String)
    
    /// Error thrown when using the wrong request method on a given API endpoing
    case EndpointWrongMethod(endpoint: String)
    
    /// Error thrown when an unexpected reqponse is returned from the API
    case InvalidResponse(responseText: String)
    
    /// Error thrown when communication with the server times out
    case ServerTimeout(url: String)
    
    
    var errorDescription: String? {
        switch self {
        case let .PresetAnimationNotFound(presetID):
            return "\(presetID) -> Preset animation ID not found"
                
        case let .EndpointNotFound(endpoint):
            return "\(endpoint) -> 404: The requested endpoint does not exist. The NeoWiFi server may be running an older firmware version that does not support this."
            
        case let .EndpointWrongMethod(endpoint):
            return "\(endpoint) -> 405: The wrong request method was used for this endpoint. The NeoWiFi server may be running an older firmware version that does not support this."
            
        case let .InvalidResponse(responseText):
            return "\(responseText) -> Invalid response received from the NeoWiFi server."
            
        case let .ServerTimeout(url):
            return "Connection timed out. Max retries exceeded with url: \(url)"
        }
    }
}
