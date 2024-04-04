//
//  AudioUnitHostModel.swift
//  sc55AU2
//
//  Created by Giulio Zausa on 02.04.24.
//

import SwiftUI
import CoreMIDI
import AudioToolbox

class AudioUnitHostModel: ObservableObject {
    /// The playback engine used to play audio.
    private let playEngine = SimplePlayEngine()

    /// The model providing information about the current Audio Unit
    @Published private(set) var viewModel = AudioUnitViewModel()

    var isPlaying: Bool {
        playEngine.isPlaying
    }

    /// Audio Component Description
    let type: String
    let subType: String
    let manufacturer: String

    let wantsAudio: Bool
    let wantsMIDI: Bool
    let isFreeRunning: Bool

    let auValString: String

    init(type: String = "aumu", subType: String = "SC55", manufacturer: String = "SC55") {
        self.type = type
        self.subType = subType
        self.manufacturer = manufacturer
        let wantsAudio = type.fourCharCode == kAudioUnitType_MusicEffect || type.fourCharCode == kAudioUnitType_Effect
        self.wantsAudio = wantsAudio

        let wantsMIDI = type.fourCharCode == kAudioUnitType_MIDIProcessor ||
        type.fourCharCode == kAudioUnitType_MusicDevice ||
        type.fourCharCode == kAudioUnitType_MusicEffect
        self.wantsMIDI = wantsMIDI

        let isFreeRunning = type.fourCharCode == kAudioUnitType_MIDIProcessor ||
        type.fourCharCode == kAudioUnitType_MusicDevice ||
        type.fourCharCode == kAudioUnitType_Generator
        self.isFreeRunning = isFreeRunning

        auValString = "\(type) \(subType) \(manufacturer)"

        loadAudioUnit()
    }

    private func loadAudioUnit() {
        playEngine.initComponent(type: type,
                                 subType: subType,
                                 manufacturer: manufacturer) { [self] result, viewController in
            switch result {
            case .success(_):
                self.viewModel = AudioUnitViewModel(showAudioControls: self.wantsAudio,
                                                    showMIDIContols: self.wantsMIDI,
                                                    title: self.auValString,
                                                    message: "Successfully loaded (\(self.auValString))",
                                                    viewController: viewController)

                if self.isFreeRunning {
                    self.playEngine.startPlaying()
                }
            case .failure(let error):
                self.viewModel = AudioUnitViewModel(showAudioControls: false,
                                                    showMIDIContols: false,
                                                    title: self.auValString,
                                                    message: "Failed to load Audio Unit with error: \(error.localizedDescription)",
                                                    viewController: nil)
            }
        }
    }

    func startPlaying() {
        playEngine.startPlaying()
    }

    func stopPlaying() {
        playEngine.stopPlaying()
    }
}
