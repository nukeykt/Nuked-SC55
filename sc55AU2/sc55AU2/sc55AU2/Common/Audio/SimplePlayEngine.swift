//
//  SimplePlayEngine.swift
//  sc55AU2
//
//  Created by Giulio Zausa on 02.04.24.
//

import Foundation
import CoreAudioKit
import AVFoundation

#if os(iOS)
import UIKit
#elseif os(macOS)
import AppKit
#endif

/// Wraps and Audio Unit extension and provides helper functions.
extension AVAudioUnit {

    var wantsAudioInput: Bool {
        let componentType = self.auAudioUnit.componentDescription.componentType
        return componentType == kAudioUnitType_MusicEffect || componentType == kAudioUnitType_Effect
    }
    
    static fileprivate func findComponent(type: String, subType: String, manufacturer: String) -> AVAudioUnitComponent? {
        // Make a component description matching any Audio Unit of the selected component type.
        let description = AudioComponentDescription(componentType: type.fourCharCode!,
                                                    componentSubType: subType.fourCharCode!,
                                                    componentManufacturer: manufacturer.fourCharCode!,
                                                    componentFlags: 0,
                                                    componentFlagsMask: 0)
        return AVAudioUnitComponentManager.shared().components(matching: description).first
    }
    
    fileprivate func loadAudioUnitViewController(completion: @escaping (ViewController?) -> Void) {
        auAudioUnit.requestViewController { [weak self] viewController in
            DispatchQueue.main.async {
                if #available(macOS 13.0, iOS 16.0, *) {
                    if let self = self, viewController == nil {
                            let genericViewController = AUGenericViewController()
                            genericViewController.auAudioUnit = self.auAudioUnit
                            completion(genericViewController)
                            return
                    }
                }
                completion(viewController)
            }
        }
    }
}

/// Manages the interaction with the AudioToolbox and AVFoundation frameworks.
public class SimplePlayEngine {
    
    private var avAudioUnit: AVAudioUnit?
    
    // Synchronizes starting/stopping the engine and scheduling file segments.
    private let stateChangeQueue = DispatchQueue(label: "com.example.apple-samplecode.StateChangeQueue")
    
    // Playback engine.
    private let engine = AVAudioEngine()
    
    // Engine's player node.
    private let player = AVAudioPlayerNode()
    
    // File to play.
    private var file: AVAudioFile?
    
    // Whether we are playing.
    private (set) var isPlaying = false
    
    // This block will be called every render cycle and will receive MIDI events
    private let midiOutBlock: AUMIDIOutputEventBlock = { sampleTime, cable, length, data in return noErr }
    
    // This block can be used to send MIDI UMP events to the Audio Unit
    var scheduleMIDIEventListBlock: AUMIDIEventListBlock? = nil
    
    // MARK: Initialization
    
    public init() {
        engine.attach(player)
        
        guard let fileURL = Bundle.main.url(forResource: "Synth", withExtension: "aif") else {
            fatalError("\"Synth.aif\" file not found.")
        }
        setPlayerFile(fileURL)
        
        engine.prepare()
        setupMIDI()
    }
    
    private func setupMIDI() {
        if !MIDIManager.shared.setupPort(midiProtocol: MIDIProtocolID._2_0, receiveBlock: { [weak self] eventList, _ in
            if let scheduleMIDIEventListBlock = self?.scheduleMIDIEventListBlock {
                _ = scheduleMIDIEventListBlock(AUEventSampleTimeImmediate, 0, eventList)
            }
        }) {
            fatalError("Failed to setup Core MIDI")
        }
    }
    
    func initComponent(type: String, subType: String, manufacturer: String, completion: @escaping (Result<Bool, Error>, ViewController?) -> Void) {
        // Reset the engine to remove any configured audio units.
        reset()
        
        guard let component = AVAudioUnit.findComponent(type: type, subType: subType, manufacturer: manufacturer) else {
            fatalError("Failed to find component with type: \(type), subtype: \(subType), manufacturer: \(manufacturer))" )
        }
        
        // Instantiate the audio unit.
        AVAudioUnit.instantiate(with: component.audioComponentDescription,
                                options: AudioComponentInstantiationOptions.loadOutOfProcess) { avAudioUnit, error in
            
            guard let audioUnit = avAudioUnit, error == nil else {
                completion(.failure(error!), nil)
                return
            }
            
            self.avAudioUnit = audioUnit
            
            self.connect(avAudioUnit: audioUnit)
            
            // Load view controller and call completion handler
            audioUnit.loadAudioUnitViewController { viewController in
                completion(.success(true), viewController)
            }
        }
    }
    
    private func setPlayerFile(_ fileURL: URL) {
        do {
            let file = try AVAudioFile(forReading: fileURL)
            self.file = file
            engine.connect(player, to: engine.mainMixerNode, format: file.processingFormat)
        } catch {
            fatalError("Could not create AVAudioFile instance. error: \(error).")
        }
    }
    
    private func setSessionActive(_ active: Bool) {
#if os(iOS)
        do {
            let session = AVAudioSession.sharedInstance()
            try session.setCategory(.playback, mode: .default)
            try session.setActive(active)
        } catch {
            fatalError("Could not set Audio Session active \(active). error: \(error).")
        }
#endif
    }
    
    // MARK: Playback State
    
    public func startPlaying() {
        stateChangeQueue.sync {
            if !self.isPlaying { self.startPlayingInternal() }
        }
    }
    
    public func stopPlaying() {
        stateChangeQueue.sync {
            if self.isPlaying { self.stopPlayingInternal() }
        }
    }
    
    public func togglePlay() -> Bool {
        if isPlaying {
            stopPlaying()
        } else {
            startPlaying()
        }
        return isPlaying
    }
    
    private func startPlayingInternal() {
        guard let avAudioUnit = self.avAudioUnit else {
            return
        }
        
        // assumptions: we are protected by stateChangeQueue. we are not playing.
        setSessionActive(true)
        
        if avAudioUnit.wantsAudioInput {
            // Schedule buffers on the player.
            scheduleEffectLoop()
            scheduleEffectLoop()
        }
        
        let hardwareFormat = engine.outputNode.outputFormat(forBus: 0)
        engine.connect(engine.mainMixerNode, to: engine.outputNode, format: hardwareFormat)
        
        // Start the engine.
        do {
            try engine.start()
        } catch {
            isPlaying = false
            fatalError("Could not start engine. error: \(error).")
        }
        
        if avAudioUnit.wantsAudioInput {
            // Start the player.
            player.play()
        }
        
        isPlaying = true
    }
    
    private func stopPlayingInternal() {
        guard let avAudioUnit = self.avAudioUnit else {
            return
        }
        
        if avAudioUnit.wantsAudioInput {
            player.stop()
        }
        engine.stop()
        isPlaying = false
        setSessionActive(false)
    }
    
    private func scheduleEffectLoop() {
        guard let file = file else {
            fatalError("`file` must not be nil in \(#function).")
        }
        
        player.scheduleFile(file, at: nil) {
            self.stateChangeQueue.async {
                if self.isPlaying {
                    self.scheduleEffectLoop()
                }
            }
        }
    }
    
    private func resetAudioLoop() {
        guard let avAudioUnit = self.avAudioUnit else {
            return
        }
        
        if avAudioUnit.wantsAudioInput {
            // Connect player -> mixer.
            guard let format = file?.processingFormat else { fatalError("No AVAudioFile defined (processing format unavailable).") }
            engine.connect(player, to: engine.mainMixerNode, format: format)
        }
    }
    
    public func reset() {
        connect(avAudioUnit: nil)
    }
    
    public func connect(avAudioUnit: AVAudioUnit?, completion: @escaping (() -> Void) = {}) {
        guard let avAudioUnit = self.avAudioUnit else {
            return
        }
        
        // Break the audio unit -> mixer connection
        engine.disconnectNodeInput(engine.mainMixerNode)
        
        resetAudioLoop()
        
        // We're done with the unit; release all references.
        engine.detach(avAudioUnit)
        
        // Internal function to resume playing and call the completion handler.
        func rewiringComplete() {
            scheduleMIDIEventListBlock = auAudioUnit.scheduleMIDIEventListBlock
            if isPlaying {
                player.play()
            }
            completion()
        }
        
        let hardwareFormat = engine.outputNode.outputFormat(forBus: 0)
        
        // Connect the main mixer -> output node
        engine.connect(engine.mainMixerNode, to: engine.outputNode, format: hardwareFormat)
        
        // Pause the player before re-wiring it. It is not simple to keep it playing across an insertion or deletion.
        if isPlaying {
            player.pause()
        }
        
        let auAudioUnit = avAudioUnit.auAudioUnit
        
        if !auAudioUnit.midiOutputNames.isEmpty {
            auAudioUnit.midiOutputEventBlock = midiOutBlock
        }
        
        // Attach the AVAudioUnit the the graph.
        engine.attach(avAudioUnit)
        
        if avAudioUnit.wantsAudioInput {
            // Disconnect the player -> mixer.
            engine.disconnectNodeInput(engine.mainMixerNode)
            
            // Connect the player -> effect -> mixer.
            if let format = file?.processingFormat {
                engine.connect(player, to: avAudioUnit, format: format)
                engine.connect(avAudioUnit, to: engine.mainMixerNode, format: format)
            }
        } else {
            let stereoFormat = AVAudioFormat(standardFormatWithSampleRate: hardwareFormat.sampleRate, channels: 2)
            engine.connect(avAudioUnit, to: engine.mainMixerNode, format: stereoFormat)
        }
        rewiringComplete()
    }
}
