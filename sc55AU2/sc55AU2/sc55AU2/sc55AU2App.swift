//
//  sc55AU2App.swift
//  sc55AU2
//
//  Created by Giulio Zausa on 02.04.24.
//

import CoreMIDI
import SwiftUI

@main
struct sc55AU2App: App {
    @ObservedObject private var hostModel = AudioUnitHostModel()

    var body: some Scene {
        WindowGroup {
            ContentView(hostModel: hostModel)
        }
    }
}
