//
//  sc55AU2ExtensionMainView.swift
//  sc55AU2Extension
//
//  Created by Giulio Zausa on 02.04.24.
//

import SwiftUI

struct sc55AU2ExtensionMainView: View {
//    var parameterTree: ObservableAUParameterGroup
    var audioUnit: sc55AU2ExtensionAudioUnit?
    
    var body: some View {
        TabView {
            EmulatorView(audioUnit: audioUnit).tabItem { Text("Emulator") }
        }
    }
}

#Preview {
    sc55AU2ExtensionMainView()
}
