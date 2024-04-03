//
//  MIDIManager.swift
//  sc55AU2
//
//  Created by Giulio Zausa on 02.04.24.
//

import Foundation
import CoreMIDI

class MIDIManager: Identifiable, ObservableObject {
    let id = UUID()
    private var client = MIDIClientRef()
    private var port = MIDIPortRef()
    
    static let shared = MIDIManager()
    
    private var clientName: CFString {
        guard let bundleName = Bundle.main.infoDictionary?["CFBundleName"] as? String else {
            return "MIDIManager_Client" as CFString
        }
        return "\(bundleName)_MIDIClient" as CFString
    }
    
    private var sources: [MIDIEndpointRef] {
        var srcs = [MIDIEndpointRef]()
        
        for index in 0..<MIDIGetNumberOfSources() {
            srcs.append(MIDIGetSource(index))
        }
        return srcs
    }
    
    private var portName: CFString {
        "MIDI Input Port (2.0)" as CFString
    }
    
    private func setupClient() -> Bool {
        let status = MIDIClientCreateWithBlock(clientName, &client, { notificationPointer in
            let pointer = UnsafeRawPointer(notificationPointer)
            
            switch notificationPointer.pointee.messageID {
            case .msgObjectAdded:
                let msg = pointer.assumingMemoryBound(to: MIDIObjectAddRemoveNotification.self).pointee
                guard msg.childType == .source else {
                    break
                }
                
                let res = MIDIPortConnectSource(self.port, msg.child, nil)
                if res != noErr {
                    print("Failed to connect to MIDI Source: \(msg.child)")
                }
                
            case .msgObjectRemoved:
                let msg = pointer.assumingMemoryBound(to: MIDIObjectAddRemoveNotification.self).pointee
                guard msg.childType == .source else {
                    break
                }
                
                let res = MIDIPortDisconnectSource(self.port, msg.child)
                if res != noErr {
                    print("Failed to disconnect MIDI Source: \(msg.child)")
                }
                
            case .msgIOError:
                let msg = pointer.assumingMemoryBound(to: MIDIIOErrorNotification.self).pointee
                print("IO Error Occurred: \(msg.errorCode)")
                
            default:
                break
            }
        })
        
        guard status == noErr else {
            print("Failed to create MIDI Client")
            return false
        }
        return true
    }
    
    func setupPort(midiProtocol: MIDIProtocolID, receiveBlock: @escaping MIDIReceiveBlock) -> Bool {
        guard setupClient() else {
            return false
        }
        
        if MIDIInputPortCreateWithProtocol(client, portName, midiProtocol, &port, receiveBlock) != noErr {
            return false
        }
        
        for source in self.sources {
            if MIDIPortConnectSource(port, source, nil) != noErr {
                print("Failed to connect to source \(source)")
                return false
            }
        }
        return true
    }
}
