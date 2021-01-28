//
//  NeoWifiStrip.swift
//  NeoWiFi
//
//  Created by Kyler Freas on 6/1/20.
//  Copyright © 2020 Hugh Jorgan Industries. All rights reserved.
//

import Foundation

/// Handles communication with the NeoWiFi server
class NeoWifiStrip {
    var serverIP: String
    var baseURL: String
    
    var pixelCount: Int
    var pixels: [Pixel] = []
    
    var timeout: Int
    var maxRequestSize: Int

    init(serverIP: String, pixelCount: Int = 0, timeout: Int = 1, maxRequestSize: Int = 40) {
        self.serverIP = serverIP
        self.pixelCount = pixelCount
        self.timeout = timeout
        self.maxRequestSize = maxRequestSize
        
        self.baseURL = "http://\(serverIP)"
        
        if pixelCount > 0 {
            // Initialize all pixels in the strip
            self.pixels = (0...pixelCount-1).map { Pixel(index: $0) }
        }
    }
    
    private func sendGetRequest(strURL: String) {
        // Create URL
        let url = URL(string: strURL)
        guard let requestUrl = url else { fatalError() }
        
        // Create URL Request
        var request = URLRequest(url: requestUrl)
        
        // Specify HTTP Method to use
        request.httpMethod = "GET"
        
        // Send HTTP Request
        let task = URLSession.shared.dataTask(with: request) { (data, response, error) in
            
            // Check if Error took place
            if let error = error {
                print("Error took place \(error)")
                return
            }
            
            // Read HTTP Response Status code
            if let response = response as? HTTPURLResponse {
                print("Response HTTP Status code: \(response.statusCode)")
            }
            
            // Convert HTTP Response Data to a simple String
            if let data = data, let dataString = String(data: data, encoding: .utf8) {
                print("Response data string:\n \(dataString)")
            }
        }
        
        task.resume()
    }
    
    private func sendPostRequest(strURL: String, json: Dictionary<String, UInt>) {
        
    }
    
    private func chunkify(payload: [Dictionary<String, UInt>]) {
        
    }
}
