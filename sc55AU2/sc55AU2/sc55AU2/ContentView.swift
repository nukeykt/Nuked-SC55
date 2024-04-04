//
//  ContentView.swift
//  sc55AU2
//
//  Created by Giulio Zausa on 02.04.24.
//

import AudioToolbox
import SwiftUI

struct ContentView: View {
    @ObservedObject var hostModel: AudioUnitHostModel
    var margin = 10.0
    var doubleMargin: Double {
        margin * 2.0
    }
    
    var body: some View {
        VStack() {
            Text("\(hostModel.viewModel.title )")
                .textSelection(.enabled)
                .padding()
            VStack(alignment: .center) {
                if let viewController = hostModel.viewModel.viewController {
                    AUViewControllerUI(viewController: viewController)
                        .padding(margin)
                } else {
                    VStack() {
                        Text(hostModel.viewModel.message)
                            .padding()
                    }
                    .frame(minWidth: 400, minHeight: 200)
                }
            }
            .padding(doubleMargin)
            
            if hostModel.viewModel.showAudioControls {
                Text("Audio Playback")
                Button {
                    hostModel.isPlaying ? hostModel.stopPlaying() : hostModel.startPlaying()
                    
                } label: {
                    Text(hostModel.isPlaying ? "Stop" : "Play")
                }
            }
            if hostModel.viewModel.showMIDIContols {
                Text("MIDI Input: Enabled")
            }
            Spacer()
                .frame(height: margin)
        }
    }
}

#Preview {
    ContentView(hostModel: AudioUnitHostModel())
}
